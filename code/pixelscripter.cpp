#include "pixelscripter.hpp"

#include <QGridLayout>
#include <QFile>
#include <QDir>

#include <dlfcn.h>

static std::string system_exec(char const * cmd) {
	char buffer [128];
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != nullptr)
            result += buffer;
    }
    return result;
}

static std::string randomString(size_t length) {
	
	char const * chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
   std::string ret;
   for(int i = 0; i < length; i++)
   {
       int index = qrand() % strlen(chars);
       ret += chars[index];
   }
   return ret;
}

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
	QStringList types = {"int", "short", "float", "double", "void", "struct", "long", "unsigned", "char", "uint", "uchar", "ushort"};
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
	
	this->setWindowFlags(Qt::WindowStaysOnTopHint);
}

PixelScripter::~PixelScripter() {
	chk_thr_go.store(false);
	delete testpb;
	if (ps_state) dlclose(ps_state);
}

void PixelScripter::keyPressEvent(QKeyEvent * QKE) {
	if (QKE->key() == Qt::Key_Escape) this->close();
}

void PixelScripter::set_process(QImage img_in, QString name) {
	proc_mut.write_lock();
	proc_image = img_in;
	proc_name = name;
	proc_new = true;
	proc_mut.write_unlock();
}

void PixelScripter::setEditorText(QString text, bool compile) {
	if (text.isEmpty()) {
		tedit->setText(template_src);
	} else {
		tedit->setText(text);
	}
	ps_mut.write_unlock();
	if (compile) this->compile_src();
}

void PixelScripter::compile_src() {
	
	ps_mut.write_lock();
	
	system_exec("mkdir -p ~/.local/share/flamingo2");
	QString source_str = tedit->document()->toPlainText();
	QFile src_file {QDir::homePath() + "/.local/share/flamingo2/ps.cc"};
	src_file.open(QIODevice::WriteOnly | QIODevice::Truncate);
	src_file.write(source_str.toLatin1());
	src_file.close();
	
	std::string randomFileName = randomString(16);
	
	std::string complog = system_exec(("gcc -o ~/.local/share/flamingo2/" + randomFileName + ".so -Wall -Wextra -Wpedantic -Ofast -std=c++17 -march=native -mtune=native -shared ~/.local/share/flamingo2/ps.cc 2>&1").c_str());
	void * ps_handle = dlopen((QDir::homePath() + "/.local/share/flamingo2/" + QString::fromStdString(randomFileName) + ".so").toStdString().c_str(), RTLD_NOW);
	
	system_exec(("rm ~/.local/share/flamingo2/" + randomFileName + ".so").c_str());
	
	if (!ps_handle) {
		complog += "\nCompilation Failed!";
		complog += "\n" + std::string(dlerror());
	} else {
		auto nproc_image_mode = reinterpret_cast<proc_image_mode_func>(dlsym(ps_handle, "image_mode"));
		auto nproc_line_mode = reinterpret_cast<proc_line_mode_func>(dlsym(ps_handle, "line_mode"));
		auto nproc_pixel_mode = reinterpret_cast<proc_pixel_mode_func>(dlsym(ps_handle, "pixel_mode"));
		if (!nproc_image_mode && !nproc_line_mode && !nproc_pixel_mode) {
			complog += "\nCompilation succeeded, but no valid entry point was found.";
		} else {
			proc_image_mode = nproc_image_mode;
			proc_line_mode = nproc_line_mode;
			proc_pixel_mode = nproc_pixel_mode;
			
			std::string using_mode;
			if (proc_image_mode) {
				using_mode = "Image";
			} else if (proc_line_mode) {
				using_mode = "Line";
			} else if (proc_pixel_mode) {
				using_mode = "Pixel";
			}
			
			complog += "\nCompilation Success! Using " + using_mode + " mode.";

			if (ps_state) dlclose(ps_state);
			ps_state = ps_handle;
			emit compilation_success();
		}
	}
	ps_mut.write_unlock();
	comp_stat->setText(QString::fromStdString(complog));
}

void PixelScripter::chk_thr_run() {
	while (chk_thr_go) {
		proc_mut.write_lock();
		if (chk_thr_name != proc_name || proc_new) {
			proc_new = false;
			chk_thr_name = proc_name;
			QImage fImg = proc_image;
			proc_mut.write_unlock();
			QImage tImg = {fImg.size(), QImage::Format_ARGB32};
			
			ps_mut.read_lock();
			
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
			
			ps_mut.read_unlock();
			emit process_complete(tImg, chk_thr_name);
			
		} else {
			proc_mut.write_unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(25));
		}
	}
}
