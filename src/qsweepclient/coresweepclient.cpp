#include "coresweepclient.h"

#include <QApplication>
#include <QQmlContext>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QLineSeries>
#include <QChartView>
#include <QTime>
#include <QtCore/qdebug.h>

#include "userinterface.h"
#include "qsweeptopic.h"
#include "qsweeprequest.h"
#include "qsweepanswer.h"
#include "qsweepspectr.h"
#include "qhackrfinfo.h"
#include "qsweepmessagelog.h"
#include "qsweepsystemmonitor.h"
#include "chart/datasource.h"
#include "chart/waterfallitem.h"
#include "systemmonitorinterface.h"
#include "statesweepclient.h"
#include "settings/sweepclientsettings.h"

static const QString config_suffix(QString(".config"));

CoreSweepClient::CoreSweepClient(QObject *parent) : QObject(parent),
    ptrUserInterface(new UserInterface(this)),
    ptrMqttClient(new QMqttClient(this)),
    ptrSweepTopic(new QSweepTopic(this)),
    ptrHackrfInfoModel(new HackrfInfoModel(this)),
    ptrMessageLogModel(new MessageLogModel(this)),
    ptrDataSource(new DataSource(this)),
    ptrAxisX(new QValueAxis(this)),
    ptrAxisY(new QValueAxis(this)),
    ptrSystemMonitorInterface(new SystemMonitorInterface(this)),
    ptrStateSweepClient(new StateSweepClient(this))
{
    m_timerReceive = new QTime;
}

int CoreSweepClient::runCoreSweepClient(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setApplicationName("CoreSweepClient");
    QCoreApplication::setApplicationVersion("1.0");

    QApplication app(argc, argv);

    initialization();

    if(readSettings(app.applicationFilePath()))
    {
        launching();
    }

    ptrEngine = new QQmlApplicationEngine(this);

    // Настройка осей графика
    ptrAxisX = new QValueAxis(this);
    ptrAxisX->setTitleText("Freq");
    ptrAxisX->setMin(ptrUserInterface->frequencyMin());
    ptrAxisX->setMax(ptrUserInterface->frequencyMax());
//    axisX->setLabelFormat("%i");
//    axisX->setTickCount(1);

    ptrAxisY = new QValueAxis(this);
    ptrAxisY->setTitleText("Level");
    ptrAxisY->setMin(ptrDataSource->minValueY());
    ptrAxisY->setMax(ptrDataSource->maxValueY());
//    axisY->setLabelFormat("%g");
//    axisY->setTickCount(5);

    qmlRegisterType<WaterfallItem>("waterfall", 1, 0, "Waterfall");

    QQmlContext *context = ptrEngine->rootContext();
    context->setContextProperty("userInterface", ptrUserInterface);
    context->setContextProperty("hackrfInfoModel", ptrHackrfInfoModel);
    context->setContextProperty("messageLogModel", ptrMessageLogModel);
    context->setContextProperty("dataSource", ptrDataSource);
    context->setContextProperty("valueAxisY", ptrAxisY);
    context->setContextProperty("valueAxisX", ptrAxisX);
    context->setContextProperty("systemMonitorInterface", ptrSystemMonitorInterface);
    context->setContextProperty("stateSweepClient", ptrStateSweepClient);

    ptrEngine->load(QUrl(QLatin1String("qrc:/main.qml")));

    QObject *rootObject = ptrEngine->rootObjects().first();
//    QObject *qmlChartView = rootObject->findChild<QObject*>("chartViewSpectr");
//    qmlChartView->setProperty("title", tr("Spectr"));

    QObject *qmlPlotWaterfall = rootObject->findChild<QObject*>("plotWaterfall");
    WaterfallItem *plotWaterfall = static_cast<WaterfallItem *>(qmlPlotWaterfall);

    connect(ptrDataSource, &DataSource::sendPowerSpectr,
            plotWaterfall, &WaterfallItem::onPowerSpectr);

    if (ptrEngine->rootObjects().isEmpty())
        return -1;

    return app.exec();
}

