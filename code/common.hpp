#ifndef COMMON_HPP
#define COMMON_HPP

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <QWidget>
#include <QtWidgets>
#include <QDebug>
#include <QSettings>

class SanePushButton : public QPushButton {
	Q_OBJECT
signals:
	void used();
public:
	SanePushButton(QString text, QWidget * parent = 0) : QPushButton(text, parent) {}
protected:
	virtual void mouseReleaseEvent(QMouseEvent * QME) {
		if (this->isDown()) emit used();
		QPushButton::mouseReleaseEvent(QME);
	}
};

#endif //COMMON_HPP
