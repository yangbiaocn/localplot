#include "mainwindow.h"
#include "ui_mainwindow.h"

/**
 * Reference: http://cstep.luberth.com/HPGL.pdf
 *
 * Language Structure:
 * Using Inkscape's default format -> XXy1,y1,y2,y2;
 * Two uppercase characters followed by a CSV list and terminated with a semicolon.
 *
 * Path Vertex Object:
 * State: up or down
 * Vertex coordinate: in graphic units (1/1016") (0.025mm)
 *
 * Program file structure
 * hpgl_cmd - Structure for storing a single hpgl command
 * hpgl_obj - An hpgl object, or cluster of commands that share similar
 *             properties and transformations.
 */

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Instantiate settings object
    QCoreApplication::setOrganizationName("Makers Local 256");
    QCoreApplication::setOrganizationDomain("makerslocal.org");
    QCoreApplication::setApplicationName("localplot");
    settings = new QSettings();

    // Initialize interface
    ui->comboBox_baud->insertItems(0, QStringList() << "2400" << "4800" << "9600" << "19200" << "38400" << "57600" << "115200");
    ui->comboBox_baud->setCurrentIndex(2);
    //ui->comboBox_bytesize->insertItems(0, QStringList() << "8" << "7" << "6" << "5");
    ui->comboBox_bytesize->addItem("8", 8);
    ui->comboBox_bytesize->addItem("7", 7);
    ui->comboBox_bytesize->addItem("6", 6);
    ui->comboBox_bytesize->addItem("5", 5);
    ui->comboBox_parity->insertItems(0, QStringList() << "None" << "Odd" << "Even" << "Mark" << "Space");
    ui->comboBox_stopbits->insertItems(0, QStringList() << "1" << "1.5" << "2");
    do_refreshSerialList();

    // Load saved settings
    ui->lineEdit_filePath->setText(settings->value("mainwindow/filePath", "").toString());
    ui->spinBox_downPen_size->setValue(settings->value("pen/down/size", 2).toInt());
    ui->spinBox_downPen_red->setValue(settings->value("pen/down/red", 100).toInt());
    ui->spinBox_downPen_green->setValue(settings->value("pen/down/green", 150).toInt());
    ui->spinBox_downPen_blue->setValue(settings->value("pen/down/blue", 200).toInt());
    ui->spinBox_upPen_size->setValue(settings->value("pen/up/size", 1).toInt());
    ui->spinBox_upPen_red->setValue(settings->value("pen/up/red", 250).toInt());
    ui->spinBox_upPen_green->setValue(settings->value("pen/up/green", 150).toInt());
    ui->spinBox_upPen_blue->setValue(settings->value("pen/up/blue", 150).toInt());

    for (int i = 0; i < ui->comboBox_serialPort->count(); i++)
    {
        if (settings->value("serial/port") == ui->comboBox_serialPort->itemData(i))
        {
            ui->comboBox_serialPort->setCurrentIndex(i);
            break;
        }
    }

