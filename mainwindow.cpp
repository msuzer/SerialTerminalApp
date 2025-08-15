#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QTime>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDateTime>
#include <QCloseEvent>
#include <QMenu>
#include <QScrollBar>
#include <QStatusBar>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , serialMgr(new SerialPortManager(this))
    , settings("MyCompany", "SerialTerminalApp")
{
    ui->setupUi(this);

    statusBar()->addPermanentWidget(ui->labelStatusLED);
    statusBar()->addPermanentWidget(ui->labelStatusText);

    setupConnections();
    ui->textOutput->installEventFilter(this);

    refreshPortList();
    loadSettings();
    setStatusLED("ready");
    updateUiState(false);
}

MainWindow::~MainWindow() {
    saveSettings();
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    serialMgr->close();        // ✅ Make sure port is disconnected!
    saveSettings();
    QMainWindow::closeEvent(event);  // pass to base class
}

void MainWindow::setupConnections() {
    connect(ui->comboBaud, &QComboBox::currentTextChanged, this, &MainWindow::handleBaudRateSelected);
    connect(ui->btnConnect, &QPushButton::clicked, this, &MainWindow::handleConnectClicked);
    connect(ui->btnDisconnect, &QPushButton::clicked, this, &MainWindow::handleDisconnectClicked);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &MainWindow::refreshPortList);
    connect(ui->btnSend, &QPushButton::clicked, this, &MainWindow::handleSendClicked);
    connect(ui->lineInput, &QLineEdit::returnPressed, this, &MainWindow::handleSendClicked);
    connect(ui->btnSaveLog, &QPushButton::clicked, this, &MainWindow::saveLogToFile);
    connect(ui->btnClearLog, &QPushButton::clicked, [this]() { ui->textOutput->clear(); });
    connect(ui->btnSortHistory, &QPushButton::clicked, this, &MainWindow::sortHistoryAZ);
    connect(ui->btnClearHistory, &QPushButton::clicked, this, &MainWindow::clearCommandHistory);
    connect(ui->checkDarkTheme, &QCheckBox::toggled, this, &MainWindow::applyDarkTheme);
    connect(ui->listHistory, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        ui->lineInput->setText(item->text());
    });
    ui->listHistory->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listHistory, &QListWidget::customContextMenuRequested, this, &MainWindow::showHistoryContextMenu);

    connect(serialMgr, &SerialPortManager::dataReceived, this, &MainWindow::handleSerialReceived);
    connect(serialMgr, &SerialPortManager::connected, this, &MainWindow::handleSerialConnected);
    connect(serialMgr, &SerialPortManager::disconnected, this, &MainWindow::handleSerialDisconnected);
    connect(serialMgr, &SerialPortManager::errorOccurred, this, &MainWindow::handleSerialError);
}

void MainWindow::handleBaudRateSelected(const QString &text) {
    if (text == "Custom...") {
        bool ok;
        int baud = ui->comboBaud->currentText().toInt();
        int customBaud = QInputDialog::getInt(this, "Custom Baud Rate", "Enter custom baud rate:",
                                              baud, 50, 4000000, 1, &ok);
        if (!ok) {
            // Revert to previous valid selection
            int lastIndex = settings.value("baudIndex", 0).toInt();
            ui->comboBaud->setCurrentIndex(lastIndex);
            return;
        }

        QString customStr = QString::number(customBaud);
        int exists = ui->comboBaud->findText(customStr);
        if (exists == -1) {
            ui->comboBaud->insertItem(ui->comboBaud->count() - 1, customStr);  // before "Custom..."
        }

        ui->comboBaud->setCurrentText(customStr);
    }
}

void MainWindow::handleConnectClicked() {
    QString port = ui->comboPort->currentText();
    int baud = ui->comboBaud->currentText().toInt();
    QString format = ui->comboFormat->currentText();
    if (serialMgr->open(port, baud, format)) {
        updateUiState(true);
    }
}

void MainWindow::handleDisconnectClicked() {
    serialMgr->close();
    updateUiState(false);
}

void MainWindow::handleSendClicked() {
    QString text = ui->lineInput->text().trimmed();
    if (!text.isEmpty() && serialMgr->isOpen()) {
        serialMgr->write(text, getSelectedEOL());
        printLine(text, "tx");
        if (!commandHistory.contains(text)) {
            commandHistory.append(text);
            ui->listHistory->addItem(text);
        }
        ui->lineInput->clear();
    }
}

