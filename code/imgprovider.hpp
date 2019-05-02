#ifndef IMGPROVIDER_HPP
#define IMGPROVIDER_HPP

#include "common.hpp"

#include <QImage>
#include <QList>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QCollator>

#include "rwmutex.hpp"
#include "providerargs.hpp"

class AsyncImageProvider : public QObject {
	Q_OBJECT
signals:
	void Loaded(QImage);
public:
	virtual ~AsyncImageProvider() {}
	virtual QString CurrentName() = 0;
	virtual QString const & CurrentPath() = 0;
	virtual void SetProviderArguments(ProviderArgs const & args) = 0;
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
	PreloadingWeightedCategoryImageProvider();
	virtual ~PreloadingWeightedCategoryImageProvider();
	virtual QString CurrentName();
	virtual QString const & CurrentPath();
	virtual void SetProviderArguments(ProviderArgs const & args);
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
	void validateRandom();
	bool indexIsValid(unsigned int, unsigned int);
private:
	unsigned int cindex {0};
	unsigned int lindex {0};
	Navdir navdir {Navdir::Neutral};
	QTimer * workClock = nullptr;
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
	inline void providerArgDirRecursor(QDir from, QList<QString> & paths, int max_depth, int cur_depth, QCollator & col);
	QList<std::shared_ptr<ImgEntry>> loaded;
	rwmutex pargs_m;

private slots:
	void workTick();
};

#endif //IMGPROVIDER_HPP
