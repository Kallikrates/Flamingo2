#include "providerargs.hpp"

ProviderArgs::ProviderArgs(QList<QString> inargs) {
	inargs.removeFirst();
	Recurse recur = Recurse::NoRecur;
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
			args.append({dir, Recurse::NoRecur, 1.0f});
		}
	} else if (inargs.length() == 0) args.append({QFileInfo(QDir::current().canonicalPath()), Recurse::NoRecur, 1.0f});
	else for (QString arg : inargs) {
		if (arg[0] == '-') {
			if (arg.length() > 1) {
				switch (arg[1].toLatin1()) {
				case 'R':
					recur = Recurse::SingleCat;
					break;
				case 'D':
					recur = Recurse::MultiCat;
					break;
				case 'W':
					QString f = arg.mid(2);
					weight = f.toFloat();
					break;
				}
			}
		} else {
			QFileInfo fi {arg};
			if (fi.exists()) args.append({fi, recur, weight});
			recur = Recurse::NoRecur;
			weight = 1.0f;
		}
	}
}
