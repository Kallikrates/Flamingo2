#include "imgprovider.hpp"

#include <QImageReader>
#include <QDebug>

static const QImage nullImg {0, 0, QImage::Format_Mono};

inline void PreloadingWeightedCategoryImageProvider::providerArgDirRecursor(QDir from, QList<QString> & paths) {
	QFileInfoList qfil = from.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
	for (QFileInfo fi : qfil) {
		if (fi.isFile()) paths.append(fi.canonicalFilePath());
		else providerArgDirRecursor(QDir(fi.canonicalFilePath()), paths);
	}
}

PreloadingWeightedCategoryImageProvider::PreloadingWeightedCategoryImageProvider(ProviderArgs const & args) : AsyncImageProvider(0) {
	for (ProviderArg const & arg : args.getArgs()) {
		if (arg.path.isDir()) {
			if (arg.recurse) {
				QList<QString> paths;
				providerArgDirRecursor(QDir(arg.path.canonicalFilePath()), paths);
				if (paths.length() > 0) {
					QList<std::shared_ptr<ImgEntry>> imgs;
					for (QString str : paths) {
						imgs.append(std::shared_ptr<ImgEntry> (new ImgEntry {str, QImage {0, 0, QImage::Format_Mono}}));
					}
					cats.append({arg.path.canonicalFilePath(), arg.weight, imgs});
				}
			} else {
				QFileInfoList qfil = QDir(arg.path.canonicalFilePath()).entryInfoList(QDir::Files);
				QList<std::shared_ptr<ImgEntry>> imgs;
				for (QFileInfo & fi : qfil) {
					imgs.append(std::shared_ptr<ImgEntry> (new ImgEntry {fi.canonicalFilePath(), nullImg}));
				}
				if (imgs.length() > 0) cats.append({arg.path.canonicalFilePath(), arg.weight, imgs});
			}
		} else if (arg.path.isFile()) {
			qDebug() << "TODO: Files";
		} else {
			qDebug() << "Invalid provider argument.";
		}
	}
	if (!args.getReqStart().isNull()) this->Seek(args.getReqStart());

	workClock = new QTimer(this);
	QObject::connect(workClock, SIGNAL(timeout()), this, SLOT(workTick()));
	workClock->setSingleShot(false);
	workClock->start(25);

	clearClock = new QTimer(this);
	QObject::connect(clearClock, SIGNAL(timeout()), this, SLOT(clearTick()));
	clearClock->setSingleShot(false);
	clearClock->start(500);
}

PreloadingWeightedCategoryImageProvider::~PreloadingWeightedCategoryImageProvider() {
	workLock.lock();
	workClock->stop();
	clearClock->stop();
	workLock.unlock();
	for (IETL * ietl : loaders) {
		if (ietl->thread->joinable()) ietl->thread->join();
		delete ietl->thread;
		delete ietl;
	}
	delete workClock;
	delete clearClock;
}

static const QString nullStr {};

QString const & PreloadingWeightedCategoryImageProvider::CurrentName() {
	if (cats.length() == 0) return nullStr;
	indexLock.lock();
	QString const & str = getCurrentEntry()->path;
	indexLock.unlock();
	return str;
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
	if (navdir != Navdir::Random) {
		this->resetRandom();
		navdir = Navdir::Random;
	} else {
		PreloadSet p = this->advanceRandom();
		indexLock.lock();
		cindex = p.cindex;
		lindex = p.lindex;
		indexLock.unlock();
	}
	Current();
}

void PreloadingWeightedCategoryImageProvider::Remove() {
	if (cats.length() == 0) return;
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
	case Navdir::Random:
		this->resetRandom();
		break;
	}
	Current();
	indexLock.unlock();
}

inline std::shared_ptr<PreloadingWeightedCategoryImageProvider::ImgEntry> PreloadingWeightedCategoryImageProvider::getCurrentEntry() {
	return cats[cindex].entries[lindex];
}

void PreloadingWeightedCategoryImageProvider::Remove(QString str) {
	if (cats.length() == 0) return;
	workLock.lock();
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
					if (navdir == Navdir::Random) {
						this->resetRandom();
					}
				}
				break;
			}
		}
	}
	indexLock.unlock();
	workLock.unlock();
}

void PreloadingWeightedCategoryImageProvider::Seek(QString name) {
	indexLock.lock();
	bool found = false;
	for (unsigned int c = 0; c < (unsigned int)cats.length() && !found; c++) {
		for (unsigned int l = 0; l < (unsigned int)cats[c].entries.length() && !found; l++) {
			std::shared_ptr<ImgEntry> & entry = cats[c].entries[l];
			if (entry->path == name) {
				cindex = c;
				lindex = l;
				found = true;
			}
		}
	}
	if (found) Current();
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
	workLock.unlock();
	me->finished = true;
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

inline PreloadingWeightedCategoryImageProvider::PreloadSet PreloadingWeightedCategoryImageProvider::generateRandomPreload() {
	if (cats.length() == 0) return {0, 0};
	unsigned int cdx = 0;
	if (cats.length() > 1) {
		float sum = 0.0f;
		for (CatEntry & cat : cats) sum += cat.weight;
		float value = (qrand() / (float)RAND_MAX) * sum;
		cdx = cats.length();
		while (value > 0 && cdx-- > 0) {
			value -= cats[cdx].weight;
		}
	}
	return {cdx, (unsigned int)qrand() % cats[cdx].entries.length()};
}

void PreloadingWeightedCategoryImageProvider::resetRandom() {
	workLock.lock();
	preloads.clear();
	for (unsigned int i = 0; i < numPreload; i++) preloads.append(generateRandomPreload());
	PreloadSet & p = preloads.first();
	cindex = p.cindex;
	lindex = p.lindex;
	workLock.unlock();
}

PreloadingWeightedCategoryImageProvider::PreloadSet PreloadingWeightedCategoryImageProvider::advanceRandom() {
	workLock.lock();
	preloads.removeFirst();
	PreloadSet p = preloads.front();
	preloads.append(generateRandomPreload());
	workLock.unlock();
	return p;

}


void PreloadingWeightedCategoryImageProvider::workTick() {
	if (cats.length() == 0) {
		emit Loaded(nullImg);
		return;
	}
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
		if (navdir != Navdir::Random) {
			unsigned int curPreload = 1;
			int stepPreload = 1;
			preloads.clear();
			unsigned int pc, lc;
			this->peekIndex(pc, lc);
			preloads.append({pc, lc});
			while (curPreload < numPreload) {
				switch (navdir) {
				case Navdir::Random:
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
		}
		for (PreloadSet & p : preloads) {
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
	}
	indexLock.unlock();
	workLock.unlock();
}

void PreloadingWeightedCategoryImageProvider::clearTick() {
	workLock.lock();
	indexLock.lock();
	for (unsigned int c = 0; c < (unsigned int)cats.length(); c++) {
		CatEntry const & cent = cats.at(c);
		unsigned int llen = (unsigned int)cent.entries.length();
		for (unsigned int l = 0; l < llen; l++) {
			bool unload = true;
			for (PreloadSet p : preloads) if (p.cindex == c && p.lindex == l) unload = false;
			if (unload && !cent.entries[l]->img.isNull()) {
				cent.entries[l]->img = nullImg;
			}
		}
	}
	indexLock.unlock();
	workLock.unlock();
}
