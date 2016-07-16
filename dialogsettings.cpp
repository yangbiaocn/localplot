#include "dialogsettings.h"
#include "ui_dialogsettings.h"

DialogSettings::DialogSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSettings)
{
    ui->setupUi(this);

    // Instantiate settings object
    init_localplot_settings();
    settings = new QSettings();

    // Set up the drawing pens
    upPen.setStyle(Qt::DotLine);
    do_updatePens();

    // Initialize interface
//    ui->comboBox_baud->insertItems(0, QStringList() << "2400" << "4800"
//                                   << "9600" << "19200" << "38400"
//                                   << "57600" << "115200");
    ui->comboBox_baud->clear();
    ui->comboBox_baud->insertItem(0, "2400", 2400);
    ui->comboBox_baud->insertItem(1, "4800", 4800);
    ui->comboBox_baud->insertItem(2, "9600", 9600);
    ui->comboBox_baud->insertItem(3, "19200", 19200);
    ui->comboBox_baud->insertItem(4, "38400", 38400);
    ui->comboBox_baud->insertItem(5, "57600", 57600);
    ui->comboBox_baud->insertItem(6, "115200", 115200);
    ui->comboBox_baud->setCurrentIndex(2);

//    ui->comboBox_bytesize->insertItems(0, QStringList() << "8" << "7"
//                                       << "6" << "5");
    ui->comboBox_bytesize->clear();
    ui->comboBox_bytesize->insertItem(0, "8", 8);
    ui->comboBox_bytesize->insertItem(1, "7", 7);
    ui->comboBox_bytesize->insertItem(2, "6", 6);
    ui->comboBox_bytesize->insertItem(3, "5", 5);
    ui->comboBox_bytesize->setCurrentIndex(0);

//    ui->comboBox_parity->insertItems(0, QStringList() << "None" << "Odd"
//                                     << "Even" << "Mark" << "Space");
    ui->comboBox_parity->clear();
    ui->comboBox_parity->insertItem(0, "None", "none");
    ui->comboBox_parity->insertItem(1, "Odd", "odd");
    ui->comboBox_parity->insertItem(2, "Even", "even");
    ui->comboBox_parity->insertItem(3, "Mark", "mark");
    ui->comboBox_parity->insertItem(4, "Space", "space");
    ui->comboBox_parity->setCurrentIndex(0);

//    ui->comboBox_stopbits->insertItems(0, QStringList() << "1" << "1.5" << "2");
    ui->comboBox_stopbits->clear();
    ui->comboBox_stopbits->insertItem(0, "1", 1);
    ui->comboBox_stopbits->insertItem(1, "1.5", 3);
    ui->comboBox_stopbits->insertItem(2, "2", 2);
    ui->comboBox_stopbits->setCurrentIndex(0);

    do_refreshSerialList();

    // Load saved settings
    ui->spinBox_downPen_size->setValue(settings->value("pen/down/size", SETDEF_PEN_DOWN_SIZE).toInt());
    ui->spinBox_downPen_red->setValue(settings->value("pen/down/red", SETDEF_PEN_DOWN_RED).toInt());
    ui->spinBox_downPen_green->setValue(settings->value("pen/down/green", SETDEF_PEN_DOWN_GREEN).toInt());
    ui->spinBox_downPen_blue->setValue(settings->value("pen/down/blue", SETDEF_PEN_DOWN_BLUE).toInt());
    ui->spinBox_upPen_size->setValue(settings->value("pen/up/size", SETDEF_PEN_UP_SIZE).toInt());
    ui->spinBox_upPen_red->setValue(settings->value("pen/up/red", SETDEF_PEN_UP_RED).toInt());
    ui->spinBox_upPen_green->setValue(settings->value("pen/up/green", SETDEF_PEN_UP_GREEN).toInt());
    ui->spinBox_upPen_blue->setValue(settings->value("pen/up/blue", SETDEF_PEN_DOWN_BLUE).toInt());

    for (int i = 0; i < ui->comboBox_serialPort->count(); i++)
    {
        if (settings->value("serial/port") == ui->comboBox_serialPort->itemData(i))
        {
            ui->comboBox_serialPort->setCurrentIndex(i);
            break;
        }
    }

    connect(ui->pushButton_serialRefresh, SIGNAL(clicked()), this, SLOT(do_refreshSerialList()));

    // Update settings on UI change
    connect(ui->spinBox_downPen_size, SIGNAL(valueChanged(int)), this, SLOT(do_drawDemoView()));
    connect(ui->spinBox_downPen_red, SIGNAL(valueChanged(int)), this, SLOT(do_drawDemoView()));
    connect(ui->spinBox_downPen_green, SIGNAL(valueChanged(int)), this, SLOT(do_drawDemoView()));
    connect(ui->spinBox_downPen_blue, SIGNAL(valueChanged(int)), this, SLOT(do_drawDemoView()));
    connect(ui->spinBox_upPen_size, SIGNAL(valueChanged(int)), this, SLOT(do_drawDemoView()));
    connect(ui->spinBox_upPen_red, SIGNAL(valueChanged(int)), this, SLOT(do_drawDemoView()));
    connect(ui->spinBox_upPen_green, SIGNAL(valueChanged(int)), this, SLOT(do_drawDemoView()));
    connect(ui->spinBox_upPen_blue, SIGNAL(valueChanged(int)), this, SLOT(do_drawDemoView()));

    connect(ui->checkBox_cutterSpeed, SIGNAL(stateChanged(int)), this, SLOT(do_updateUi()));
    connect(ui->spinBox_cutterSpeed, SIGNAL(valueChanged(int)), this, SLOT(do_updateUi()));

    ui->graphicsView_penDownDemo->setScene(&penDownDemoScene);
    ui->graphicsView_penUpDemo->setScene(&penUpDemoScene);

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(do_saveAndClose())); // Save settings
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(close())); // Discard settings

    do_drawDemoView();
}

