#include "sweep_write_settings.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>

static const QString HOST_BROKER_KEY = QStringLiteral("host_broker");
static const QString PORT_BROKER_KEY = QStringLiteral("port_broker");
static const QString DELAYED_LAUNCH_KEY = QStringLiteral("delayed_launch");
static const QString DB_PATH_KEY = QStringLiteral("db_path");
static const QString DB_CHUNK_KEY = QStringLiteral("db_chunk");
static const QString DB_CHUNK_SIZE_MB_KEY = QStringLiteral("db_chunk_size_mb");

class sweep_write_settings_data : public QSharedData {
public:
    sweep_write_settings_data(): QSharedData()
    {
        m_valid = false;
        m_host_broker = "127.0.0.1";
        m_port_broker = 1883;
        m_delayed_launch = 1000;
        m_db_path = "";
        m_db_chunk = 1;
        m_db_chunk_size_mb = 100;
    }
    sweep_write_settings_data(const sweep_write_settings_data &other) : QSharedData(other)
    {
        m_valid = other.m_valid;
        m_host_broker = other.m_host_broker;
        m_port_broker = other.m_port_broker;
        m_delayed_launch = other.m_delayed_launch;
        m_db_path = other.m_db_path;
        m_db_chunk = other.m_db_chunk;
        m_db_chunk_size_mb = other.m_db_chunk_size_mb;
    }

    ~sweep_write_settings_data() {}

    bool m_valid;
    int m_delayed_launch;
    // broker
    quint16 m_port_broker;
    QString m_host_broker;
    // database sqlite
    QString m_db_path;
    // database chunk count
    qint32 m_db_chunk;
    qint32 m_db_chunk_size_mb;
};

sweep_write_settings::sweep_write_settings() : data(new sweep_write_settings_data)
{
}

sweep_write_settings::sweep_write_settings(const sweep_write_settings &rhs) : data(rhs.data)
{
}

sweep_write_settings::sweep_write_settings(const QByteArray &json, const bool binary) : data(new sweep_write_settings_data)
{
    QJsonDocument doc;
    if (binary)
        doc = QJsonDocument::fromBinaryData(json, QJsonDocument::BypassValidation);
    else
        doc = QJsonDocument::fromJson(json);

    const QJsonObject jsonObject = doc.object();
    data->m_host_broker = jsonObject.value(HOST_BROKER_KEY).toString();
    data->m_port_broker = jsonObject.value(PORT_BROKER_KEY).toString().toUShort();
    data->m_delayed_launch = jsonObject.value(DELAYED_LAUNCH_KEY).toInt(1000);
    data->m_db_path = jsonObject.value(DB_PATH_KEY).toString();
    data->m_db_chunk = jsonObject.value(DB_CHUNK_KEY).toInt(1);
    data->m_db_chunk_size_mb = jsonObject.value(DB_CHUNK_SIZE_MB_KEY).toInt(100);

    if(!doc.isEmpty())
        data->m_valid = true;
    else
        data->m_valid = false;
}

sweep_write_settings &sweep_write_settings::operator=(const sweep_write_settings &rhs)
{
    if (this != &rhs) {
        data.operator=(rhs.data);
    }
    return *this;
}

sweep_write_settings::~sweep_write_settings()
{
}

bool sweep_write_settings::is_valid() const
{
    return data->m_valid;
}

void sweep_write_settings::set_host_broker(const QString &value)
{
    data->m_host_broker = value;
}

QString sweep_write_settings::host_broker() const
{
    return data->m_host_broker;
}

void sweep_write_settings::set_port_broker(const quint16 &value)
{
    data->m_port_broker = value;
}

quint16 sweep_write_settings::port_broker() const
{
    return data->m_port_broker;
}

void sweep_write_settings::set_delayed_launch(const int &value)
{
    data->m_delayed_launch = value;
}

int sweep_write_settings::delayed_launch() const
{
    return data->m_delayed_launch;
}

void sweep_write_settings::set_db_path(const QString &value)
{
    data->m_db_path = value;
}

QString sweep_write_settings::db_path() const
{
    return data->m_db_path;
}

void sweep_write_settings::set_db_chunk(const qint32 &value)
{
    data->m_db_chunk = value;
}

qint32 sweep_write_settings::db_chunk() const
{
    return data->m_db_chunk;
}

void sweep_write_settings::set_db_chunk_size_mb(const qint32 &value)
{
    data->m_db_chunk_size_mb = value;
}

qint32 sweep_write_settings::db_chunk_size_mb() const
{
    return data->m_db_chunk_size_mb;
}

QByteArray sweep_write_settings::exportToJson(const bool binary, const bool isCompact) const
{
    QJsonObject jsonObject;
    jsonObject.insert(HOST_BROKER_KEY, data->m_host_broker);
    jsonObject.insert(PORT_BROKER_KEY, QString::number(data->m_port_broker));
    jsonObject.insert(DELAYED_LAUNCH_KEY, data->m_delayed_launch);
    jsonObject.insert(DB_PATH_KEY, data->m_db_path);
    jsonObject.insert(DB_CHUNK_KEY, data->m_db_chunk);
    jsonObject.insert(DB_CHUNK_SIZE_MB_KEY, data->m_db_chunk_size_mb);

    QJsonDocument doc(jsonObject);

    if(binary)
        return doc.toBinaryData();
    else{
        if(isCompact)
            return doc.toJson(QJsonDocument::Compact);
        else
            return doc.toJson();
    }
}
