/**
 * Localplot - Main UI thread header
 * Christopher Bero <bigbero@gmail.com>
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMainWindow>
#include <QFileDialog>
#include <QGuiApplication>
#include <QScreen>

#include "hpgl.h"
#include "settings.h"
#include "ancilla.h"
#include "etc.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

/*
 * please_  for signals to threads
 */
signals:
    void please_plotter_openSerial();
    void please_plotter_closeSerial();
    void please_plotter_doPlot();
    void please_plotter_cancelPlot();
    void please_plotter_loadFile(QString filePath);

/*
 * do_      for ui action
 * update_  for settings change
 * handle_  for ui update
 */
private slots:
    void do_loadFile(QString filePath);
    void do_drawView();
    void do_updatePens();
    void do_openDialogAbout();
    void do_openDialogSettings();

    // plotter thread
//    void do_openSerial();
//    void do_closeSerial();
    void do_plot();
    void do_cancelPlot();
    void handle_ancillaThreadStart();
    void handle_ancillaThreadQuit();

    void update_filePath();

    void handle_serialOpened();
    void handle_serialClosed();
    void handle_serialConnectBtn();
    void handle_selectFileBtn();
    void handle_objectTransform();
    void handle_plotStarted();
    void handle_plotCancelled();
//    void handle_autoTranslateBtn();
    void handle_plottingPercent(int percent);

    void addPolygon(QPolygonF poly);

private:
    Ui::MainWindow *ui;
    QGraphicsScene plotScene;
    QGraphicsScene penDownDemoScene;
    QGraphicsScene penUpDemoScene;
    QPen downPen;
    QPen upPen;
    AncillaryThread * ancilla;
    QVector<QPolygonF *> hpgl_items;
};

#endif // MAINWINDOW_H
