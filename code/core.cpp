#include "core.hpp"
#include "mainwindow.hpp"

Core::Core(int &argc, char **&argv) : QApplication(argc, argv) {
	window = new MainWindow();
	window->show();
}

Core::~Core() {
	delete window;
}

int main (int argc, char * * argv) {
	Core C {argc, argv};
	return C.exec();
}
