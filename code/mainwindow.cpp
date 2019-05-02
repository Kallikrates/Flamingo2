#include "mainwindow.hpp"
#include "imgview.hpp"
#include "overlayer.hpp"
#include "wallp.hpp"
#include "pixelscripter.hpp"

MainWindow::MainWindow(QStringList arguments) : QWidget(0) {
	settings.beginGroup("Options");
	options.readSettings(settings);
	settings.endGroup();

	layout = new QGridLayout(this);
	layout->setMargin(0);
	view = new ImageView(this);
	over = new Overlayer(this);
	over->setFlicker(Overlayer::Flicker::Load, true);
	pixscr = new PixelScripter(nullptr);
	QObject::connect(pixscr, SIGNAL(compilation_success()), this, SLOT(handlePixScrCompilation()));
	QObject::connect(pixscr, SIGNAL(process_complete(QImage, QString)), this, SLOT(handlePixScrProcComplete(QImage, QString)));
	layout->addWidget(view, 0, 0, 1, 1);
	layout->addWidget(over, 0, 0, 1, 1);
	QObject::connect(view, SIGNAL(bilProc()), this, SLOT(handleBilProc()));
	QObject::connect(view, SIGNAL(bilComplete()), this, SLOT(handleBilComp()));
	opwin = new OptionsWindow(options, 0);
	QObject::connect(opwin, SIGNAL(applied()), this, SLOT(handleOptionsApplied()));
	provider = new PreloadingWeightedCategoryImageProvider();
	QObject::connect(provider, SIGNAL(Loaded(QImage)), this, SLOT(handleImage(QImage)));
	provider->Current();

	settings.beginGroup("MainWindow");
	this->setGeometry(settings.value("wingeo", this->geometry()).toRect());
	settings.endGroup();
	
	settings.beginGroup("PixelScript");
	pixscr->setEditorText(settings.value("saved_script", "").toString(), true);
	settings.endGroup();

	opwin->show();
	opwin->hide();
	opwin->setFixedSize(opwin->minimumSize());

	provider->SetProviderArguments({arguments});
}

MainWindow::~MainWindow() {
	if (slideshowActive) {
		slideshowActive.store(false);
		if (slideshowThread->joinable()) slideshowThread->join();
		delete slideshowThread;
	}
	delete pixscr;
	delete opwin;
	delete over;
	delete provider;
	delete view;
	delete layout;
}

void MainWindow::mousePressEvent(QMouseEvent * QME) {
	switch(QME->button()) {
	case Qt::RightButton:
		QME->accept();
		this->close();
		break;
	case Qt::ForwardButton:
		QME->accept();
		provider->Next();
		break;
	case Qt::BackButton:
		QME->accept();
		provider->Previous();
		break;
	default:
		return;
	}
}

