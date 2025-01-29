#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDateTime>
#include <QChartView>
#include <QLineSeries>
#include <QChart>
#include <QValueAxis>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void loop();
    bool insertarDatos();
private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *manager;

    QChart *chartDis;
    QLineSeries *seriesdistancia;
    QValueAxis *axisXDis;
    QValueAxis *axisYDis;
    qint64 startTime;

    void fetchSensorData();
    void updateChart(int personCount);
    void updateLCD(int personCount);
    void updateTextEdit(const QString &status);
};

#endif // MAINWINDOW_H
