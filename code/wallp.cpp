#include "wallp.hpp"

#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QTemporaryFile>
#include <QImageWriter>
#include <QColorSpace>

#include <QtDBus>

char const * wallp_template = R"AWT(

local awful = require('awful')
local gears = require('gears')

coords = mouse.coords()
gears.wallpaper.fit('%1', awful.screen.getbycoord(coords.x, coords.y), false)
	
)AWT";

void set_wallpaper(QImage img) {
	
	auto iface = new QDBusInterface { "org.awesomewm.awful", "/", "org.awesomewm.awful.Remote", QDBusConnection::sessionBus() };
	
	if (!iface->isValid()) {
		qDebug() << "Failed to open DBus interface for wallpapering.";
		return;
	}
	
	QTemporaryFile img_tmp {QDir::tempPath()+QDir::separator()+"f2_XXXXXX.bmp"};
	img_tmp.open();
	
	QImageWriter pngw;
	pngw.setFormat("BMP");
	pngw.setDevice(&img_tmp);
	pngw.write(img);
	
	auto msg = iface->call("Eval", QString(wallp_template).arg(img_tmp.fileName()));
}