//    settings->setValue("port", ui->comboBox_serialPort->currentData());
//    settings->setValue("parity", ui->comboBox_parity->currentData());
//    settings->setValue("baud", ui->comboBox_baud->currentData());
//    settings->setValue("bytesize", ui->comboBox_bytesize->currentData());
//    settings->setValue("stopbits", ui->comboBox_stopbits->currentData());
//    settings->setValue("xonxoff", ui->radioButton_XonXoff->isChecked());
//    settings->setValue("ischecked", ui->radioButton_RtsCts->isChecked());
//    settings->setValue("dsrdtr", ui->radioButton_DsrDtr->isChecked());

    // Connect actions
    connect(ui->pushButton_serialRefresh, SIGNAL(clicked()), this, SLOT(do_refreshSerialList()));
    connect(ui->pushButton_serialConnect, SIGNAL(clicked()), this, SLOT(handle_serialConnectBtn()));
    connect(ui->pushButton_fileSelect, SIGNAL(clicked()), this, SLOT(handle_selectFileBtn()));
    connect(ui->pushButton_fileLoad, SIGNAL(clicked()), this, SLOT(do_loadFile()));
    connect(ui->pushButton_doPlot, SIGNAL(clicked()), this, SLOT(do_plot()));

    // Set up the drawing pens
    upPen.setStyle(Qt::DotLine);
    do_updatePens();

    // Update settings on UI change
    connect(ui->spinBox_downPen_size, SIGNAL(valueChanged(int)), this, SLOT(update_penDown()));
    connect(ui->spinBox_downPen_red, SIGNAL(valueChanged(int)), this, SLOT(update_penDown()));
    connect(ui->spinBox_downPen_green, SIGNAL(valueChanged(int)), this, SLOT(update_penDown()));
    connect(ui->spinBox_downPen_blue, SIGNAL(valueChanged(int)), this, SLOT(update_penDown()));
    connect(ui->spinBox_upPen_size, SIGNAL(valueChanged(int)), this, SLOT(update_penUp()));
    connect(ui->spinBox_upPen_red, SIGNAL(valueChanged(int)), this, SLOT(update_penUp()));
    connect(ui->spinBox_upPen_green, SIGNAL(valueChanged(int)), this, SLOT(update_penUp()));
    connect(ui->spinBox_upPen_blue, SIGNAL(valueChanged(int)), this, SLOT(update_penUp()));
    connect(ui->lineEdit_filePath, SIGNAL(editingFinished()), this, SLOT(update_filePath()));
    connect(ui->comboBox_serialPort, SIGNAL(currentIndexChanged(int)), this, SLOT(update_serialDevice()));
    connect(ui->checkBox_cutterSpeed, SIGNAL(toggled(bool)), this, SLOT(update_cutterSpeed(bool)));

    connect(QGuiApplication::primaryScreen(), SIGNAL(physicalSizeChanged(QSizeF)), this, SLOT(do_drawView())); // Update view if the pixel DPI changes


    connect(ui->doubleSpinBox_objScale, SIGNAL(valueChanged(double)), this, SLOT(handle_objectTransform())); // Update view if the scale changes
    connect(ui->doubleSpinBox_objRotation, SIGNAL(valueChanged(double)), this, SLOT(handle_objectTransform())); // Update view if the scale changes
    connect(ui->spinBox_objTranslationX, SIGNAL(valueChanged(int)), this, SLOT(handle_objectTransform())); // Update view if the scale changes
    connect(ui->spinBox_objTranslationY, SIGNAL(valueChanged(int)), this, SLOT(handle_objectTransform())); // Update view if the scale changes

    ui->graphicsView_view->setScene(&plotScene);
    ui->graphicsView_penDownDemo->setScene(&penDownDemoScene);
    ui->graphicsView_penUpDemo->setScene(&penUpDemoScene);

    do_drawView();
    do_drawDemoView();
}

MainWindow::~MainWindow()
{
    do_closeSerial();

    delete settings;
    delete ui;
}

QString MainWindow::timeStamp()
{
    return(QTime::currentTime().toString("[HH:mm:ss:zzz] "));
}

void MainWindow::update_penDown()
{
    settings->beginGroup("pen/down");
    settings->setValue("size", ui->spinBox_downPen_size->value());
    settings->setValue("red", ui->spinBox_downPen_red->value());
    settings->setValue("green", ui->spinBox_downPen_green->value());
    settings->setValue("blue", ui->spinBox_downPen_blue->value());
    settings->endGroup();
}

void MainWindow::update_penUp()
{
    settings->beginGroup("pen/up");
    settings->setValue("size", ui->spinBox_upPen_size->value());
    settings->setValue("red", ui->spinBox_upPen_red->value());
    settings->setValue("green", ui->spinBox_upPen_green->value());
    settings->setValue("blue", ui->spinBox_upPen_blue->value());
    settings->endGroup();
    if (ui->tabWidget->currentIndex() == 0)
    {
        do_drawDemoView();
    }
    else if (ui->tabWidget->currentIndex() == 1)
    {
        do_drawView();
    }
}

void MainWindow::update_filePath()
{
    settings->beginGroup("mainwindow");
    settings->setValue("filePath", ui->lineEdit_filePath->text());
    settings->endGroup();
}

void MainWindow::update_serialDevice()
{
    settings->beginGroup("serial");
    settings->setValue("port", ui->comboBox_serialPort->currentData());
    settings->setValue("parity", ui->comboBox_parity->currentData());
    settings->setValue("baud", ui->comboBox_baud->currentData());
    settings->setValue("bytesize", ui->comboBox_bytesize->currentData());
    settings->setValue("stopbits", ui->comboBox_stopbits->currentData());
    settings->setValue("xonxoff", ui->radioButton_XonXoff->isChecked());
    settings->setValue("ischecked", ui->radioButton_RtsCts->isChecked());
    settings->setValue("dsrdtr", ui->radioButton_DsrDtr->isChecked());
    settings->endGroup();
}

