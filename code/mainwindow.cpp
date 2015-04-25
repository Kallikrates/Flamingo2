#include "mainwindow.hpp"
#include "imgview.hpp"
#include "overlayer.hpp"

MainWindow::MainWindow(QStringList arguments) : QWidget(0) {
	layout = new QGridLayout(this);
	layout->setMargin(0);
	view = new ImageView(this);
	over = new Overlayer(this);
	over->setFlicker(Overlayer::Flicker::Load, true);
	layout->addWidget(view, 0, 0, 1, 1);
	layout->addWidget(over, 0, 0, 1, 1);
	QObject::connect(view, SIGNAL(bilProc()), this, SLOT(handleBilProc()));
	QObject::connect(view, SIGNAL(bilComplete()), this, SLOT(handleBilComp()));
	opwin = new OptionsWindow(0);
	this->options = this->opwin->getOptions();
	provider = new PreloadingWeightedCategoryImageProvider(arguments);
	QObject::connect(provider, SIGNAL(Loaded(QImage)), this, SLOT(handleImage(QImage)));
	provider->Current();
}

MainWindow::~MainWindow() {
	if (slideshowActive) {
		slideshowActive.store(false);
		if (slideshowThread->joinable()) slideshowThread->join();
		delete slideshowThread;
	}
	delete opwin;
	delete over;
	delete provider;
	delete view;
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
	case Qt::Key_O:
		opwin->show();
		break;
	case Qt::Key_S:
		if (slideshowActive) {
			slideshowActive.store(false);
			if (slideshowThread->joinable()) slideshowThread->join();
			delete slideshowThread;
			over->setNotification("Slideshow Stopped");
		} else {
			slideshowActive.store(true);
			slideshowThread = new std::thread(&MainWindow::slideshowRun, this);

			over->setNotification(QString("Slideshow Started (%0 ms interval)").arg(options.slideshowInterval.count()), 120);
		}
		break;
	case Qt::Key_F:
		if (this->windowState() & Qt::WindowFullScreen) {
			this->setWindowState(this->windowState() & ~Qt::WindowFullScreen);
			over->setNotification("Fullscreen Disabled");
		} else {
			this->setWindowState(this->windowState() | Qt::WindowFullScreen);
			over->setNotification("Fullscreen Enabled");
		}
		break;
	case Qt::Key_I:
		if (over->getIndicatorsEnabled()) {
			over->setIndicatorsEnabled(false);
			over->setNotification("Load Indicators Hidden");
		} else {
			over->setIndicatorsEnabled(true);
			over->setNotification("Load Indicators Shown");
		}
	default:
		return;
	}
	this->setWindowTitle("f2: "+provider->CurrentName());
}

void MainWindow::handleImage(QImage img) {
	this->setWindowTitle("f2: "+provider->CurrentName());
	over->setFlicker(Overlayer::Flicker::Load, false);
	view->setImage(img, options.viewKeep);
}

void MainWindow::handleBilProc() {
	over->setFlicker(Overlayer::Flicker::Bilinear, true);
}

void MainWindow::handleBilComp() {
	over->setFlicker(Overlayer::Flicker::Bilinear, false);
}

void MainWindow::slideshowRun() {
	std::chrono::high_resolution_clock::time_point frameBegin = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point frameNow;
	while (slideshowActive) {
		frameNow = std::chrono::high_resolution_clock::now();
		if (frameNow - frameBegin >= options.slideshowInterval) {
			frameBegin = frameNow;
			switch(options.slideshowDir) {
			case SlideshowDirection::Forward:
				provider->Next();
				break;
			case SlideshowDirection::Backward:
				provider->Previous();
				break;
			case SlideshowDirection::Random:
				provider->Random();
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}
