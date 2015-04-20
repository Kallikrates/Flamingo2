#ifndef CORE_HPP
#define CORE_HPP

#include <QApplication>

class MainWindow;

class Core : public QApplication {
	Q_OBJECT
public:
	Core(int & argc, char * * & argv);
	virtual ~Core();
private:
	MainWindow * window = nullptr;
};

#endif //CORE_HPP
