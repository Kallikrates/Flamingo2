#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <atomic>
#include <thread>
#include <QWidget>
#include <QtWidgets>

#include "imgprovider.hpp"
#include "options.hpp"

class ImageView;
class Overlayer;

class MainWindow : public QWidget {
	Q_OBJECT
public:
	MainWindow(QStringList arguments);
	virtual ~MainWindow();
protected:
	virtual void keyPressEvent(QKeyEvent *);
protected slots:
	void handleImage(QImage);
	void handleBilProc();
	void handleBilComp();
private:
	Options options;
	QGridLayout * layout = nullptr;
	ImageView * view = nullptr;
	AsyncImageProvider * provider = nullptr;
	Overlayer * over = nullptr;
	OptionsWindow * opwin = nullptr;

	void slideshowRun();
	std::atomic_bool slideshowActive {false};
	std::thread * slideshowThread = nullptr;
};

#endif //MAINWINDOW_HPP