void CoreSweepClient::onConnectToHost(const QString &host, const quint16 &port)
{
#ifdef QT_DEBUG
    qDebug() << Q_FUNC_INFO << tr("Connect to host:") << host << tr("port:") << port;
#endif

    if(ptrMqttClient)
    {
        ptrMqttClient->setHostname(host);
        ptrMqttClient->setPort(port);

        if (ptrMqttClient->state() == QMqttClient::Disconnected)
            ptrMqttClient->connectToHost();
    }
}

void CoreSweepClient::onDisconnectFromHost()
{
    if (ptrMqttClient->state() == QMqttClient::Connected){
        ptrMqttClient->unsubscribe(ptrSweepTopic->sweepTopic(QSweepTopic::TOPIC_DATA));
        ptrMqttClient->disconnectFromHost();
    }
}

void CoreSweepClient::initialization()
{
    // MQTT
    connect(ptrMqttClient, &QMqttClient::messageReceived,
            this, &CoreSweepClient::messageReceived);
    connect(ptrMqttClient, &QMqttClient::stateChanged,
            this, &CoreSweepClient::updateLogStateChange);
    connect(ptrMqttClient, &QMqttClient::disconnected,
            this, &CoreSweepClient::brokerDisconnected);
    connect(ptrMqttClient, &QMqttClient::connected,
            this, &CoreSweepClient::pingReceived);
    connect(ptrMqttClient, &QMqttClient::pingResponseReceived,
            this , &CoreSweepClient::pingReceived);
    // user interface
    connect(ptrUserInterface, &UserInterface::sendRequestSweepServer,
            this, &CoreSweepClient::sendingRequest);
    connect(this, &CoreSweepClient::sendStartSpectr,
            ptrUserInterface, &UserInterface::sendStartSpectr);
    connect(ptrUserInterface, &UserInterface::sendClearMaxPowerSpectr,
            ptrDataSource, &DataSource::clearMaxPowerSpectr);
    // StateSweepClient
    connect(this, &CoreSweepClient::sendStateConnected,
            ptrStateSweepClient, &StateSweepClient::onConnect);
    connect(this, &CoreSweepClient::sendStateConnected,
            ptrUserInterface, &UserInterface::onRequestSweepInfo);
    connect(this, &CoreSweepClient::sendStateDisconnected,
            ptrStateSweepClient, &StateSweepClient::onDisconnect);
}

bool CoreSweepClient::readSettings(const QString &file)
{
    bool isRead(false);

    if(!file.isEmpty())
    {
        QFileInfo info(file);
        const QString fileConfig(info.absolutePath()+QDir::separator()+info.baseName()+config_suffix);
        QFileInfo config(fileConfig);

        if(config.exists())
        {
            QFile file(fileConfig);

            if(file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                const SweepClientSettings settings(file.readAll(), false); // !!!!!!!!!!!!!!!

                ptrSweepClientSettings = new SweepClientSettings(file.readAll(), false);
                file.close();

                isRead = ptrSweepClientSettings->isValid();

                ptrUserInterface->onSweepClientSettings(settings);
                // MessageLogModel
                ptrMessageLogModel->setMaxSize(ptrSweepClientSettings->maxSizeMessageLog());


                return isRead;
            }else{
                qCritical("Can't file open ('%s').", qUtf8Printable(config.fileName()));
                qCritical("Error: '%s'", qUtf8Printable(file.errorString()));
            }
        }else{
            qCritical("File '%s' does not exist!", qUtf8Printable(config.fileName()));

            fprintf(stderr, "Create new file settings: %s\n", qUtf8Printable(fileConfig));
            saveSettings(fileConfig);
        }
    }

    return isRead;
}

bool CoreSweepClient::saveSettings(const QString &file)
{
    bool isSave(false);

    if(!file.isEmpty())
    {
        const QFileInfo info(file);
        const QString fileConfig(info.absolutePath()+QDir::separator()+info.baseName()+config_suffix);
        QFileInfo config(fileConfig);
        bool fileExists = config.exists();

        QFile file(fileConfig);

        if(file.open(QIODevice::ReadWrite | QIODevice::Text)) {
            if(fileExists){
                if(ptrSweepClientSettings)
                    file.write(ptrSweepClientSettings->exportToJson());
            }else{
                const auto defaultSettings = SweepClientSettings();
                file.write(defaultSettings.exportToJson(false, false));
            }

            file.close();
            isSave = true;
        }else{
            qCritical("Can't file open ('%s').", qUtf8Printable(config.fileName()));
            qCritical("Error: '%s'", qUtf8Printable(file.errorString()));
        }
    }

    return isSave;
}

