// MainWindow.h
#pragma once

#include <QMainWindow>
#include <QSettings>
#include <QLabel>
#include "serialport_manager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void handleBaudRateSelected(const QString &text);
    void handleConnectClicked();
    void handleDisconnectClicked();
    void handleSendClicked();
    void handleSerialReceived(const QString &line);
    void handleSerialConnected(const QString &msg);
    void handleSerialDisconnected();
    void handleSerialError(const QString &error);

    void saveLogToFile();
    void refreshPortList();
    void applyDarkTheme(bool enabled);
    void clearCommandHistory();
    void sortHistoryAZ();
    void showHistoryContextMenu(const QPoint &pos);

private:
    Ui::MainWindow *ui;
    SerialPortManager *serialMgr;
    QSettings settings;
    QStringList commandHistory;

    void setupConnections();
    void updateUiState(bool isConnected);
    void setStatusLED(const QString &state);
    QString getSelectedEOL() const;
    void printLine(const QString &line, const QString &type = "rx");
    void saveSettings();
    void loadSettings();
    void loadCommandHistory();
    void saveCommandHistory();
};
