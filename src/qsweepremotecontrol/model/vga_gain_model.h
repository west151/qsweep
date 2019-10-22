#ifndef vga_gain_model_H
#define vga_gain_model_H

#include <QAbstractListModel>
#include "vga_gain.h"

class vga_gain_model : public QAbstractListModel
{
    Q_OBJECT
public:

    enum data_role {
        DATAROLE_TITLE = Qt::UserRole + 1,
        DATAROLE_VGA_GAIN
    };

    explicit vga_gain_model(QObject* parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QHash< int, QByteArray > roleNames() const Q_DECL_OVERRIDE;

    void add_result(const QString &title, const quint32 &value);

    Q_INVOKABLE quint32 get_lna_gain_value_by_index(const int &index);

private:
    QList<vga_gain> m_data;

};

#endif // vga_gain_model_H
