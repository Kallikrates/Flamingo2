#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QWidget>
#include <QtWidgets>

#include "imgprovider.hpp"

class ImageView;
class Overlayer;

class MainWindow : public QWidget {
	Q_OBJECT
public:
	MainWindow(QStringList arguments);
	virtual ~MainWindow();
protected:
	virtual void keyPressEvent(QKeyEvent *);
	virtual void paintEvent(QPaintEvent *);
protected slots:
	void handleImage(QImage);
	void handleBilProc();
	void handleBilComp();
private:
	QGridLayout * layout = nullptr;
	ImageView * view = nullptr;
	AsyncImageProvider * provider = nullptr;
	Overlayer * over = nullptr;
};

#endif //MAINWINDOW_HPP
