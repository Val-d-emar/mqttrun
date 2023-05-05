#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFile>
#include <QMainWindow>
#include <QMqttClient>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_RUN_clicked();
    virtual void resizeEvent ( QResizeEvent * e);

private:
    Ui::MainWindow *ui;
    QMqttClient *mqtt_cli;
    QFile *r_file;
    QStringList *strList;
    void stateChanged();
    void subStateChanged();
    QMqttSubscription *subscription;
};
#endif // MAINWINDOW_H
