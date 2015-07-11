#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "common.hpp"
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
	virtual void mousePressEvent(QMouseEvent *);
	virtual void closeEvent(QCloseEvent *);
protected slots:
	void handleImage(QImage);
	void handleBilProc();
	void handleBilComp();
private:
	QGridLayout * layout = nullptr;
	ImageView * view = nullptr;
	AsyncImageProvider * provider = nullptr;
	Overlayer * over = nullptr;
	OptionsWindow * opwin = nullptr;

	void slideshowRun();
	std::atomic_bool slideshowActive {false};
	std::thread * slideshowThread = nullptr;

	QSettings settings {"Sensory Systems", "Flamingo2"};
	Options options;
private slots:
	void handleOptionsApplied();
};

#endif //MAINWINDOW_HPP