void MainWindow::handleSerialReceived(const QString &line) {
    printLine(line, "rx");
}

void MainWindow::handleSerialConnected(const QString &msg) {
    printLine(msg, "info");
    ui->labelStatusText->setText(msg);
    ui->labelStatusLED->setStyleSheet("background-color: green; border-radius: 6px;");
}

void MainWindow::handleSerialDisconnected() {
    printLine("Disconnected", "warn");
    ui->labelStatusText->setText("Disconnected");
    ui->labelStatusLED->setStyleSheet("background-color: red; border-radius: 6px;");
}

void MainWindow::handleSerialError(const QString &error) {
    printLine(error, "warn");
    QMessageBox::critical(this, "Serial Error", error);
}

void MainWindow::refreshPortList() {
    ui->comboPort->clear();
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        ui->comboPort->addItem(info.portName());
    }
}

void MainWindow::saveSettings() {
    settings.setValue("port", ui->comboPort->currentText());
    settings.setValue("baud", ui->comboBaud->currentText());
    settings.setValue("baudIndex", ui->comboBaud->currentIndex());
    settings.setValue("format", ui->comboFormat->currentIndex());
    settings.setValue("eol", ui->comboEOL->currentIndex());
    settings.setValue("timestamp", ui->checkTimestamp->isChecked());
    settings.setValue("autoscroll", ui->checkAutoScroll->isChecked());

    settings.setValue("darkTheme", ui->checkDarkTheme->isChecked());
    settings.setValue("fontSize", ui->textOutput->font().pointSize());

    settings.setValue("mainWindowSize", size());   // save size
    settings.setValue("mainWindowPos", pos());     // optional: save position

    saveCommandHistory();  // ✅
}

void MainWindow::loadSettings() {
    // 0 ensures "8N1" is default when no setting is saved
    ui->comboFormat->setCurrentIndex(settings.value("format", 0).toInt());

    ui->comboEOL->setCurrentIndex(settings.value("eol", 1).toInt());  // default: \n
    ui->checkTimestamp->setChecked(settings.value("timestamp", false).toBool());
    ui->checkAutoScroll->setChecked(settings.value("autoscroll", true).toBool());

    ui->checkDarkTheme->setChecked(settings.value("darkTheme", false).toBool());
    int fontSize = settings.value("fontSize", 10).toInt();
    QFont f("Courier", fontSize);
    ui->textOutput->setFont(f);

    QVariant savedSize = settings.value("mainWindowSize");
    if (savedSize.isValid()) {
        resize(savedSize.toSize());
    }

    QVariant savedPos = settings.value("mainWindowPos");
    if (savedPos.isValid()) {
        move(savedPos.toPoint());
    }

    loadCommandHistory();  // ✅
    sortHistoryAZ();          // ensure A–Z on startup

    QString port = settings.value("port", "").toString();
    QString baud = settings.value("baud", "9600").toString();

    int portIndex = ui->comboPort->findText(port);
    if (portIndex != -1)
        ui->comboPort->setCurrentIndex(portIndex);
    int baudIndex = ui->comboBaud->findText(baud);
    if (baudIndex != -1)
        ui->comboBaud->setCurrentIndex(baudIndex);
}

void MainWindow::saveLogToFile() {
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString defaultFile = QString("%1/serial_log_%2.txt").arg(desktopPath, timestamp);

    QString fileName = QFileDialog::getSaveFileName(this, "Save Log File", defaultFile, "Text Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << ui->textOutput->toPlainText();
        file.close();
    } else {
        QMessageBox::warning(this, "Error", "Failed to save file: " + file.errorString());
    }
}

void MainWindow::updateUiState(bool isConnected) {
    ui->btnConnect->setEnabled(!isConnected);
    ui->btnDisconnect->setEnabled(isConnected);
    ui->comboPort->setEnabled(!isConnected);
    ui->comboBaud->setEnabled(!isConnected);
    ui->comboFormat->setEnabled(!isConnected);
}

QString MainWindow::getSelectedEOL() const {
    switch (ui->comboEOL->currentIndex()) {
    case 1: return "\n";
    case 2: return "\r";
    case 3: return "\r\n";
    default: return "";
    }
}