void MainWindow::do_refreshSerialList()
{
    QList<QSerialPortInfo> _ports = serialPorts.availablePorts();
    int _index = _ports.count();
    ui->comboBox_serialPort->clear();
    for (int i = 0; i < _index; ++i)
    {
        ui->comboBox_serialPort->insertItem(i, _ports.at(i).portName());
        ui->textBrowser_console->append(_ports.at(i).portName());
    }
    ui->textBrowser_console->append(timeStamp() + "Serial list refreshed \\o/");
}

void MainWindow::do_openSerial()
{
    int _portIndex = ui->comboBox_serialPort->currentIndex();
    QSerialPortInfo _device;

    if (_portIndex >= 0 && serialPorts.availablePorts().count() > _portIndex)
        _device = serialPorts.availablePorts().at(_portIndex);
    else
    {
        do_refreshSerialList();
        do_closeSerial();
        return;
    }
    if (!serialBuffer.isNull())
    {
        serialBuffer.clear();
        handle_serialClosed();
    }
    serialBuffer = new QSerialPort(_device);
    serialBuffer->setBaudRate(atoi(ui->comboBox_baud->currentText().toStdString().c_str()));

    int dataBits = ui->comboBox_bytesize->currentData().toInt();
    if (dataBits == 8)
    {
        serialBuffer->setDataBits(QSerialPort::Data8);
    }
    else if (dataBits == 7)
    {
        serialBuffer->setDataBits(QSerialPort::Data7);
    }
    else if (dataBits == 6)
    {
        serialBuffer->setDataBits(QSerialPort::Data6);
    }
    else if (dataBits == 5)
    {
        serialBuffer->setDataBits(QSerialPort::Data5);
    }

    QString parity = ui->comboBox_parity->currentText();
    if (parity == "None")
    {
        serialBuffer->setParity(QSerialPort::NoParity);
    }
    else if (parity == "Odd")
    {
        serialBuffer->setParity(QSerialPort::OddParity);
    }
    else if (parity == "Even")
    {
        serialBuffer->setParity(QSerialPort::EvenParity);
    }
    else if (parity == "Mark")
    {
        serialBuffer->setParity(QSerialPort::MarkParity);
    }
    else if (parity == "Space")
    {
        serialBuffer->setParity(QSerialPort::SpaceParity);
    }

    QString stopBits = ui->comboBox_stopbits->currentText();
    if (stopBits == "1")
    {
        serialBuffer->setStopBits(QSerialPort::OneStop);
    }
    else if (stopBits == "1.5")
    {
        serialBuffer->setStopBits(QSerialPort::OneAndHalfStop);
    }
    else if (stopBits == "2")
    {
        serialBuffer->setStopBits(QSerialPort::TwoStop);
    }

    if (ui->radioButton_XonXoff->isChecked())
    {
        serialBuffer->setFlowControl(QSerialPort::SoftwareControl);
    }
    else if (ui->radioButton_RtsCts->isChecked())
    {
        serialBuffer->setFlowControl(QSerialPort::HardwareControl);
    }
    else
    {
        serialBuffer->setFlowControl(QSerialPort::NoFlowControl);
    }

    serialBuffer->open(QIODevice::WriteOnly);
    if (serialBuffer->isOpen())
    {
        handle_serialOpened();
    }
    else
    {
        ui->textBrowser_console->append(timeStamp() + "Serial port didn't open? :'(");
        do_closeSerial();
    }

}

void MainWindow::do_closeSerial()
{
    if (!serialBuffer.isNull())
    {
        serialBuffer->close();
        serialBuffer.clear();
    }
    handle_serialClosed();
}

void MainWindow::update_cutterSpeed(bool checked)
{
    ui->spinBox_cutterSpeed->setEnabled(checked);
}

void MainWindow::handle_serialConnectBtn()
{
    if (ui->pushButton_serialConnect->text() == "Connect")
    {
        do_openSerial();
    }
    else
    {
        do_closeSerial();
    }
}

void MainWindow::handle_serialOpened()
{
    ui->textBrowser_console->append(timeStamp() + "Serial port opened x)");
    ui->pushButton_serialConnect->setText("Disconnect");
    ui->comboBox_serialPort->setEnabled(false);
    ui->pushButton_serialRefresh->setEnabled(false);
    ui->comboBox_baud->setEnabled(false);
    ui->comboBox_bytesize->setEnabled(false);
    ui->comboBox_parity->setEnabled(false);
    ui->comboBox_stopbits->setEnabled(false);
    ui->radioButton_DsrDtr->setEnabled(false);
    ui->radioButton_RtsCts->setEnabled(false);
    ui->radioButton_XonXoff->setEnabled(false);
}

