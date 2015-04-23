#include "mainwindow.hpp"
#include "imgview.hpp"
#include "overlayer.hpp"

MainWindow::MainWindow(QStringList arguments) : QWidget(0) {
	layout = new QGridLayout(this);
	layout->setMargin(0);
	view = new ImageView(this);
	over = new Overlayer(this);
	layout->addWidget(view, 0, 0, 1, 1);
	layout->addWidget(over, 0, 0, 1, 1);
	QObject::connect(view, SIGNAL(bilProc()), this, SLOT(handleBilProc()));
	QObject::connect(view, SIGNAL(bilComplete()), this, SLOT(handleBilComp()));
	provider = new PreloadingWeightedCategoryImageProvider(arguments);
	QObject::connect(provider, SIGNAL(Loaded(QImage)), this, SLOT(handleImage(QImage)));
	provider->Current();
}

MainWindow::~MainWindow() {
	delete over;
	delete view;
	delete provider;
	delete layout;
}

void MainWindow::keyPressEvent(QKeyEvent * QKE) {
	QKE->accept();
	switch (QKE->key()) {
	case Qt::Key_Right:
	case Qt::Key_Left:
	case Qt::Key_Up:
	case Qt::Key_Delete:
		over->setFlicker(Overlayer::Flicker::Load, true);
		break;
	}
	switch (QKE->key()) {
	case Qt::Key_Escape:
		this->close();
		break;
	case Qt::Key_Right:
		provider->Next();
		break;
	case Qt::Key_Left:
		provider->Previous();
		break;
	case Qt::Key_Up:
		provider->Random();
		break;
	case Qt::Key_Delete:
		provider->Remove();
		break;
	default:
		return;
	}
}

void MainWindow::paintEvent(QPaintEvent * QPE) {
	//over->setGeometry(0, 0, this->width(), this->height());
	QWidget::paintEvent(QPE);
}

void MainWindow::handleImage(QImage img) {
	this->setWindowTitle("f2: "+provider->CurrentName());
	over->setFlicker(Overlayer::Flicker::Load, false);
	view->setImage(img);
}

void MainWindow::handleBilProc() {
	over->setFlicker(Overlayer::Flicker::Bilinear, true);
}

void MainWindow::handleBilComp() {
	over->setFlicker(Overlayer::Flicker::Bilinear, false);
}
