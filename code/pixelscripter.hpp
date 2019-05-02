#pragma once

#include "common.hpp"
#include "rwmutex.hpp"

#include <atomic>
#include <thread>

#include <QWidget>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextEdit>
#include <QImage>

class CSyntaxHighlighter : public QSyntaxHighlighter {
	Q_OBJECT
public:
	CSyntaxHighlighter(QTextDocument * doc);
protected:
	void highlightBlock(QString const & text) Q_DECL_OVERRIDE;
private:
	struct CHighlightRule {
		QRegExp pattern;
		QTextCharFormat format;
	};
	QVector<CHighlightRule> rules;
};

class PixelScripter : public QWidget {
	Q_OBJECT
signals:
	void compilation_success();
	void process_complete(QImage, QString);
public:
	PixelScripter(QWidget * parent = nullptr);
	virtual ~PixelScripter();
	void set_process(QImage, QString);
	QString internal_err_msg;
	
	void setEditorText(QString text, bool compile);
	QString getEditorText() { return tedit->toPlainText(); }
protected:
	SanePushButton * testpb = nullptr;
	CSyntaxHighlighter * syn = nullptr;
	QTextDocument * doc = nullptr;
	QTextEdit * tedit = nullptr;
	QTextEdit * comp_stat = nullptr;
	QString proc_name;
	QImage proc_image;
	bool proc_new;
	void keyPressEvent(QKeyEvent *) Q_DECL_OVERRIDE;
protected slots:
	void compile_src();
private:
	typedef void(*ps_proc)(uint32_t * *, uint32_t * *, int, int );
	ps_proc ps_proc_sym = nullptr;
	void * ps_state = nullptr;
	rwmutex ps_mut, proc_mut;
	std::thread * chk_thr;
	std::atomic_bool chk_thr_go {true};
	void chk_thr_run();
	QString chk_thr_name;
};
