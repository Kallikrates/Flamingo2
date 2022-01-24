#include "core.hpp"
#include "mainwindow.hpp"

#include <QTextCodec>

Core::Core(int &argc, char **&argv) : QApplication(argc, argv) {
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

	qDebug() << QImageReader::supportedImageFormats();
	window = new MainWindow(this->arguments());
	window->show();
}

Core::~Core() {
	delete window;
}

int main (int argc, char * * argv) {
	Core C {argc, argv};
	return C.exec();
}
