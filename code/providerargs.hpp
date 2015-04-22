#ifndef PROVIDERARGS_HPP
#define PROVIDERARGS_HPP

#include <QList>
#include <QString>
#include <QDir>
#include <QFileInfo>

struct ProviderArg {
	QFileInfo path;
	bool recurse;
	float weight;
};

class ProviderArgs {
public:
	ProviderArgs(QList<QString>);
	virtual ~ProviderArgs() {}
	QList<ProviderArg> const & getArgs() const {return args;}
	QString const & getReqStart() const {return reqStart;}
protected:
	QList<ProviderArg> args;
	QString reqStart;
};

#endif //PROVIDERARGS_HPP
