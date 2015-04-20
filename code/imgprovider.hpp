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

class AsyncImageProvider : public QObject {
	Q_OBJECT
signals:
	void Loaded(QImage);
public:
	virtual ~AsyncImageProvider() {}
public slots:
	virtual void Current() = 0;
	virtual void Next() = 0;
	virtual void Previous() = 0;
	virtual void Random() = 0;
	virtual void Remove() = 0;
protected:
	AsyncImageProvider(QObject * parent = 0) : QObject(parent) {}
};

class PreloadingLinearImageProvider : public AsyncImageProvider {
	Q_OBJECT
public:
	PreloadingLinearImageProvider();
	virtual ~PreloadingLinearImageProvider();
public slots:
	virtual void Current();
	virtual void Next();
	virtual void Previous();
	virtual void Random();
	virtual void Remove();
	virtual void Remove(QString);
protected:
	struct ImgEntry {QString path; QImage img;};
	enum class Navdir {Neutral, Forward, Backward};
	unsigned int maxThreads = std::thread::hardware_concurrency();
	unsigned int numPreload = 5;
private:
	std::atomic<unsigned int> index {0};
	Navdir navdir {Navdir::Neutral};
	QTimer * workClock = nullptr;
	std::recursive_mutex workLock;
	std::recursive_mutex indexLock;
	struct IETL {std::shared_ptr<ImgEntry> entry; std::thread * thread; std::atomic<bool> finished;};
	QList<std::shared_ptr<ImgEntry>> entries;
	QList<IETL *> loaders;
	void loadrun(IETL *);
	unsigned int peekIndex(int delta);
private slots:
	void workTick();
};

class PreloadingWeightedCategoryImageProvider : public AsyncImageProvider {
	Q_OBJECT
public:
	PreloadingWeightedCategoryImageProvider();
	virtual ~PreloadingWeightedCategoryImageProvider();
public slots:
	virtual void Current();
	virtual void Next();
	virtual void Previous();
	virtual void Random();
	virtual void Remove();
	virtual void Remove(QString);
protected:
	struct ImgEntry {QString path; QImage img;};
	struct CatEntry {QString catname; float weight; QList<std::shared_ptr<ImgEntry>> entries;};
	enum class Navdir {Neutral, Forward, Backward};
	unsigned int maxThreads = std::thread::hardware_concurrency();
	unsigned int numPreload = 5;
	inline std::shared_ptr<ImgEntry> getCurrentEntry();
private:
	unsigned int cindex {0};
	unsigned int lindex {0};
	Navdir navdir {Navdir::Neutral};
	QTimer * workClock = nullptr;
	std::recursive_mutex workLock;
	std::recursive_mutex indexLock;
	struct PreloadSet {unsigned int cindex; unsigned int lindex;};
	struct IETL {std::shared_ptr<ImgEntry> entry; std::thread * thread; std::atomic<bool> finished;};
	QList<CatEntry> cats;
	QList<IETL *> loaders;
	void loadrun(IETL *);
	void peekIndex(unsigned int & cinout, unsigned int & linout, int delta = 0);
private slots:
	void workTick();
};

#endif //IMGPROVIDER_HPP