DialogSettings::~DialogSettings()
{
    delete settings;
    delete ui;
}

void DialogSettings::do_saveAndClose()
{
    settings->beginGroup("pen");
    {
        settings->setValue("down/size", ui->spinBox_downPen_size->value());
        settings->setValue("down/red", ui->spinBox_downPen_red->value());
        settings->setValue("down/green", ui->spinBox_downPen_green->value());
        settings->setValue("down/blue", ui->spinBox_downPen_blue->value());
        settings->setValue("up/size", ui->spinBox_upPen_size->value());
        settings->setValue("up/red", ui->spinBox_upPen_red->value());
        settings->setValue("up/green", ui->spinBox_upPen_green->value());
        settings->setValue("up/blue", ui->spinBox_upPen_blue->value());
    }
    settings->endGroup();

    settings->beginGroup("serial");
    {
        settings->setValue("port", ui->comboBox_serialPort->currentData());
        settings->setValue("parity", ui->comboBox_parity->currentData());
        settings->setValue("baud", ui->comboBox_baud->currentData());
        settings->setValue("bytesize", ui->comboBox_bytesize->currentData());
        settings->setValue("stopbits", ui->comboBox_stopbits->currentData());
        settings->setValue("xonxoff", ui->radioButton_XonXoff->isChecked());
        settings->setValue("rtscts", ui->radioButton_RtsCts->isChecked());
        settings->setValue("dsrdtr", ui->radioButton_DsrDtr->isChecked());
    }
    settings->endGroup();

    settings->beginGroup("cutter");
    {
        settings->setValue("incremental", ui->checkBox_cutterSpeed->isChecked());
        settings->setValue("speed", ui->spinBox_cutterSpeed->value());
        settings->setValue("speed/axis", ui->radioButton_cutterAxisSpeed->isChecked());
    }
    settings->endGroup();

    close();
}

void DialogSettings::do_updateUi()
{
    // Enable/disable cutter settings
    ui->spinBox_cutterSpeed->setEnabled(ui->checkBox_cutterSpeed->isChecked());
    ui->radioButton_cutterAxisSpeed->setEnabled(ui->checkBox_cutterSpeed->isChecked());
    ui->radioButton_cutterSpeedAbsolute->setEnabled(ui->checkBox_cutterSpeed->isChecked());
}

void DialogSettings::do_refreshSerialList()
{
    QList<QSerialPortInfo> _ports = serialPorts.availablePorts();
    ui->comboBox_serialPort->clear();

    for (int i = 0; i < _ports.count(); ++i)
    {
        ui->comboBox_serialPort->insertItem(i, _ports.at(i).portName(), _ports.at(i).systemLocation());
    }
}

void DialogSettings::do_updatePens()
{
    // Variables
    int rgbColor[3];
    int penSize;
    QColor penColor;

    // Set downPen
    penSize = ui->spinBox_downPen_size->value();
    rgbColor[0] = ui->spinBox_downPen_red->value();
    rgbColor[1] = ui->spinBox_downPen_green->value();
    rgbColor[2] = ui->spinBox_downPen_blue->value();
    penColor = QColor(rgbColor[0], rgbColor[1], rgbColor[2]);
    downPen.setColor(penColor);
    downPen.setWidth(penSize);

    // Set upPen
    penSize = ui->spinBox_upPen_size->value();
    rgbColor[0] = ui->spinBox_upPen_red->value();
    rgbColor[1] = ui->spinBox_upPen_green->value();
    rgbColor[2] = ui->spinBox_upPen_blue->value();
    penColor = QColor(rgbColor[0], rgbColor[1], rgbColor[2]);
    upPen.setColor(penColor);
    upPen.setWidth(penSize);
}

void DialogSettings::do_drawDemoView()
{
    do_updatePens();
    penDownDemoScene.clear();
    penUpDemoScene.clear();
    penDownDemoScene.addLine(0, 0, 28, 0, downPen);
    penUpDemoScene.addLine(0, 0, 28, 0, upPen);
    ui->graphicsView_penDownDemo->show();
    ui->graphicsView_penUpDemo->show();
}