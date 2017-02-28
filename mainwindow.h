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
#include <QGraphicsTextItem>
#include <QPushButton>
#include <QGraphicsItem>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QListView>
#include <QModelIndex>

#include "hpgl.h"
#include "settings.h"
#include "ancilla.h"
#include "etc.h"
#include "hpglgraphicsview.h"

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
    void please_plotter_doPlot(const QVector<QGraphicsPolygonItem *>);
    void please_plotter_cancelPlot();
    void please_plotter_loadFile(QString filePath);

/*
 * do_      for ui/proc action
 * update_  for settings change
 * handle_  for ui update
 */
private slots:
    void do_loadFile(QString filePath);
    void do_openDialogAbout();
    void do_openDialogSettings();

    // UI
    void update_filePath();
    void handle_selectFileBtn();
    void handle_plotStarted();
    void handle_plotCancelled();
    void handle_plotFinished();
    void handle_plottingPercent(int percent);

    // View/Scene
    void sceneSetup();
    void do_updatePens();
    void sceneClearHpgl();
    void sceneSetSceneRect();
    void sceneConstrainItems();
    void handle_groupingItems();
    void addPolygon(QPolygonF poly);

    // plotter thread
    void do_plot();
    void do_cancelPlot();
    void handle_ancillaThreadStart();
    void handle_ancillaThreadQuit();
    void handle_ancillaThreadStatus(QString _consoleText);

protected:
//    bool eventFilter(QObject *obj, QEvent *ev);

private:
    Ui::MainWindow *ui;
    QGraphicsScene plotScene;
//    hpglScene plotScene;
    QPen downPen;
    QPen upPen;
    QThread ancillaryThreadInstance;
    QPointer<AncillaryThread> ancilla;
    QVector<QGraphicsPolygonItem *> hpgl_items;
    QTimer drawTimer; // Measures performance of drawView()
    QGraphicsItemGroup * hpgl_items_group;
    QStringListModel * listModel;
};

#endif // MAINWINDOW_H
