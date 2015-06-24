#ifndef OVERLAYER_HPP
#define OVERLAYER_HPP

#include "common.hpp"

#include <QTimer>

class Overlayer : public QWidget {
	Q_OBJECT
public:
	enum class Flicker {Load, Bilinear};
	Overlayer(QWidget * parent = 0);
	virtual ~Overlayer();
	void setFlicker(Flicker, bool);
	void setNotification(QString str, int duration = 40);
	void setIndicatorsEnabled (bool);
	bool getIndicatorsEnabled();
protected:
	qreal indOffs = 2;
	qreal indSize = 5;
	void paintEvent(QPaintEvent * QPE);
private:
	int loadFlickerV = 0;
	int loadFlickerS = 0;
	int bilFlickerV = 0;
	int bilFlickerS = 0;
	QTimer * elementClock = nullptr;
	QString notifText;
	int notifValue = 0;
	bool indicatorsEnabled {true};
private slots:
	void manageElements();
};

#endif //OVERLAYER_HPP
