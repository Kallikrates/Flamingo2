#include "overlayer.hpp"

#include <QtWidgets>
#include <QtGui>

inline static void byteConstrain(int & in) {
	if (in > 255) in = 255;
	if (in < 0) in = 0;
}

Overlayer::Overlayer(QWidget * parent) : QWidget(parent) {
	this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	this->setPalette(Qt::transparent);
	this->setAttribute(Qt::WA_TransparentForMouseEvents);

	flickerer = new QTimer(this);
	flickerer->setSingleShot(false);
	QObject::connect(flickerer, SIGNAL(timeout()), this, SLOT(manageFlicker()));
	flickerer->start(25);
}

Overlayer::~Overlayer() {
	flickerer->stop();
	delete flickerer;
}

void Overlayer::setFlicker(Flicker f, bool b) {
	switch (f) {
	case Flicker::Load:
		if (b && loadFlickerS) break;
		loadFlickerS = b ? 1 : 0;
		loadFlickerV = 0;
		break;
	case Flicker::Bilinear:
		if (b && bilFlickerS) break;
		bilFlickerS = b ? 1 : 0;
		bilFlickerV = 0;
		break;
	}
}

void Overlayer::paintEvent(QPaintEvent *QPE) {
	QPE->accept();
	QPainter paint {this};
	paint.setPen(Qt::NoPen);
	if (loadFlickerS) {
		QBrush lb {QColor(255, 0, 0, loadFlickerV), Qt::SolidPattern};
		paint.setBrush(lb);
		paint.drawRect(indOffs, this->height() - (indOffs + indSize), indSize, indSize);
	}
	if (bilFlickerS) {
		QBrush lb {QColor(0, 255, 0, bilFlickerV), Qt::SolidPattern};
		paint.setBrush(lb);
		paint.drawRect(indOffs + (indOffs + indSize), this->height() - (indOffs + indSize), indSize, indSize);
	}
}

static int flm = 255 / 4;

void Overlayer::manageFlicker() {
	switch (loadFlickerS) {
	case 0:
		break;
	case -1:
		if (loadFlickerV <= 0) {
			loadFlickerS = 1;
		} else loadFlickerV-=flm;
		break;
	case 1:
		if (loadFlickerV >= 255) {
			loadFlickerS = -1;
		} else loadFlickerV+=flm;
		break;
	}
	byteConstrain(loadFlickerV);

	switch (bilFlickerS) {
	case 0:
		break;
	case -1:
		if (bilFlickerV <= 0) {
			bilFlickerS = 1;
		} else bilFlickerV-=flm;
		break;
	case 1:
		if (bilFlickerV >= 255) {
			bilFlickerS = -1;
		} else bilFlickerV+=flm;
		break;
	}
	byteConstrain(bilFlickerV);

	this->repaint();
}
