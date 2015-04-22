#include "providerargs.hpp"

#include <QDebug>

ProviderArgs::ProviderArgs(QList<QString> inargs) {
	inargs.removeFirst();
	bool recur = false;
	float weight = 1.0f;
	if (inargs.length() == 1) {
		QFileInfo fi {inargs[0]};
		if (fi.exists()) {
			QFileInfo dir;
			if (fi.isFile()) {
				dir = {fi.canonicalPath()};
				reqStart = fi.canonicalFilePath();
			}
			else dir = {fi.canonicalFilePath()};
			args.append({dir, false, 1.0f});
		}
	} else if (inargs.length() == 0) args.append({QFileInfo(QDir::current().canonicalPath()), false, 1.0f});
	else for (QString arg : inargs) {
		if (arg[0] == '-') {
			if (arg.length() > 1) {
				switch (arg[1].toLatin1()) {
				case 'R':
					recur = true;
				case 'W':
					QString f = arg.mid(2);
					weight = f.toFloat();
				}
			}
		} else {
			QFileInfo fi {arg};
			if (fi.exists()) args.append({fi, recur, weight});
			recur = false;
			weight = 1.0f;
		}
	}
}