void MainWindow::keyPressEvent(QKeyEvent * QKE) {
	QKE->accept();
	switch (QKE->key()) {
	case Qt::Key_Right:
	case Qt::Key_Left:
	case Qt::Key_Up:
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
	case Qt::Key_Delete: {
		QString cur = provider->CurrentPath();
		if (QMessageBox::warning(this, "Delete?", QString("%1\n\nThis image will be HARD deleted, it will not go to any sort of trash bin. Proceed?").arg(cur), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
			QFile(cur).remove();
			over->setFlicker(Overlayer::Flicker::Load, true);
			provider->Remove();
		}
		break; }
	case Qt::Key_O:
		{
		QSize oms = opwin->minimumSize();
		int mx = this->x() + (this->width() / 2) - oms.width() / 2;
		int my = this->y() + (this->height() / 2) - oms.height() / 2;
		opwin->setGeometry({QPoint{mx, my}, oms});
		opwin->show();
		}
		break;
	case Qt::Key_P:
		{
		QSize oms = pixscr->minimumSize();
		int mx = this->x() + (this->width() / 2) - oms.width() / 2;
		int my = this->y() + (this->height() / 2) - oms.height() / 2;
		pixscr->setGeometry({QPoint{mx, my}, oms});
		pixscr->show();
		}
		break;
	case Qt::Key_B: {
			this->view->bilGood.store(!this->view->bilGood);
			if (this->view->bilGood) {
				over->setNotification("Bilinear Sampling Enabled");
			} else {
				over->setNotification("Bilinear Sampling Disabled");
			}
		} break;
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
	case Qt::Key_M:
		if (this->windowState() & Qt::WindowMaximized) {
			this->setWindowState(this->windowState() & ~Qt::WindowMaximized);
			over->setNotification("Unmaximized");
		} else {
			this->setWindowState(this->windowState() | Qt::WindowMaximized);
			over->setNotification("Maximized");
		}
		break;
	case Qt::Key_D:
		if (this->windowFlags() & Qt::WindowStaysOnBottomHint) {
			this->setWindowFlags(this->windowFlags() & ~Qt::WindowStaysOnBottomHint);
			over->setNotification("Desktop Mode Disabled");
		} else {
			this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnBottomHint);
			over->setNotification("Desktop Mode Enabled");
		}
		this->show();
		break;
	case Qt::Key_I:
		if (over->getIndicatorsEnabled()) {
			over->setIndicatorsEnabled(false);
			over->setNotification("Load Indicators Hidden");
		} else {
			over->setIndicatorsEnabled(true);
			over->setNotification("Load Indicators Shown");
		}
		break;
	case Qt::Key_V: {
		QImage img = view->getImageOfView();
		QString loc = QFileDialog::getSaveFileName(this, "Save Image View", QString("f2view_") + provider->CurrentName(), tr("JPEG Image (*.jpg);;Portable Network Graphics (*.png)"));
		if (!loc.isEmpty()) {
			img.save(loc, nullptr, 95);
		}
		break;
	}
	case Qt::Key_W: {
		QImage img = view->getImageOfView();
		set_wallpaper(img);
		break;
	}
	case Qt::Key_1:
		view->setKeepState(ImageView::KEEP_FIT);
		break;
	case Qt::Key_2:
		view->setKeepState(ImageView::KEEP_FIT_FORCE);
		break;
	case Qt::Key_3:
		view->setKeepState(ImageView::KEEP_EXPANDED);
		break;
	case Qt::Key_4:
		view->setKeepState(ImageView::KEEP_EQUAL);
		break;
	default:
		return;
	}
	this->setWindowTitle("f2: "+provider->CurrentName());
}

void MainWindow::closeEvent(QCloseEvent *) {
	if (this->windowState() & Qt::WindowFullScreen) {
		this->setWindowState(this->windowState() & ~Qt::WindowFullScreen);
	}

	settings.beginGroup("Options");
	options.writeSetttings(settings);
	settings.endGroup();

	settings.beginGroup("MainWindow");
	settings.setValue("wingeo", this->geometry());
	settings.endGroup();
	
	settings.beginGroup("PixelScript");
	settings.setValue("saved_script", pixscr->getEditorText());
	settings.endGroup();
}

void MainWindow::handleImage(QImage img) {
	this->setWindowTitle("f2: "+provider->CurrentName());
	over->setFlicker(Overlayer::Flicker::Load, false);
	if (options.use_ps) {
		pixscr->set_process(img, provider->CurrentPath());
		over->setFlicker(Overlayer::Flicker::PixelScript, true);
	} else {
		view->setImage(img, options.viewKeep);
	}
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
		std::this_thread::sleep_for(std::chrono::microseconds(500));
	}
}

void MainWindow::handleOptionsApplied() {
	bool old_use_ps = options.use_ps;
	this->options = opwin->getOptions();
	if (options.use_ps != old_use_ps) provider->Current();
}

void MainWindow::handlePixScrProcComplete(QImage img, QString str) {
	if (provider->CurrentPath() == str) {
		over->setFlicker(Overlayer::Flicker::PixelScript, false);
		view->setImage(img, options.viewKeep);
	}
}

void MainWindow::handlePixScrCompilation() {
	if (options.use_ps) provider->Current();
}
