#include "qsweepparams.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

static const QString FREQUENCY_MIN_KEY = QStringLiteral("frequency_min");
static const QString FREQUENCY_MAX_KEY = QStringLiteral("frequency_max");
static const QString FFT_BIN_WIDTH_KEY = QStringLiteral("fft_bin_width");
static const QString LNA_GAIN_KEY = QStringLiteral("lna_gain");
static const QString VGA_GAIN_KEY = QStringLiteral("vga_gain");

QSweepParams::QSweepParams(QObject *parent) : QObject(parent)
{
    m_valid = false;
    m_frequency_min = 1;
    m_frequency_max = 6000;
    m_fft_bin_width = 500000;
    m_lna_gain = 0;
    m_vga_gain = 0;
}

QSweepParams::QSweepParams(const QByteArray &json, bool binary)
{
    QJsonDocument doc;
    if (binary)
        doc = QJsonDocument::fromBinaryData(json, QJsonDocument::BypassValidation);
    else
        doc = QJsonDocument::fromJson(json);

    QJsonObject jsonObject = doc.object();
    bool ok;
    m_frequency_min = jsonObject[FREQUENCY_MIN_KEY].toString().toUInt(&ok, 10);
    m_frequency_max = jsonObject[FREQUENCY_MAX_KEY].toString().toUInt();
    m_fft_bin_width = jsonObject[FFT_BIN_WIDTH_KEY].toString().toUInt();
    m_lna_gain = jsonObject[LNA_GAIN_KEY].toString().toUInt();
    m_vga_gain = jsonObject[VGA_GAIN_KEY].toString().toUInt();

    if(!doc.isEmpty())
        m_valid = true;
    else
        m_valid = false;
}

void QSweepParams::setLnaGain(const quint32 &value)
{
    m_lna_gain = value;
}

quint32 QSweepParams::lnaGain() const
{
    return m_lna_gain;
}

void QSweepParams::setVgaGain(const quint32 &value)
{
    m_vga_gain = value;
}

quint32 QSweepParams::vgaGain() const
{
    return m_vga_gain;
}

void QSweepParams::setFFTBinWidth(const quint32 &value)
{
    m_fft_bin_width = value;
}

quint32 QSweepParams::fftBinWidth() const
{
    return m_fft_bin_width;
}

void QSweepParams::setFrequencyMin(const quint32 &value)
{
    m_frequency_min = value;
}

quint32 QSweepParams::frequencyMin() const
{
    return m_frequency_min;
}

void QSweepParams::setFrequencyMax(const quint32 &value)
{
    m_frequency_max = value;
}

quint32 QSweepParams::frequencyMax() const
{
    return m_frequency_max;
}

QByteArray QSweepParams::exportToJson(bool binary) const
{
    QJsonObject jsonObject;
    jsonObject[FREQUENCY_MIN_KEY] = QString::number(m_frequency_min);
    jsonObject[FREQUENCY_MAX_KEY] = QString::number(m_frequency_max);
    jsonObject[FFT_BIN_WIDTH_KEY] = QString::number(m_fft_bin_width);
    jsonObject[LNA_GAIN_KEY] = QString::number(m_lna_gain);
    jsonObject[VGA_GAIN_KEY] = QString::number(m_vga_gain);

    QJsonDocument doc(jsonObject);

    if(binary)
        return doc.toBinaryData();
    else
        return doc.toJson(QJsonDocument::Compact);
}