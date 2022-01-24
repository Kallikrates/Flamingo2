#include "providerargs.hpp"

ProviderArgs::ProviderArgs(QList<QString> inargs) {
	inargs.removeFirst();
	Recurse recur = Recurse::NoRecur;
	float weight = 1.0f;
	int depth = -1;
	if (inargs.length() == 1) {
		QFileInfo fi {inargs[0]};
		if (fi.exists()) {
			QFileInfo dir;
			if (fi.isFile()) {
				dir = QFileInfo {fi.canonicalPath()};
				reqStart = fi.canonicalFilePath();
			}
			else dir = QFileInfo {fi.canonicalFilePath()};
			args.append({dir, Recurse::NoRecur, 1.0f});
		}
	} else if (inargs.length() == 0) args.append({QFileInfo(QDir::current().canonicalPath()), Recurse::NoRecur, 1.0f});
	else for (QString arg : inargs) {
		if (arg[0] == '-') {
			if (arg.length() > 1) {
				switch (arg[1].toLatin1()) {
				case 'S':
					recur = Recurse::NoRecur;
					break;
				case 'R':
					recur = Recurse::SingleCat;
					break;
				case 'M':
					recur = Recurse::MultiCat;
					break;
				case 'w': {
					QString f = arg.mid(2);
					weight = f.toFloat();
					if (!weight) weight = 1;
					break; }
				case 'd': {
					QString f = arg.mid(2);
					depth = f.toUInt();
					break; }
				}
			}
		} else {
			QFileInfo fi {arg};
			if (fi.exists()) args.append({fi, recur, weight, depth});
		}
	}
}
