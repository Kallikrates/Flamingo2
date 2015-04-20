#include "imgprovider.hpp"

#include <QFile>
#include <QDir>
#include <QImageReader>

PreloadingWeightedCategoryImageProvider::PreloadingWeightedCategoryImageProvider() : AsyncImageProvider(0) {
	QFileInfoList qfil = QDir::current().entryInfoList(QDir::Files);
	cats.append({"TEST", 1.0f, {}});
	for (QFileInfo & fi : qfil) {
		QString path = fi.canonicalFilePath();
		if (!path.contains("btest")) {
			cats[0].entries.append(std::shared_ptr<ImgEntry>(new ImgEntry {path, QImage(0, 0, QImage::Format_Mono)}));
		}
	}
	cats.append({"TEST2", 1.0f, {}});
	for (QFileInfo & fi : qfil) {
		QString path = fi.canonicalFilePath();
		if (path.contains("btest")) {
			cats[1].entries.append(std::shared_ptr<ImgEntry>(new ImgEntry {path, QImage(0, 0, QImage::Format_Mono)}));
		}
	}
	workClock = new QTimer(this);
	QObject::connect(workClock, SIGNAL(timeout()), this, SLOT(workTick()));
	workClock->setSingleShot(false);
	workClock->start(25);
}

PreloadingWeightedCategoryImageProvider::~PreloadingWeightedCategoryImageProvider() {
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

void PreloadingWeightedCategoryImageProvider::Current() {
	if (cats.length() == 0) return;
	workLock.lock();
	indexLock.lock();
	if (!getCurrentEntry()->img.isNull()) emit Loaded(getCurrentEntry()->img);
	indexLock.unlock();
	workLock.unlock();
}

void PreloadingWeightedCategoryImageProvider::Next() {
	indexLock.lock();
	this->peekIndex(cindex, lindex, 1);
	indexLock.unlock();
	navdir = Navdir::Forward;
	Current();
}

void PreloadingWeightedCategoryImageProvider::Previous() {
	indexLock.lock();
	this->peekIndex(cindex, lindex, -1);
	indexLock.unlock();
	navdir = Navdir::Backward;
	Current();
}

void PreloadingWeightedCategoryImageProvider::Random() {
	navdir = Navdir::Neutral;
	indexLock.lock();
	cindex = qrand() % cats.length();
	lindex = qrand() % cats[cindex].entries.length();
	indexLock.unlock();
	Current();
}

void PreloadingWeightedCategoryImageProvider::Remove() {
	indexLock.lock();
	cats[cindex].entries.removeAt(lindex);
	if (cats[cindex].entries.length() == 0) cats.removeAt(cindex);
	switch (navdir) {
	case Navdir::Neutral:
	case Navdir::Forward:
			this->peekIndex(cindex, lindex);
		break;
	case Navdir::Backward:
			this->peekIndex(cindex, lindex, -1);
		break;
	}
	Current();
	indexLock.unlock();
}

inline std::shared_ptr<PreloadingWeightedCategoryImageProvider::ImgEntry> PreloadingWeightedCategoryImageProvider::getCurrentEntry() {
	return cats[cindex].entries[lindex];
}

void PreloadingWeightedCategoryImageProvider::Remove(QString str) {
	indexLock.lock();
	for (unsigned int c = 0; c < (unsigned int)cats.length(); c++) {
		for (unsigned int l = 0; l < (unsigned int)cats[c].entries.length(); l++) {
			std::shared_ptr<ImgEntry> & entry = cats[c].entries[l];
			if (entry->path == str) {
				if (c == cindex && l == lindex) Remove();
				else {
					cats[c].entries.removeAt(l);
					if (l < lindex) lindex--;
					if (cats[cindex].entries.length() == 0) cats.removeAt(cindex);
					this->peekIndex(cindex, lindex);
				}
			}
		}
	}
	indexLock.unlock();
}

void PreloadingWeightedCategoryImageProvider::loadrun(IETL * me) {
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

void PreloadingWeightedCategoryImageProvider::peekIndex(unsigned int & cinout, unsigned int & linout, int delta) {
	if (cats.length() == 0) {
		cinout = 0;
		linout = 0;
		return;
	}
	int cdx = cindex;
	long ndx = lindex;
	if (cdx >= cats.length()) {
		cdx = 0;
		ndx = 0;
	} else 	if (cdx < 0) {
		cdx = cats.length() - 1;
		ndx = cats[cdx].entries.length() - 1;
	}
	ndx = ndx + delta;
	while (ndx >= cats[cdx].entries.length()) {
		ndx -= cats[cdx].entries.length();
		cdx++;
		if (cdx >= cats.length()) cdx = 0;
	}
	while (ndx < 0) {
		cdx--;
		if (cdx < 0) cdx += cats.length();
		ndx += cats[cdx].entries.length();
	}
	cinout = cdx;
	linout = (unsigned int)ndx;
}

void PreloadingWeightedCategoryImageProvider::workTick() {
	if (cats.length() == 0) return;
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
			if (getCurrentEntry()->path == entry->path) emit Loaded(entry->img);
			delete ietl;
		}
	}
	if (numPreload > 0) {
		unsigned int curPreload = 1;
		int stepPreload = 1;
		QList<PreloadSet> preloads {};
		unsigned int pc, lc;
		this->peekIndex(pc, lc);
		preloads.append({pc, lc});
		while (curPreload < numPreload) {
			switch (navdir) {
			case Navdir::Neutral:
			case Navdir::Forward:
				this->peekIndex(pc, lc, stepPreload);
				preloads.append({pc, lc});
				curPreload++;
				if (curPreload >= numPreload) break;
				this->peekIndex(pc, lc, -stepPreload);
				preloads.append({pc, lc});
				curPreload++;
				stepPreload++;
				break;
			case Navdir::Backward:
				this->peekIndex(pc, lc, -stepPreload);
				preloads.append({pc, lc});
				curPreload++;
				if (curPreload >= numPreload) break;
				this->peekIndex(pc, lc, stepPreload);
				preloads.append({pc, lc});
				curPreload++;
				stepPreload++;
				break;
			}
		}
		for (PreloadSet p : preloads) {
			if ((unsigned int)loaders.length() < maxThreads) {
				auto & entry = cats[p.cindex].entries[p.lindex];
				if (!entry->img.isNull()) continue;
				bool gtg = true;
				for (IETL * ietl : loaders) {
					if (entry->path == ietl->entry->path) {
						gtg = false; break;
					}
				}
				if (gtg) {
					IETL * ietl = new IETL {entry, nullptr, {false}};
					ietl->thread = new std::thread(std::bind(&PreloadingWeightedCategoryImageProvider::loadrun, this, ietl));
					loaders.append(ietl);
				}
			}
		}
		for (unsigned int c = 0; c < (unsigned int)cats.length(); c++) {
			for (unsigned int l = 0; l < (unsigned int)cats[c].entries.length(); l++) {
				bool unload = true;
				for (PreloadSet p : preloads) if (p.cindex == c && p.lindex == l) unload = false;
				if (unload) cats[c].entries[l]->img = QImage {0, 0, QImage::Format_Mono};
			}
		}

	}
	indexLock.unlock();
	workLock.unlock();
}
