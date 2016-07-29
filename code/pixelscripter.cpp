#include "pixelscripter.hpp"

#include <QGridLayout>

#include <dlfcn.h>
#include <libtcc.h>

static char const * const template_src =
		"//you should complete only one function below, and you should remove the other ones.\n"
		"//the highest function available will be used (first image_mode, then line_mode, then pixel_mode)\n"
		"//all pixels are in 0xBBGGRRAA format\n\n"
		"//image_mode: complete control over the output image, but no parallelism.\n"
		"void image_mode(unsigned int * * pixels_in, unsigned int * * pixels_out, int width, int height) {\n\n}\n\n"
		"//line_mode: will utilize multithreading.\n"
		"void line_mode(unsigned int * pixels_in, unsigned int * pixels_out, int y, int width, int height) {\n\n}\n\n"
		"//pixel_mode: will utilize multithreading, more straightforward than line_mode, but more overhead.\n"
		"unsigned int pixel_mode(unsigned int pixel_in, int x, int y, int width, int height) {\n\n}\n\n";

CSyntaxHighlighter::CSyntaxHighlighter::CSyntaxHighlighter(QTextDocument* doc) : QSyntaxHighlighter(doc) {
	CHighlightRule rule {};
	QStringList types = {"int", "short", "float", "double", "void", "struct", "long", "unsigned"};
	QStringList logic = {"if", "for", "while", "do"};
	
	rule.format.setFontWeight(QFont::Bold);
	
	foreach(QString type, types) {
		rule.pattern = QRegExp("\\b" + type + "\\b");
		rule.format.setForeground(QColor(127, 0, 0));
		
		rules.append(rule);
	}
	
	foreach(QString log, logic) {
		rule.pattern = QRegExp("\\b" + log + "\\b");
		rule.format.setForeground(QColor(255, 0, 0));
		
		rules.append(rule);
	}
	
	rule.pattern = QRegExp("\\*");
	rule.format.setForeground(QColor(127, 0, 0));
	rules.append(rule);
	
	rule.format.setFontWeight(QFont::Normal);
	

	rule.pattern = QRegExp("\".*\"");
	rule.format.setForeground(QColor(255, 192, 192));
	rules.append(rule);
	
	rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
	rule.format.setForeground(QColor(255, 127, 127));
	rules.append(rule);
}


