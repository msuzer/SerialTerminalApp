// SerialPortManager.cpp
#include "serialport_manager.h"
#include <QTextStream>

SerialPortManager::SerialPortManager(QObject *parent) : QObject(parent) {
    connect(&serial, &QSerialPort::readyRead, this, &SerialPortManager::handleReadyRead);
}

SerialPortManager::~SerialPortManager() {
    close();
}

bool SerialPortManager::open(const QString &portName, int baudRate, const QString &format) {
    close();
    serial.setPortName(portName);
    serial.setBaudRate(baudRate);
    if (!applyFormat(format)) {
        emit errorOccurred("Unsupported serial format: " + format);
        return false;
    }
    serial.setFlowControl(QSerialPort::NoFlowControl);
    if (serial.open(QIODevice::ReadWrite)) {
        emit connected(QString("Connected to %1 @ %2 baud (%3)").arg(portName).arg(baudRate).arg(format));
        return true;
    } else {
        emit errorOccurred("Failed to open port: " + serial.errorString());
        return false;
    }
}

void SerialPortManager::close() {
    if (serial.isOpen()) {
        serial.close();
        emit disconnected();
    }
}

bool SerialPortManager::isOpen() const {
    return serial.isOpen();
}

void SerialPortManager::write(const QString &text, const QString &eol) {
    if (serial.isOpen()) {
        serial.write((text + eol).toUtf8());
    }
}

QString SerialPortManager::portName() const {
    return serial.portName();
}

int SerialPortManager::baudRate() const {
    return serial.baudRate();
}

void SerialPortManager::handleReadyRead() {
    buffer += QString::fromUtf8(serial.readAll());
    int idx;
    while ((idx = buffer.indexOf('\n')) != -1) {
        QString line = buffer.left(idx).trimmed();
        buffer.remove(0, idx + 1);
        emit dataReceived(line);
    }
    if (buffer.length() > 1024) {
        emit dataReceived("[WARN] Incomplete line flushed: " + buffer.left(1024));
        buffer.clear();
    }
}

bool SerialPortManager::applyFormat(const QString &format) {
    if (format.length() < 3) return false;
    int dataBits = format[0].digitValue();
    switch (dataBits) {
        case 5: serial.setDataBits(QSerialPort::Data5); break;
        case 6: serial.setDataBits(QSerialPort::Data6); break;
        case 7: serial.setDataBits(QSerialPort::Data7); break;
        case 8: serial.setDataBits(QSerialPort::Data8); break;
        default: return false;
    }
    QChar parityChar = format[1].toUpper();
    if      (parityChar == 'N') serial.setParity(QSerialPort::NoParity);
    else if (parityChar == 'E') serial.setParity(QSerialPort::EvenParity);
    else if (parityChar == 'O') serial.setParity(QSerialPort::OddParity);
    else if (parityChar == 'M') serial.setParity(QSerialPort::MarkParity);
    else if (parityChar == 'S') serial.setParity(QSerialPort::SpaceParity);
    else return false;
    QString stopBitStr = format.mid(2);
    if (stopBitStr == "1") serial.setStopBits(QSerialPort::OneStop);
    else if (stopBitStr == "2") serial.setStopBits(QSerialPort::TwoStop);
    else if (stopBitStr == "1.5" && dataBits == 5) serial.setStopBits(QSerialPort::OneAndHalfStop);
    else return false;
    return true;
}
