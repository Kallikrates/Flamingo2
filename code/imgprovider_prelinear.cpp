#include "imgprovider.hpp"

#include <QFile>
#include <QDir>
#include <QImageReader>
#include <set>

PreloadingLinearImageProvider::PreloadingLinearImageProvider() : AsyncImageProvider(0) {
	QFileInfoList qfil = QDir::current().entryInfoList(QDir::Files);
	for (QFileInfo & fi : qfil) {
		entries.append(std::shared_ptr<ImgEntry>(new ImgEntry {fi.canonicalFilePath(), QImage(0, 0, QImage::Format_Mono)}));
	}
	workClock = new QTimer(this);
	QObject::connect(workClock, SIGNAL(timeout()), this, SLOT(workTick()));
	workClock->setSingleShot(false);
	workClock->start(25);
}

PreloadingLinearImageProvider::~PreloadingLinearImageProvider() {
	workLock.lock();
	workClock->stop();
	workLock.unlock();
	for (IETL * ietl : loaders) {
		if (ietl->thread->joinable()) ietl->thread->join();
		delete ietl->thread;
		delete ietl;
	}
	delete workClock;
}

void PreloadingLinearImageProvider::Current() {
	if (entries.length() == 0) return;
	workLock.lock();
	indexLock.lock();
	if (!entries.at(index)->img.isNull()) emit Loaded(entries.at(index)->img);
	indexLock.unlock();
	workLock.unlock();
}

void PreloadingLinearImageProvider::Next() {
	indexLock.lock();
	index = this->peekIndex(1);
	indexLock.unlock();
	navdir = Navdir::Forward;
	Current();
}

void PreloadingLinearImageProvider::Previous() {
	indexLock.lock();
	index = this->peekIndex(-1);
	indexLock.unlock();
	navdir = Navdir::Backward;
	Current();
}

void PreloadingLinearImageProvider::Random() {
	navdir = Navdir::Neutral;
	Current();
}

void PreloadingLinearImageProvider::Remove() {
	indexLock.lock();
	entries.removeAt(index);
	switch (navdir) {
	case Navdir::Neutral:
	case Navdir::Forward:
		index = peekIndex(0);
		break;
	case Navdir::Backward:
		index = peekIndex(-1);
		break;
	}
	Current();
	indexLock.unlock();
}

void PreloadingLinearImageProvider::Remove(QString str) {
	indexLock.lock();
	for (unsigned int i = 0; i < (unsigned int)entries.length(); i++) {
		std::shared_ptr<ImgEntry> & entry = entries[i];
		if (entry->path == str) {
			if (i == index) Remove();
			else {
				entries.removeAt(i);
				if (i < index) index--;
			}
		}
	}
	indexLock.unlock();
}

void PreloadingLinearImageProvider::loadrun(IETL * me) {
	std::shared_ptr<ImgEntry> entry = me->entry;
	QImageReader reader {entry->path};
	reader.setDecideFormatFromContent(true);
	if (!reader.canRead()) this->Remove(entry->path);
	QImage img = reader.read();
	if (img.isNull()) this->Remove(entry->path);
	workLock.lock();
	entry->img = img;
	me->finished = true;
	workLock.unlock();
}

unsigned int PreloadingLinearImageProvider::peekIndex(int delta) {
	if (entries.length() == 0) return 0;
	long ndx = (long)index.load() + delta;
	while (ndx >= entries.length()) ndx -= entries.length();
	while (ndx < 0) ndx += entries.length();
	return (unsigned int)ndx;
}

void PreloadingLinearImageProvider::workTick() {
	if (entries.length() == 0) return;
	workLock.lock();
	indexLock.lock();
	QMutableListIterator<IETL *> mu {loaders};
	while (mu.hasNext()) {
		IETL * ietl = mu.next();
		if (ietl->finished) {
			mu.remove();
			if (ietl->thread->joinable()) ietl->thread->join();
			delete ietl->thread;
			auto entry = ietl->entry;
			if (entries.at(index)->path == entry->path) emit Loaded(entry->img);
			delete ietl;
		}
	}
	if (numPreload > 0) {
		unsigned int curPreload = 1;
		int stepPreload = 1;
		std::set<unsigned int> preloads {
			this->peekIndex(0),
		};
		while (curPreload < numPreload) {
			switch (navdir) {
			case Navdir::Neutral:
			case Navdir::Forward:
				preloads.emplace(this->peekIndex(stepPreload));
				curPreload++;
				if (curPreload >= numPreload) break;
				preloads.emplace(this->peekIndex(-stepPreload));
				curPreload++;
				stepPreload++;
				break;
			case Navdir::Backward:
				preloads.emplace(this->peekIndex(-stepPreload));
				curPreload++;
				if (curPreload >= numPreload) break;
				preloads.emplace(this->peekIndex(stepPreload));
				curPreload++;
				stepPreload++;
				break;
			}
		}
		for (unsigned int p : preloads) {
			if ((unsigned int)loaders.length() < maxThreads) {
				auto & entry = entries[p];
				if (!entry->img.isNull()) continue;
				bool gtg = true;
				for (IETL * ietl : loaders) {
					if (entries[p]->path == ietl->entry->path) {
						gtg = false; break;
					}
				}
				if (gtg) {
					IETL * ietl = new IETL {entry, nullptr, {false}};
					ietl->thread = new std::thread(std::bind(&PreloadingLinearImageProvider::loadrun, this, ietl));
					loaders.append(ietl);
				}
			}
		}
		for (unsigned int i = 0; i < (unsigned int)entries.length(); i++) {
			bool unload = true;
			for (unsigned int p : preloads) if (p == i) unload = false;
			if (unload) entries[i]->img = QImage {0, 0, QImage::Format_Mono};
		}

	}
	indexLock.unlock();
	workLock.unlock();
}
