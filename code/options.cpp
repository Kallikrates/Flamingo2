#include "options.hpp"

void Options::readSettings(const QSettings & settings) {
	this->slideshowInterval = std::chrono::milliseconds(settings.value("slideshowInterval", 5000).toInt());
	this->viewKeep = (ImageView::ZKEEP)settings.value("slideshowZoomKeep", 1).toInt();
	
	this->use_ps = settings.value("usePixelScripts", false).toBool();
	
	this->UUIDAutoSave = settings.value("UUIDAutoSave", false).toBool();
	this->UUIDAutoSaveDir = settings.value("UUIDAutoSaveDir", QDir::currentPath()).toString();
}

void Options::writeSetttings(QSettings & settings) const {
	settings.setValue("slideshowInterval", (int)this->slideshowInterval.count());
	settings.setValue("slideshowZoomKeep", (int)this->viewKeep);
	settings.setValue("usePixelScripts", this->use_ps);
	settings.setValue("UUIDAutoSave", this->UUIDAutoSave);
	settings.setValue("UUIDAutoSaveDir", this->UUIDAutoSaveDir);
}

OptionsWindow::OptionsWindow(Options opt, QWidget *parent) : QWidget (parent), options(opt) {
	overlayout = new QGridLayout(this);
	tabs = new QTabWidget(this);
	overlayout->addWidget(tabs, 0, 0, 1, 3);
	bCancel = new QPushButton("Close", this);
	QObject::connect(bCancel, SIGNAL(clicked()), this, SLOT(close()));
	bApply = new QPushButton("Apply", this);
	QObject::connect(bApply, SIGNAL(clicked()), this, SLOT(internalApply()));
	bOk = new QPushButton("OK", this);
	QObject::connect(bOk, SIGNAL(clicked()), this, SLOT(internalApply()));
	QObject::connect(bOk, SIGNAL(clicked()), this, SLOT(close()));
	overlayout->addWidget(bCancel, 1, 0, 1, 1);
	overlayout->addWidget(bApply, 1, 1, 1, 1);
	overlayout->addWidget(bOk, 1, 2, 1, 1);

	//Slideshow
	slideshowTabWidget = new QWidget(this);
	slideshowTabIndex = tabs->addTab(slideshowTabWidget, "Slideshow");
	QGridLayout * slideshowLayout = new QGridLayout(slideshowTabWidget);
	//slideshowLayout->setMargin(1);
	slideshowLayout->setSpacing(1);

	QWidget * slideshowIntervalWidget = new QWidget {slideshowTabWidget};
	QLabel * slideshowIntervalLabel = new QLabel("Interval: ", slideshowIntervalWidget);
	slideshowIntervalLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	slideshowIntervalSpinbox = new QSpinBox(slideshowIntervalWidget);
	slideshowIntervalSpinbox->setMaximum(std::numeric_limits<int>::max());
	slideshowIntervalSpinbox->setSingleStep(5);
	slideshowIntervalSpinbox->setMinimum(5);
	slideshowIntervalSpinbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	slideshowIntervalWidget->setLayout(new QHBoxLayout{slideshowIntervalWidget});
	slideshowIntervalWidget->layout()->addWidget(slideshowIntervalLabel);
	slideshowIntervalWidget->layout()->addWidget(slideshowIntervalSpinbox);
	slideshowLayout->addWidget(slideshowIntervalWidget);

	QWidget * slideshowZoomWidget = new QWidget {slideshowTabWidget};
	QLabel * slideshowZoomLabel = new QLabel("Initial Zoom: ", slideshowZoomWidget);
	slideshowZoomLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	slideshowZoomCBox = new QComboBox(slideshowZoomWidget);
	slideshowZoomCBox->addItems({"Fit", "Fit (Force)", "Expand To Fill", "1:1"});
	slideshowZoomCBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	slideshowZoomWidget->setLayout(new QHBoxLayout{slideshowZoomWidget});
	slideshowZoomWidget->layout()->addWidget(slideshowZoomLabel);
	slideshowZoomWidget->layout()->addWidget(slideshowZoomCBox);
	slideshowLayout->addWidget(slideshowZoomWidget);
	
	//PixelScript
	psTabWidget = new QWidget(this);
	psTabIndex = tabs->addTab(psTabWidget, "PixelScript");
	QGridLayout * psLayout = new QGridLayout(psTabWidget);
	//psLayout->setMargin(1);
	//psLayout->setSpacing(1);
	
	psCheckbox = new QCheckBox {"Use PixelScripts", psTabWidget};
	psCheckbox->setChecked(opt.use_ps);
	psLayout->addWidget(psCheckbox, 0, 0);
	
	// ViewSave
	vsTabWidget = new QWidget(this);
	vsTabIndex = tabs->addTab(vsTabWidget, "ViewSave");
	QGridLayout * vsLayout = new QGridLayout(vsTabWidget);
	
	vsUUIDAutoSaveCBox = new QCheckBox {"Use UUID Autosaving", vsTabWidget};
	vsUUIDAutoSaveCBox->setChecked(opt.UUIDAutoSave);
	vsLayout->addWidget(vsUUIDAutoSaveCBox, 0, 0);
	
	vsUUIDAutoSaveDirLE = new QLineEdit(vsTabWidget);
	vsUUIDAutoSaveDirLE->setText(opt.UUIDAutoSaveDir);
	vsLayout->addWidget(vsUUIDAutoSaveDirLE, 1, 0);
	
	this->setWindowFlags(Qt::WindowStaysOnTopHint);
}

OptionsWindow::~OptionsWindow() {}

void OptionsWindow::showEvent(QShowEvent * QSE) {
	QSE->accept();
	slideshowIntervalSpinbox->setValue(options.slideshowInterval.count());
	if (options.viewKeep) slideshowZoomCBox->setCurrentIndex((int)options.viewKeep - 1);
	else slideshowZoomCBox->setCurrentIndex(0);
	psCheckbox->setChecked(options.use_ps);
	vsUUIDAutoSaveCBox->setChecked(options.UUIDAutoSave);
	vsUUIDAutoSaveDirLE->setText(options.UUIDAutoSaveDir);
}

void OptionsWindow::keyPressEvent(QKeyEvent * QKE) {
	if (QKE->key() == Qt::Key_Escape) this->close();
}

Options const & OptionsWindow::getOptions() const {
	return options;
}

void OptionsWindow::internalApply() {
	options.slideshowInterval = std::chrono::milliseconds(slideshowIntervalSpinbox->value());
	options.viewKeep = (ImageView::ZKEEP)(slideshowZoomCBox->currentIndex() + 1);
	options.use_ps = psCheckbox->isChecked();
	options.UUIDAutoSave = vsUUIDAutoSaveCBox->isChecked();
	options.UUIDAutoSaveDir = vsUUIDAutoSaveDirLE->text();
	emit applied();
}
