#ifndef OVERLAYER_HPP
#define OVERLAYER_HPP

#include <QWidget>
#include <QTimer>

class Overlayer : public QWidget {
	Q_OBJECT
public:
	enum class Flicker {Load, Bilinear};
	Overlayer(QWidget * parent = 0);
	virtual ~Overlayer();
	void setFlicker(Flicker, bool);
protected:
	qreal indOffs = 3;
	qreal indSize = 10;
	void paintEvent(QPaintEvent * QPE);
private:
	int loadFlickerV = 0;
	int loadFlickerS = 0;
	int bilFlickerV = 0;
	int bilFlickerS = 0;
	QTimer * flickerer = nullptr;
private slots:
	void manageFlicker();
};

#endif //OVERLAYER_HPP
