#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QWidget>
#include <QtWidgets>

#include "imgprovider.hpp"

class ImageView;

class MainWindow : public QWidget {
	Q_OBJECT
public:
	MainWindow(QStringList arguments);
	virtual ~MainWindow();
protected:
	virtual void keyPressEvent(QKeyEvent *);
protected slots:
	void handleImage(QImage);
private:
	QGridLayout * layout = nullptr;
	ImageView * view = nullptr;
	AsyncImageProvider * provider = nullptr;
};

#endif //MAINWINDOW_HPP
