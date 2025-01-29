#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFocus();

    startTime = QDateTime::currentSecsSinceEpoch();

    chartDis = new QChart();
    seriesdistancia = new QLineSeries();
    chartDis->addSeries(seriesdistancia);
    chartDis->setTitle("Conteo de Personas");

    // Configuración del eje X para mostrar todo el día
    axisXDis = new QValueAxis();
    axisXDis->setTitleText("Tiempo (segundos)");
    axisXDis->setRange(0, 86400); // Rango fijo para 24 horas
    axisXDis->setTickCount(25);   // Mostrar una marca por hora
    axisXDis->setLabelFormat("%i");
    chartDis->addAxis(axisXDis, Qt::AlignBottom);
    seriesdistancia->attachAxis(axisXDis);

    // Configuración del eje Y para conteo
    axisYDis = new QValueAxis();
    axisYDis->setTitleText("Conteo de Personas");
    axisYDis->setRange(0, 100);  // Ajustar según necesidad
    chartDis->addAxis(axisYDis, Qt::AlignLeft);
    seriesdistancia->attachAxis(axisYDis);

    // Mostrar la gráfica en el widget
    ui->widget_4->setChart(chartDis);

    db.setHostName("127.0.0.1:3306");       // Cambia a la IP de tu servidor MySQL si no es local
    db.setDatabaseName("u301236925_Cucei_bath_db");   // Reemplaza con el nombre de tu base de datos
    db.setUserName("yoeldaray");         // Reemplaza con tu usuario de MySQL
    db.setPassword("Cucei_bath_db1");      // Reemplaza con tu contraseña de MySQL
    if (!db.open()) {
        qDebug() << "Error al conectar con la base de datos:" << db.lastError().text();
    } else {
        qDebug() << "Conexión exitosa con la base de datos";
    }

    manager = new QNetworkAccessManager(this);

    // Timer para actualizar datos cada 5 segundos
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::fetchSensorData);
    timer->start(5000);
}

void MainWindow::fetchSensorData() {
    QNetworkRequest request(QUrl("https://cucei-bath.website/data"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            qDebug() << "Respuesta del servidor: " << responseData;  // Para depuración

            QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
            if (!jsonResponse.isObject()) {
                qDebug() << "Error: Respuesta no es un JSON válido.";
                return;
            }

            QJsonObject jsonObj = jsonResponse.object();

            // Verificar que las claves esperadas existen en el JSON
            if (jsonObj.contains("conteo") && jsonObj.contains("estado")) {
                int personCount = jsonObj["conteo"].toInt();
                QString esp32Status = jsonObj["estado"].toString();

                qDebug() << "Conteo de Personas:" << personCount;
                qDebug() << "Estado ESP32:" << esp32Status;

                updateChart(personCount);
                updateLCD(personCount);
                updateTextEdit(esp32Status);
            } else {
                qDebug() << "Error: JSON no contiene las claves esperadas.";
            }
        } else {
            qDebug() << "Error en GET:" << reply->errorString();
        }
        reply->deleteLater();
    });
}

// Función para actualizar la gráfica con los datos recibidos
void MainWindow::updateChart(int personCount) {
    qint64 currentTime = QDateTime::currentSecsSinceEpoch() - startTime;
    seriesdistancia->append(currentTime, personCount);
}

// Función para actualizar el LCD con el conteo actual de personas
void MainWindow::updateLCD(int personCount) {
    ui->lcdNumber_4->display(personCount);
}

// Función para actualizar el QTextEdit con el estado actual de la ESP32
void MainWindow::updateTextEdit(const QString &status) {
    ui->textEdit->append("Estado ESP32: " + status);
}

void MainWindow::loop() {
    QDate fechaActual = QDate::currentDate();

    // Reinicia el conteo si ha cambiado el día
    if (fechaActual != ultimaFechaReinicio) {
        qDebug() << "Cambio de día detectado. Reiniciando conteo.";
        ultimoConteoEnviado = -1;  // Reinicia el conteo diario
        conteo = 0;
        ultimaFechaReinicio = fechaActual;  // Actualiza la fecha del último reinicio
    }

    // Verificar si los datos fueron recibidos correctamente
    if (conteo > 0) {
        // Comprobar si el conteo actual ya fue enviado a la base de datos
        if (conteo != ultimoConteoEnviado || estado != lastDoorState) {
            insertarDatos();
            // Actualizar los últimos valores conocidos
            lastDoorState = estado;
        } else {
            //qDebug() << "El conteo ya fue enviado a la base de datos. No se inserta de nuevo.";
        }
    } else {
        //qDebug() << "No se han recibido todos los datos necesarios.";
    }
}


bool MainWindow::insertarDatos() {

    if(estado != lastDoorState)
    {

    }else {
        //qDebug() << "El conteo ya fue enviado a la base de datos. No se inserta de nuevo.";
    }
    // Obtener el timestamp en formato epoch (segundos desde 1970)
    qint64 timestamp = QDateTime::currentSecsSinceEpoch();

    // Obtener el día de la semana (1 = lunes, 7 = domingo)
    int diaSemana = QDateTime::currentDateTime().date().dayOfWeek();

    // Mapear el día a su nombre en español
    QString nombreTabla;
    switch (diaSemana) {
    case 1: nombreTabla = "bath1L"; break;
    case 2: nombreTabla = "bath1M"; break;
    case 3: nombreTabla = "bath1I"; break;
    case 4: nombreTabla = "bath1J"; break;
    case 5: nombreTabla = "bath1V"; break;
    case 6: nombreTabla = "bath1S"; break;
    default:
        qDebug() << "Error: día de la semana no válido.";
        return false;
    }

    // Crear la consulta SQL para insertar datos en la tabla correspondiente
    QSqlQuery query;
    QString consulta = QString("INSERT INTO %1 (ID, contador, fecha) VALUES (:ID, :contador, :fecha)").arg(nombreTabla);
    query.prepare(consulta);

    // Asignar los valores a los placeholders
    query.bindValue(":ID", ID);
    query.bindValue(":contador", contador);
    query.bindValue(":fecha", conteo);

    // Ejecutar la consulta e imprimir el resultado
    if (!query.exec()) {
        qDebug() << "Error al insertar en la tabla" << nombreTabla << ":" << query.lastError().text();
        return false;  // Si hubo un error, regresa false
    }

    qDebug() << "Datos insertados exitosamente en" << nombreTabla
             << "ID:" << ID
             << ", contador:" << contador
             << ", fecha:" << fecha;

    ultimoConteoEnviado = conteo;  // Actualizar el último conteo enviado
    return true;  // Regresa true si la inserción fue exitosa
}

MainWindow::~MainWindow() {
    delete ui;
}
