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
   for(size_t i = 0; i < length; i++)
   {
       int index = QRandomGenerator::global()->bounded(0, (int)strlen(chars) - 1);
       ret += chars[index];
   }
   return ret;
}

static char const * const template_src = 
R"(//all pixels are in 0xAARRGGBB format

#include <cstdint>
	
struct pixel {
	
	uint8_t a, r, g, b;

	constexpr pixel() : a {0}, r {0}, g {0}, b {0} {}
	constexpr pixel(uint8_t a, uint8_t r, uint8_t g, uint8_t b) : a {a}, r {r}, g {g}, b {b} {}
	constexpr pixel(uint32_t px) :
	a { static_cast<uint8_t>((px & 0xFF000000) >> 24) },
	r { static_cast<uint8_t>((px & 0x00FF0000) >> 16) },
	g { static_cast<uint8_t>((px & 0x0000FF00) >> 8) },
	b { static_cast<uint8_t>(px & 0x000000FF)}
	{}
	
	constexpr uint32_t assemble() {
		return b + (g << 8) + (r << 16) + (a << 24);
	}
};

static inline pixel proc_pixel(pixel const & in) {
	pixel out;
	out.a = in.a;
	out.r = in.r;
	out.g = in.g * 0.6;
	out.b = in.b * 0.8;
	return out;
}

extern "C" void ps_proc ( uint32_t const * const * img_in, uint32_t * * img_out, int width, int height ) {
	for (int line = 0; line < height; line++) for ( int x = 0; x < width; x++ ) {
		img_out[line][x] = proc_pixel(img_in[line][x]).assemble();
	} 
}
)";

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
	
	tedit = new QTextEdit(this);
	tedit->setFont(font);
	tedit->setTabStopDistance(2 * tedit->fontMetrics().horizontalAdvance(' '));
	tedit->setText(template_src);
	
	testpb = new QPushButton("Compile", this);
	connect(testpb, SIGNAL(clicked()), this, SLOT(compile_src()));
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
	proc_image = img_in.convertToFormat(QImage::Format_ARGB32, Qt::AutoColor);
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
		auto ps_proc_sym_temp = reinterpret_cast<ps_proc>(dlsym(ps_handle, "ps_proc"));
		if (!ps_proc_sym_temp) {
			complog += "\nCompilation succeeded, but could not find required symbol \"ps_proc\".";
		} else {
			ps_proc_sym = ps_proc_sym_temp;
			
			complog += "\nCompilation Successful!";

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
			
			if (ps_proc_sym) {
				std::vector<uint32_t *> fDat, tDat;
				fDat.resize(fImg.height());
				tDat.resize(fImg.height());
				for (int i = 0; i < fImg.height(); i++) {
					fDat[i] = reinterpret_cast<uint32_t *>(fImg.scanLine(i));
					tDat[i] = reinterpret_cast<uint32_t *>(tImg.scanLine(i));
				}
				ps_proc_sym(fDat.data(), tDat.data(), fImg.width(), fImg.height());
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