void CoreSweepClient::launching()
{
    // connect to mqtt broker
    if(ptrMqttClient)
    {
        ptrMqttClient->setHostname(ptrUserInterface->sweepClientSettings().hostBroker());
        ptrMqttClient->setPort(ptrUserInterface->sweepClientSettings().portBroker());

        if (ptrMqttClient->state() == QMqttClient::Disconnected)
            ptrMqttClient->connectToHost();
    }

    // start timer
    m_timerReceive->start();
    m_sizeDatacReceive = 0;
}

void CoreSweepClient::messageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    switch (ptrSweepTopic->sweepTopic(topic.name())) {
    case QSweepTopic::TOPIC_INFO:
    {
        QSweepAnswer answer(message);
        const QHackrfInfo info(answer.dataAnswer(), false);

        if(info.isValid()){
            ptrHackrfInfoModel->addResult(info);

//#ifdef QT_DEBUG
//            qDebug() << "---------------------------------------------------";
//            qDebug() << Q_FUNC_INFO << tr("Type Answer:") << static_cast<qint32>(answer.typeAnswer());
//            qDebug() << Q_FUNC_INFO << tr("Index Board:") << info.indexBoard();
//            qDebug() << Q_FUNC_INFO << tr("Serial Numbers:") << info.serialNumbers();
//            qDebug() << Q_FUNC_INFO << tr("Board ID Number:") << info.boardID();
//            qDebug() << Q_FUNC_INFO << tr("Firmware Version:") << info.firmwareVersion();
//            qDebug() << Q_FUNC_INFO << tr("Part ID Number:") << info.partIDNumber();
//            qDebug() << Q_FUNC_INFO << tr("Libhackrf Version:") << info.libHackrfVersion();
//            qDebug() << Q_FUNC_INFO << tr("Size message (byte):") << message.size();
//#endif
        }
    }
        break;
    case QSweepTopic::TOPIC_MESSAGE_LOG:
    {
        QSweepAnswer answer(message);
        QSweepMessageLog log(answer.dataAnswer());
        if(ptrMessageLogModel)
            ptrMessageLogModel->addResult(log);
    }
        break;
    case QSweepTopic::TOPIC_POWER_SPECTR:
    {
        QSweepAnswer answer(message);
        QSweepSpectr powers(answer.dataAnswer());

        QVector<PowerSpectr> tmpPowerSpectr(powers.powerSpectr());

        std::sort(tmpPowerSpectr.begin(), tmpPowerSpectr.end(), [](const PowerSpectr& a, const PowerSpectr& b) {
            return a.m_frequency_min < b.m_frequency_min;
        });

        // for test
        ptrDataSource->updateDate(ptrUserInterface->frequencyMin(),
                                  ptrUserInterface->frequencyMax(),
                                  tmpPowerSpectr);

        // X update axis max & min freq
        ptrAxisX->setMin(ptrUserInterface->frequencyMin());
        ptrAxisX->setMax(ptrUserInterface->frequencyMax());
        // test
        emit sendStartSpectr();
    }
        break;

    case QSweepTopic::TOPIC_SYSTEM_MONITOR:
    {
        QSweepAnswer answer(message);
        QSweepSystemMonitor sysmon(answer.dataAnswer());
        if(ptrSystemMonitorInterface)
            ptrSystemMonitorInterface->setSystemMonitor(sysmon);
    }
        break;

    default:
        break;
    }

    m_sizeDatacReceive = m_sizeDatacReceive + message.size();

    if(m_timerReceive->elapsed()>=5000)
    {
        //qreal
#ifdef QT_DEBUG
        qDebug() << Q_FUNC_INFO
                 << "Data size:" << QString::number(m_sizeDatacReceive/1024.0, 'f', 2) << "Kbyte"
                 << "Data size:" << QString::number((m_sizeDatacReceive/1024)/1024.0, 'f', 2) << "Mbyte"
                 << QString("Time elapsed: %1 ms").arg(m_timerReceive->elapsed());
#endif
        m_timerReceive->restart();
        m_sizeDatacReceive = 0;
    }