void MainWindow::handle_serialClosed()
{
    ui->textBrowser_console->append(timeStamp() + "Serial port closed :D");
    ui->pushButton_serialConnect->setText("Connect");
    ui->comboBox_serialPort->setEnabled(true);
    ui->pushButton_serialRefresh->setEnabled(true);
    ui->comboBox_baud->setEnabled(true);
    ui->comboBox_bytesize->setEnabled(true);
    ui->comboBox_parity->setEnabled(true);
    ui->comboBox_stopbits->setEnabled(true);
    ui->radioButton_DsrDtr->setEnabled(true);
    ui->radioButton_RtsCts->setEnabled(true);
    ui->radioButton_XonXoff->setEnabled(true);
}

void MainWindow::handle_selectFileBtn()
{
    QString fileName;

    fileName = QFileDialog::getOpenFileName(this,
        tr("Open File"), "", tr("HPGL Files (*.hpgl *.HPGL)"));

    ui->lineEdit_filePath->setText(fileName);
}

//void MainWindow::handle_autoTranslateBtn()
//{
//    for (int i = 0; i < objList.count(); i++)
//    {
//        if (objList[i].minX() < 0)
//        {
//            int val = ui->spinBox_objTranslationX
//        }
//    }
//}

int MainWindow::get_nextInt(QString input, int * index)
{
    QChar tmp = input[*index];
    QString buffer = "";

    while (tmp != ',' && tmp != ';')
    {
        buffer.append(tmp);
        tmp = input[++*index];
    }
    return(atoi(buffer.toStdString().c_str()));
}

void MainWindow::do_updatePens()
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

void MainWindow::do_drawDemoView()
{
    do_updatePens();
    penDownDemoScene.clear();
    penUpDemoScene.clear();
    penDownDemoScene.addLine(0, 0, 28, 0, downPen);
    penUpDemoScene.addLine(0, 0, 28, 0, upPen);
    ui->graphicsView_penDownDemo->show();
    ui->graphicsView_penUpDemo->show();
}

void MainWindow::do_drawView()
{
    // Set up new graphics view.
    plotScene.clear();

    // physicalDpi is the number of pixels in an inch
    int xDpi = ui->graphicsView_view->physicalDpiX();
    int yDpi = ui->graphicsView_view->physicalDpiY();

    // Draw origin
    QPen originPen;
    originPen.setColor(QColor(150, 150, 150));
    originPen.setWidth(2);
    plotScene.addLine(0, 0, xDpi, 0, originPen);
    plotScene.addLine(0, 0, 0, -yDpi, originPen);

    do_updatePens();

    // scale is the value set by our user
    //double scale = ui->doubleSpinBox_objScale->value();
    double scale = 1.0;
    // Factor is the conversion from HP Graphic Unit to pixels
    double xFactor = (xDpi / 1016.0 * scale);
    double yFactor = (yDpi / 1016.0 * scale);

    QList<QLine> lines_down;
    lines_down.clear();
    QList<QLine> lines_up;
    lines_up.clear();

    for (int i = 0; i < objList.length(); i++)
    {
        // Get a list of qlines
        objList[i].gen_line_lists();
        lines_down = objList[i].lineListDown;
        lines_up = objList[i].lineListUp;

        // Transform qlines to be upright
        for (int i = 0; i < lines_down.length(); i++)
        {
            int x, y;
            x = lines_down[i].x1();
            y = lines_down[i].y1();
            x = x*xFactor;
            y = y*(-1)*yFactor;
            lines_down[i].setP1(QPoint(x, y));
            x = lines_down[i].x2();
            x = x*xFactor;
            y = lines_down[i].y2();
            y = y*(-1)*yFactor;
            lines_down[i].setP2(QPoint(x, y));
        }

        for (int i = 0; i < lines_up.length(); i++)
        {
            int x, y;
            x = lines_up[i].x1();
            x = x*xFactor;
            y = lines_up[i].y1();
            y = y*(-1)*yFactor;
            lines_up[i].setP1(QPoint(x, y));
            x = lines_up[i].x2();
            x = x*xFactor;
            y = lines_up[i].y2();
            y = y*(-1)*yFactor;
            lines_up[i].setP2(QPoint(x, y));
        }

        // Write qlines to the scene
        for (int i = 0; i < objList.length(); i++)
        {
            for (int l = 0; l < lines_down.length(); l++)
            {
                plotScene.addLine(lines_down[l], downPen);
            }
            for (int l = 0; l < lines_up.length(); l++)
            {
                plotScene.addLine(lines_up[l], upPen);
            }
        }
    }

    // Draw origin text
    QGraphicsTextItem * label = plotScene.addText("Front of Plotter");
    label->setRotation(90);
    QRectF labelRect = label->boundingRect();
    label->setY(label->y() - labelRect.width());
    plotScene.addText("(0,0)");
    QString scaleText = "Scale: " + QString::number(scale);
    QGraphicsTextItem * scaleTextItem = plotScene.addText(scaleText);
    QRectF scaleTextItemRect = scaleTextItem->boundingRect();
    scaleTextItem->setY(scaleTextItem->y() + scaleTextItemRect.height());

    // Set scene rectangle to match new items
    plotScene.setSceneRect(plotScene.itemsBoundingRect());
    //plotScene.addRect(plotScene.sceneRect(), downPen);

    // Set scene to view
    ui->graphicsView_view->setSceneRect(plotScene.sceneRect());
    ui->graphicsView_view->show();
}

