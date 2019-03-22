#ifndef SWEEP_WRITE_SETTINGS_H
#define SWEEP_WRITE_SETTINGS_H

#include <QtCore/qshareddata.h>
#include <QtCore/qmetatype.h>

class sweep_write_settings_data;

class sweep_write_settings
{
public:
    sweep_write_settings();
    sweep_write_settings(const sweep_write_settings &);
    sweep_write_settings(const QByteArray &json, const bool binary = false);
    sweep_write_settings &operator=(const sweep_write_settings &);
    ~sweep_write_settings();

    bool is_valid() const;

    void set_host_broker(const QString &);
    QString host_broker()const;

    void set_port_broker(const quint16 &);
    quint16 port_broker()const;

    void set_delayed_launch(const int &);
    int delayed_launch()const;

    void set_db_path(const QString &);
    QString db_path()const;

    QByteArray exportToJson(const bool binary = false, const bool isCompact = true) const;

private:
    QSharedDataPointer<sweep_write_settings_data> data;
};

Q_DECLARE_METATYPE(sweep_write_settings)

#endif // SWEEP_WRITE_SETTINGS_H
