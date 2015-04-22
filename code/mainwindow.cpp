#include "mainwindow.hpp"
#include "imgview.hpp"

MainWindow::MainWindow(QStringList arguments) : QWidget(0) {
	layout = new QGridLayout(this);
	layout->setMargin(0);
	view = new ImageView(this);
	layout->addWidget(view);
	provider = new PreloadingWeightedCategoryImageProvider(arguments);
	QObject::connect(provider, SIGNAL(Loaded(QImage)), this, SLOT(handleImage(QImage)));
	provider->Current();
}

MainWindow::~MainWindow() {
	delete view;
	delete provider;
	delete layout;
}

void MainWindow::keyPressEvent(QKeyEvent * QKE) {
	QKE->accept();
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

void MainWindow::handleImage(QImage img) {
	this->setWindowTitle("f2: "+provider->CurrentName());
	view->setImage(img);
}
