#include "overlayer.hpp"

#include <QtGui>

inline static void byteConstrain(int & in) {
	if (in > 255) in = 255;
	if (in < 0) in = 0;
}

Overlayer::Overlayer(QWidget * parent) : QWidget(parent) {
	this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	this->setPalette(Qt::transparent);
	this->setAttribute(Qt::WA_TransparentForMouseEvents);

	elementClock = new QTimer(this);
	elementClock->setSingleShot(false);
	QObject::connect(elementClock, SIGNAL(timeout()), this, SLOT(manageElements()));
	elementClock->start(25);
}

Overlayer::~Overlayer() {
	elementClock->stop();
	delete elementClock;
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

void Overlayer::setNotification(QString str, int duration) {
	notifText = str;
	notifValue = duration;
}

void Overlayer::setIndicatorsEnabled(bool b) {
	this->indicatorsEnabled = b;
}

bool Overlayer::getIndicatorsEnabled() {
	return this->indicatorsEnabled;
}

void Overlayer::paintEvent(QPaintEvent *QPE) {
	QPE->accept();
	QPainter paint {this};
	paint.setPen(Qt::NoPen);
	paint.setCompositionMode(QPainter::CompositionMode_Difference);
	if (loadFlickerS) {
		QBrush lb {QColor(255, 0, 127, indicatorsEnabled ? loadFlickerV : 0), Qt::SolidPattern};
		paint.setBrush(lb);
		paint.drawRect(indOffs, indOffs, indSize, indSize);
	}
	if (bilFlickerS) {
		QBrush lb {QColor(0, 255, 127, indicatorsEnabled ? bilFlickerV : 0), Qt::SolidPattern};
		paint.setBrush(lb);
		paint.drawRect(indOffs + (indOffs + indSize), indOffs, indSize, indSize);
	}
	if (notifValue) {
		int bv = notifValue * 16;
		byteConstrain(bv);
		paint.setPen(QColor(255, 255, 255, bv));
		paint.setFont(QFont("Monospace", 8, QFont::Bold));
		paint.drawText(this->rect().adjusted(indOffs, 0, 0, -indOffs), Qt::AlignBottom | Qt::AlignLeft, notifText);
	}
}

static int flm = 255 / 4;

void Overlayer::manageElements() {
	bool doRepaint = false;

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

	if (notifValue >= 0) notifValue--;

	if (loadFlickerS || bilFlickerS) doRepaint = true;
	else if (notifValue >= 0) doRepaint = true;
	if (doRepaint) this->repaint();
}
