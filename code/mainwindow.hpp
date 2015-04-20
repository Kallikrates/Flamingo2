#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QWidget>
#include <QtWidgets>

#include "imgprovider.hpp"

class ImageView;

class MainWindow : public QWidget {
	Q_OBJECT
public:
	MainWindow();
	virtual ~MainWindow();
protected:
	virtual void keyPressEvent(QKeyEvent *);
private:
	QGridLayout * layout = nullptr;
	ImageView * view = nullptr;
	AsyncImageProvider * provider = nullptr;
};

#endif //MAINWINDOW_HPP
