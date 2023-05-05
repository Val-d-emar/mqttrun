#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtMqtt/QMqttClient>
#include <QtWidgets/QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , strList(new QStringList)
{
    ui->setupUi(this);
    mqtt_cli = new QMqttClient(this);
    ui->log->hide();
    ui->log->insertPlainText(tr("Журнал: \n"));
    // устанавливаем обработчик сообщений, полученных от MQTT-брокера
    connect(mqtt_cli, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic) {
        ui->log->insertPlainText(QDateTime::currentDateTime().toString("hh:mm:ss")
                                 + tr(" Принято из: \"")
                                 + topic.name()
                                 + tr("\" сообщение: ")
                                 + message
                                 + QLatin1Char('\n'));
    });
    // устанавливаем обработчик изменения состояния подключения к MQTT-брокеру
    connect(mqtt_cli, &QMqttClient::stateChanged, this, &MainWindow::stateChanged);
}
void MainWindow::resizeEvent(QResizeEvent *event){
    if (event->size().width() > ui->log->geometry().left() + ui->log->width()){
        ui->log->show();
    } else {
        ui->log->hide();
    }
}

void MainWindow::stateChanged()
{

    ui->log->insertPlainText(QDateTime::currentDateTime().toString("hh:mm:ss")
                             + tr(": Изменение статуса подключения на ")
                             + QString::number(mqtt_cli->state())
                             + QLatin1Char('\n'));

    if (mqtt_cli->state()== QMqttClient::Connected){
        // подписываемся на топик, указанный в соответствующем поле
        subscription = mqtt_cli->subscribe(ui->lineEdit_topic->text());
        if (subscription){
            // устанавливаем обработчик изменения состояния подписки на топик
            connect(subscription, &QMqttSubscription::stateChanged, this, &MainWindow::subStateChanged);
        } else {
            ui->log->insertPlainText(tr("Ошибка подписки на топик "+
                                            ui->lineEdit_topic->text().toUtf8()));
            ui->pushButton_RUN->setEnabled(true);
        }

    } else
        if (mqtt_cli->state()== QMqttClient::Disconnected){
            ui->pushButton_RUN->setEnabled(true);
        }
}

void MainWindow::subStateChanged()
{
    ui->log->insertPlainText(QDateTime::currentDateTime().toString("hh:mm:ss")
                             + tr(": Изменение статуса подписки на ")
                             + QString::number(mqtt_cli->state())
                             + QLatin1Char('\n'));

    if (subscription->state()== QMqttSubscription::Subscribed){
        // Передаём сообщения
        qint64 ncount = 0;
        for (qint64 i=0; i< strList->count();i++){
            //Проверка на числовость
            bool ok1, ok2;
            strList->value(i).toUtf8().toLongLong(&ok1, 10);
            strList->value(i).toUtf8().toDouble(&ok2);
            if(ok1 || ok2){
                if (mqtt_cli->publish(ui->lineEdit_topic->text(),
                                      strList->value(i).toUtf8()) == -1){
                    ui->log->insertPlainText(tr("Ошибка записи параметра "+
                                                strList->value(i).toUtf8()));
                } else{
                    ncount++;
                }
            }
        }
        subscription->unsubscribe();
        ui->summary->clear();
        ui->summary->insertPlainText(tr("Отправлено ")+
                                     QString::number(ncount)+
                                     tr(" строк из ")+
                                     QString::number(strList->count()));

    } else if(subscription->state()== QMqttSubscription::Unsubscribed){
        mqtt_cli->disconnectFromHost();
        ui->pushButton_RUN->setEnabled(true);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_RUN_clicked()
{
    ui->summary->clear();
    ui->summary->insertPlainText(tr("Отправлено 0 строк"));
    if (!QFile::exists(ui->lineEdit_filepath->text())){
        ui->log->insertPlainText(tr("Ошибка, файл \"")
                                 + ui->lineEdit_filepath->text()
                                 + tr("\" не найден\n"));
    }else{
        // создаем объект QFile и открываем файл для чтения
        QFile r_file(ui->lineEdit_filepath->text());
        if (!r_file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            ui->log->insertPlainText(tr("Ошибка открытия файла \"")
                                     + ui->lineEdit_filepath->text()
                                     + tr("\"\n"));
        } else {
            ui->pushButton_RUN->setDisabled(true);
            QTextStream in(&r_file);
            strList->clear()  ;
            // считываем содержимое файла
            while (!in.atEnd()) {
                QString line = in.readLine();
                strList->append(line);
            }
            //открываем соединение
            mqtt_cli->setHostname(ui->lineEdit_host->text());
            mqtt_cli->setPort(ui->spinBox_port->value());
            mqtt_cli->setUsername(ui->lineEdit_username->text());
            mqtt_cli->setPassword(ui->lineEdit_password->text());

                if (mqtt_cli->state() == QMqttClient::Disconnected) {
                    mqtt_cli->connectToHost();
                }
                else if (mqtt_cli->state()== QMqttClient::Connected){
                    mqtt_cli->disconnectFromHost();
                    ui->pushButton_RUN->setEnabled(true);
                }
        }
    }
}



