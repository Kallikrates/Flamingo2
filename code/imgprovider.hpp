#ifndef IMGPROVIDER_HPP
#define IMGPROVIDER_HPP

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <QObject>
#include <QImage>
#include <QList>
#include <QTimer>
#include <QFile>
#include <QDir>

#include "providerargs.hpp"

class AsyncImageProvider : public QObject {
	Q_OBJECT
signals:
	void Loaded(QImage);
public:
	virtual ~AsyncImageProvider() {}
	virtual QString const & CurrentName() = 0;
public slots:
	virtual void Current() = 0;
	virtual void Next() = 0;
	virtual void Previous() = 0;
	virtual void Random() = 0;
	virtual void Remove() = 0;
protected:
	AsyncImageProvider(QObject * parent = 0) : QObject(parent) {}
};

class PreloadingWeightedCategoryImageProvider : public AsyncImageProvider {
	Q_OBJECT
public:
	PreloadingWeightedCategoryImageProvider(ProviderArgs const & args);
	virtual ~PreloadingWeightedCategoryImageProvider();
	virtual QString const & CurrentName();
public slots:
	virtual void Current();
	virtual void Next();
	virtual void Previous();
	virtual void Random();
	virtual void Remove();
	virtual void Remove(QString);
	virtual void Seek(QString);
protected:
	struct ImgEntry {QString path; QImage img;};
	struct CatEntry {QString catname; float weight; QList<std::shared_ptr<ImgEntry>> entries;};
	enum class Navdir {Neutral, Forward, Backward, Random};
	unsigned int maxThreads = std::thread::hardware_concurrency();
	unsigned int numPreload = 5;
	inline std::shared_ptr<ImgEntry> getCurrentEntry();
private:
	unsigned int cindex {0};
	unsigned int lindex {0};
	Navdir navdir {Navdir::Neutral};
	QTimer * workClock = nullptr;
	QTimer * clearClock = nullptr;
	std::recursive_mutex workLock;
	std::recursive_mutex indexLock;
	struct PreloadSet {unsigned int cindex; unsigned int lindex;};
	QList<PreloadSet> preloads;
	struct IETL {std::shared_ptr<ImgEntry> entry; std::thread * thread; std::atomic<bool> finished;};
	QList<CatEntry> cats;
	QList<IETL *> loaders;
	void loadrun(IETL *);
	void peekIndex(unsigned int & cinout, unsigned int & linout, int delta = 0);
	inline PreloadSet generateRandomPreload();
	void resetRandom();
	PreloadSet advanceRandom();
	inline void providerArgDirRecursor(QDir from, QList<QString> & paths);
private slots:
	void workTick();
	void clearTick();
};

#endif //IMGPROVIDER_HPP
