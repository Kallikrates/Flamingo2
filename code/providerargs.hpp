#ifndef PROVIDERARGS_HPP
#define PROVIDERARGS_HPP

#include "common.hpp"

#include <QList>
#include <QString>
#include <QDir>
#include <QFileInfo>

enum class Recurse {NoRecur, SingleCat, MultiCat};

struct ProviderArg {
	QFileInfo path;
	Recurse recurse;
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