void MainWindow::printLine(const QString& line, const QString& type) {
    if (ui->checkPause->isChecked()) return;

    QString ts;
    if (ui->checkTimestamp->isChecked()) {
        // ts = "[" + QTime::currentTime().toString("HH:mm:ss.zzz") + "] ";
        ts = "[" + QTime::currentTime().toString("HH:mm:ss") + "] ";
    }

    QString styled = line.toHtmlEscaped();  // escape for safety

    QString prefix;
    QString color;

    if (type == "warn") {
        prefix = "[WARN] ";
        color = "orange";
    } else if (type == "tx") {
        prefix = "[TX] ";
        color = "lightblue";
    } else if (type == "info") {
        prefix = "[INFO] ";
        color = "gray";
    } else {
        prefix = "[RX] ";
        color = "";  // Use inherited style
    }

    QString output;
    if (color.isEmpty())
        output = QString("<span>%1%2</span>").arg(ts, styled);
    else
        output = QString("<span style='color:%1;'>%2%3</span>").arg(color, ts, styled);

    ui->textOutput->append(output);

    if (ui->checkAutoScroll->isChecked()) {
        QScrollBar *vScroll = ui->textOutput->verticalScrollBar();
        vScroll->setValue(vScroll->maximum());
    }
}

void MainWindow::showHistoryContextMenu(const QPoint &pos) {
    QListWidgetItem *item = ui->listHistory->itemAt(pos);
    if (!item) return;

    QMenu contextMenu;
    QAction *deleteAction = contextMenu.addAction("Delete");
    QAction *selected = contextMenu.exec(ui->listHistory->viewport()->mapToGlobal(pos));
    if (selected == deleteAction) {
        commandHistory.removeAll(item->text());
        delete item;
    }
}

void MainWindow::loadCommandHistory() {
    commandHistory = settings.value("cmdHistory").toStringList();
    ui->listHistory->clear();
    for (const QString &cmd : commandHistory) {
        ui->listHistory->addItem(cmd);
    }
}

void MainWindow::saveCommandHistory() {
    settings.setValue("cmdHistory", commandHistory);
}

void MainWindow::sortHistoryAZ() {
    // Sort the visible list (case-insensitive)
    ui->listHistory->sortItems(Qt::AscendingOrder);

    // Sync back to the in-memory list and save
    QStringList items;
    items.reserve(ui->listHistory->count());
    for (int i = 0; i < ui->listHistory->count(); ++i) {
        items << ui->listHistory->item(i)->text();
    }
    items.removeDuplicates();                // keep unique entries
    commandHistory = items;                  // update your persistent list
    saveCommandHistory();                    // if you have this helper
}

void MainWindow::applyDarkTheme(bool enabled) {
    if (enabled) {
        qApp->setStyleSheet("QWidget { background-color: #2b2b2b; color: #f0f0f0; } "
                            "QTextEdit, QLineEdit, QListWidget { background-color: #3c3c3c; color: #ffffff; }");
    } else {
        qApp->setStyleSheet("");
    }
    settings.setValue("darkTheme", enabled);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->textOutput && event->type() == QEvent::Wheel) {
        QWheelEvent *wheel = static_cast<QWheelEvent *>(event);
        if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
            int delta = wheel->angleDelta().y() > 0 ? 1 : -1;
            int size = ui->textOutput->font().pointSize() + delta;
            size = qBound(8, size, 32);
            QFont f = ui->textOutput->font();
            f.setPointSize(size);
            ui->textOutput->setFont(f);
            settings.setValue("fontSize", size);
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::clearCommandHistory() {
    commandHistory.clear();
    ui->listHistory->clear();
}

void MainWindow::setStatusLED(const QString &state) {
    QString color;
    QString text;

    if (state == "ready") {
        color = "#888";
        text = "Ready";
    } else if (state == "connected") {
        color = "green";
        text = "Connected";
    } else if (state == "disconnected") {
        color = "red";
        text = "Disconnected";
    } else {
        color = "#000";
        text = "Unknown";
    }

    ui->labelStatusLED->setStyleSheet(
        QString("QLabel { background-color: %1; border-radius: 5px; min-width: 10px; min-height: 10px; }")
            .arg(color)
        );
    ui->labelStatusText->setText(text);
}