//QString::number(12.625, 'f', 2);.
//    if(m_timerReceive->elapsed()>=5000)
//    {
//        qDebug("Time elapsed: %d ms", m_timerReceive->elapsed());
//        qDebug() << Q_FUNC_INFO
//                 << m_sizeDatacReceive
//                 << "(" << m_sizeDatacReceive/1024 << ")"
//                 << m_timerReceive->elapsed()/1000;

//        m_timerReceive->restart();
//        m_sizeDatacReceive = 0;
//    }

#ifdef QT_DEBUG
//    qDebug() << "---------------------------------------------------";
//    qDebug() << Q_FUNC_INFO << topic.name() << ":" << "message size:" << message.size() << "byte";
//    qDebug() << "---------------------------------------------------";
#endif

}

void CoreSweepClient::updateLogStateChange()
{
    const QString content = QDateTime::currentDateTime().toString()
            + QLatin1String(": State Change")
            + QString::number(ptrMqttClient->state());

    // Subscribers topic
    if (ptrMqttClient->state() == QMqttClient::Connected)
    {
        emit sendStateConnected();

        auto subscription = ptrMqttClient->subscribe(ptrSweepTopic->sweepTopic(QSweepTopic::TOPIC_INFO));

        if (!subscription)
        {
#ifdef QT_DEBUG
            qDebug() << Q_FUNC_INFO << "Could not subscribe. Is there a valid connection?";
#endif
            return;
        }

        auto subscription1 = ptrMqttClient->subscribe(ptrSweepTopic->sweepTopic(QSweepTopic::TOPIC_MESSAGE_LOG));

        if (!subscription1)
        {
#ifdef QT_DEBUG
            qDebug() << Q_FUNC_INFO << "Could not subscribe. Is there a valid connection?";
#endif
            return;
        }

        auto subscription2 = ptrMqttClient->subscribe(ptrSweepTopic->sweepTopic(QSweepTopic::TOPIC_POWER_SPECTR));

        if (!subscription2)
        {
#ifdef QT_DEBUG
            qDebug() << Q_FUNC_INFO << "Could not subscribe. Is there a valid connection?";
#endif
            return;
        }

        auto subscription3 = ptrMqttClient->subscribe(ptrSweepTopic->sweepTopic(QSweepTopic::TOPIC_SYSTEM_MONITOR));

        if (!subscription3)
        {
#ifdef QT_DEBUG
            qDebug() << Q_FUNC_INFO << "Could not subscribe. Is there a valid connection?";
#endif
            return;
        }
    }

    if (ptrMqttClient->state() == QMqttClient::Disconnected)
    {
        emit sendStateDisconnected();
    }

#ifdef QT_DEBUG
    qDebug() << Q_FUNC_INFO << content;
#endif
}

void CoreSweepClient::brokerDisconnected()
{
#ifdef QT_DEBUG
    qDebug() << Q_FUNC_INFO;
#endif
}

void CoreSweepClient::pingReceived()
{
    ptrUserInterface->onPingReceived();

#ifdef QT_DEBUG
    qDebug() << Q_FUNC_INFO;
#endif
}

void CoreSweepClient::connecting()
{
#ifdef QT_DEBUG
    qDebug() << Q_FUNC_INFO;
#endif
}

void CoreSweepClient::sendingRequest(const QSweepRequest &value)
{
    if(ptrMqttClient) {
        if (ptrMqttClient->state() == QMqttClient::Connected) {
            qint32 result = ptrMqttClient->publish(ptrSweepTopic->sweepTopic(QSweepTopic::TOPIC_CTRL), value.exportToJson());
#ifdef QT_DEBUG
            qDebug() << Q_FUNC_INFO << tr("Data sending to host result:") << result << value.exportToJson();
#endif
        }
    }
}