void MainWindow::do_loadFile()
{
    if (inputFile.isOpen())
    {
        inputFile.close();
    }
    QString filePath = ui->lineEdit_filePath->text();
    if (filePath.isEmpty())
    {
        return;
    }
    inputFile.setFileName(filePath);
    inputFile.open(QIODevice::ReadOnly);
    objList.clear();
    //ui->textBrowser_read->clear();

    QString buffer = "";
    QTextStream fstream(&inputFile);
    buffer = fstream.readAll();
    objList.push_back(hpgl_obj(buffer));

    settings->setValue("MainWindow/filePath", filePath);

    do_drawView();
}

void MainWindow::do_plot()
{
    // Variables
    int cutterSpeed = ui->spinBox_cutterSpeed->value(); // in mm/s
    hpgl_obj obj;
    QString printThis;
    int cmdCount;
    QString command;
    double time;
    QProcess process;

    qDebug() << "Plotting file!";
    if (serialBuffer.isNull() || !serialBuffer->isOpen() || objList.isEmpty())
    {
        ui->textBrowser_console->append(timeStamp() + "Can't plot!");
        return;
    }
    for (int i = 0; i < objList.count(); i++)
    {
        obj = objList.at(i);
        cmdCount = obj.cmdCount();
        for (int cmd_index = 0; cmd_index < cmdCount; cmd_index++)
        {
            printThis = obj.cmdPrint(cmd_index);
            if (printThis == "OOB")
            {
                ui->textBrowser_console->append("ERROR: Object Out Of Bounds! Cannot Plot! D:");
                //ui->textBrowser_console->append("(try the auto translation button)");
                ui->textBrowser_console->append("(An X or Y value is less than zero)");
                return;
            }
            serialBuffer->write(printThis.toStdString().c_str());
            if (ui->checkBox_cutterSpeed->isChecked())
            {
                serialBuffer->flush();
                command = "sleep ";
                time = obj.cmdMM(cmd_index) / cutterSpeed;
                if (time == 0)
                    continue;
                command += QString::number(time);
                qDebug() << "Starting sleep command: sleep " << time;
                process.start(command);
                process.waitForFinished(60000); // Waits for up to 60s
                qDebug() << "Done with sleep command";
            }
        }
    }
    qDebug() << "Done plotting.";
}

void MainWindow::handle_objectTransform()
{
    QTransform Tscale, Trotate, Ttranslate;
    double scale = ui->doubleSpinBox_objScale->value();
    double rotation = ui->doubleSpinBox_objRotation->value();
    int translateX = ui->spinBox_objTranslationX->value();
    int translateY = ui->spinBox_objTranslationY->value();

    Tscale.scale(scale, scale);
    Trotate.rotate(rotation);
    Ttranslate.translate(translateX, translateY);

    //qDebug() << "MATRIX: " << transform;

    for (int i = 0; i < objList.count(); i++)
    {
        objList[i].cmdTransformScale = Tscale;
        objList[i].cmdTransformRotate = Trotate;
        objList[i].cmdTransformTranslate = Ttranslate;
    }
    do_drawView();
}































