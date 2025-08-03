// SerialPortManager.h
#pragma once

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>

class SerialPortManager : public QObject {
    Q_OBJECT

public:
    explicit SerialPortManager(QObject *parent = nullptr);
    ~SerialPortManager();

    bool open(const QString &portName, int baudRate, const QString &format);
    void close();
    bool isOpen() const;

    void write(const QString &text, const QString &eol);
    QString portName() const;
    int baudRate() const;

signals:
    void dataReceived(const QString &line);
    void errorOccurred(const QString &message);
    void connected(const QString &summary);
    void disconnected();

private slots:
    void handleReadyRead();

private:
    QSerialPort serial;
    QString buffer;

    bool applyFormat(const QString &format);
};