void CSyntaxHighlighter::highlightBlock(QString const & text) {
	QTextCharFormat format;
	format.setForeground(QColor(255, 220, 220));
	setFormat(0, text.length(), format);
    foreach (const CHighlightRule &rule, rules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
}

static void tcc_err(void * opaque, char const * msg) {
	PixelScripter * ps = reinterpret_cast<PixelScripter *>(opaque);
	ps->internal_err_msg = msg;
}

PixelScripter::PixelScripter(QWidget * parent) : QWidget(parent) {
	
	QFont font;
	font.setStyleHint(QFont::Monospace);
	font.setFamily("DejaVu Sans Mono");
	font.setFixedPitch(true);
	font.setPointSize(8);
	QFontMetrics metrics(font);
	
	tedit = new QTextEdit(this);
	tedit->setFont(font);
	tedit->setTabStopWidth(2 * metrics.width(' '));
	tedit->setText(template_src);
	
	testpb = new SanePushButton("Compile", this);
	connect(testpb, SIGNAL(used()), this, SLOT(compile_src()));
	comp_stat = new QTextEdit(this);
	comp_stat->setReadOnly(true);
	comp_stat->setMaximumHeight(100);
	
	QGridLayout * layout = new QGridLayout(this);
	this->setLayout(layout);
	layout->addWidget(tedit, 0, 0);
	layout->addWidget(comp_stat, 1, 0);
	layout->addWidget(testpb, 2, 0);
	
	syn = new CSyntaxHighlighter(tedit->document());
	
	tedit->setStyleSheet("background-color: #200000;");
	
	this->setMinimumSize(400, 300);
	
	chk_thr = new std::thread(&PixelScripter::chk_thr_run, this);
}

PixelScripter::~PixelScripter() {
	chk_thr_go.store(false);
	delete testpb;
	if (tcc_state) tcc_delete(reinterpret_cast<TCCState *>(tcc_state));
}

void PixelScripter::keyPressEvent(QKeyEvent * QKE) {
	if (QKE->key() == Qt::Key_Escape) this->close();
}

void PixelScripter::set_process(QImage img_in, QString name) {
	proc_mut.write_lock();
	proc_image = img_in;
	proc_name = name;
	proc_mut.write_unlock();
}

void PixelScripter::setEditorText(QString text, bool compile) {
	if (text.isEmpty()) {
		tedit->setText(template_src);
	} else {
		tedit->setText(text);
		this->compile_src();
	}
}

void PixelScripter::compile_src() {
	
	tcc_mut.write_lock();
	
	QByteArray q_src = tedit->document()->toPlainText().toLatin1();
	char const * src = q_src.data();
	
	TCCState * new_state = tcc_new();
	tcc_set_error_func(new_state, this, tcc_err);
	
	if (tcc_compile_string(new_state, src) != -1) {
		tcc_relocate(new_state, TCC_RELOCATE_AUTO);
		auto nproc_image_mode = reinterpret_cast<proc_image_mode_func>(tcc_get_symbol(new_state, "image_mode"));
		auto nproc_line_mode = reinterpret_cast<proc_line_mode_func>(tcc_get_symbol(new_state, "line_mode"));
		auto nproc_pixel_mode = reinterpret_cast<proc_pixel_mode_func>(tcc_get_symbol(new_state, "pixel_mode"));
		if (!nproc_image_mode && !nproc_line_mode && !nproc_pixel_mode) {
			comp_stat->setText("Compilation succeeded, but no valid entry point was found... did you read the default text?");
			tcc_delete(new_state);
		} else {
			comp_stat->setText("Compilation Success!");
			proc_image_mode = nproc_image_mode;
			proc_line_mode = nproc_line_mode;
			proc_pixel_mode = nproc_pixel_mode;
			if (tcc_state) tcc_delete(reinterpret_cast<TCCState *>(tcc_state));
			tcc_state = new_state;
			emit compilation_success();
		}
	} else {
		comp_stat->setText("Compilation FAILED!\nTCC Error Log:\n" + internal_err_msg);
		tcc_delete(new_state);
	}
	
	tcc_mut.write_unlock();
}

void PixelScripter::chk_thr_run() {
	while (chk_thr_go) {
		proc_mut.read_lock();
		if (chk_thr_name != proc_name) {
			chk_thr_name = proc_name;
			QImage fImg = proc_image;
			proc_mut.read_unlock();
			QImage tImg = {fImg.size(), QImage::Format_ARGB32};
			
			tcc_mut.read_lock();
			
			if (proc_image_mode) {
				unsigned int * * fDat, * * tDat;
				fDat = new unsigned int * [fImg.height()];
				tDat = new unsigned int * [fImg.height()];
				for (int i = 0; i < fImg.height(); i++) {
					fDat[i] = reinterpret_cast<unsigned int *>(fImg.scanLine(i));
					tDat[i] = reinterpret_cast<unsigned int *>(tImg.scanLine(i));
				}
				proc_image_mode(fDat, tDat, fImg.width(), fImg.height());
			} else if (proc_line_mode) {
				for (int i = 0; i < fImg.height(); i++) {
					proc_line_mode(reinterpret_cast<unsigned int *>(fImg.scanLine(i)), reinterpret_cast<unsigned int *>(tImg.scanLine(i)), i, fImg.width(), fImg.height());
				}
			} else if (proc_pixel_mode) {
				for (int y = 0; y < fImg.height(); y++) {
					for (int x = 0; x < fImg.width(); x++) {
						tImg.setPixel(x, y, proc_pixel_mode(fImg.pixel(x, y), x, y, fImg.width(), fImg.height()));
					}
				}
			} else {
				tImg = fImg;
			}
			
			tcc_mut.read_unlock();
			emit process_complete(tImg, chk_thr_name);
			
		} else {
			proc_mut.read_unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(25));
		}
	}
}
