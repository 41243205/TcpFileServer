#include "tcpfileserver.h"

#define tr QStringLiteral

TcpFileServer::TcpFileServer(QWidget *parent)
    : QDialog(parent)
{
    totalBytes = 0;
    byteReceived = 0;
    fileNameSize = 0;

    // 初始化 UI 控件
    serverProgressBar = new QProgressBar;
    serverStatusLabel = new QLabel(tr("伺服器端就緒"));

    // 新增 IP 和 Port 輸入框
    ipLabel = new QLabel(QStringLiteral("IP 地址："));
    ipLineEdit = new QLineEdit("127.0.0.1");
    portLabel = new QLabel(QStringLiteral("端口號："));
    portLineEdit = new QLineEdit("16998");

    startButton = new QPushButton(tr("接收"));
    quitButton = new QPushButton(tr("退出"));
    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(startButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    // 主佈局
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(serverProgressBar);  // 添加進度條
    mainLayout->addWidget(serverStatusLabel);  // 添加狀態標籤

    // 新增 IP 和 Port 到界面
    QHBoxLayout *ipLayout = new QHBoxLayout;
    ipLayout->addWidget(ipLabel);
    ipLayout->addWidget(ipLineEdit);

    QHBoxLayout *portLayout = new QHBoxLayout;
    portLayout->addWidget(portLabel);
    portLayout->addWidget(portLineEdit);

    mainLayout->addLayout(ipLayout);
    mainLayout->addLayout(portLayout);

    mainLayout->addStretch(1);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
    setWindowTitle(tr("接收檔案"));

    // 連接信號與槽
    connect(startButton, SIGNAL(clicked()), this, SLOT(start()));
    connect(quitButton, &QPushButton::clicked, qApp, &QApplication::quit);
    connect(&tcpServer, SIGNAL(newConnection()), this, SLOT(acceptConnection()));
    connect(&tcpServer, SIGNAL(acceptError(QAbstractSocket::SocketError)), this,
            SLOT(displayError(QAbstractSocket::SocketError)));
}



TcpFileServer::~TcpFileServer()
{
    if (tcpServer.isListening()) {
        tcpServer.close(); // 關閉伺服器
    }

    if (tcpServerConnection) {
        tcpServerConnection->close(); // 關閉客戶端連線
        tcpServerConnection->deleteLater();
        tcpServerConnection = nullptr;
    }

    if (localFile) {
        localFile->close(); // 關閉文件
        delete localFile;
        localFile = nullptr;
    }
}

void TcpFileServer::start()
{
    startButton->setEnabled(false);
    byteReceived = 0;
    fileNameSize = 0;

    // 使用界面中輸入的 IP 地址和端口號
    ipAddress = ipLineEdit->text();
    port = portLineEdit->text().toInt();


    // 啟動伺服器
    while (!tcpServer.isListening() &&
           !tcpServer.listen(QHostAddress(ipAddress), port)) {
        QMessageBox::StandardButton ret = QMessageBox::critical(this,
                                                                tr("錯誤"),
                                                                tr("無法啟動伺服器: %1.").arg(tcpServer.errorString()),
                                                                QMessageBox::Retry | QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel) {
            startButton->setEnabled(true);
            return;
        }
    }
    serverStatusLabel->setText(tr("監聽中..."));
}

void TcpFileServer::acceptConnection()
{
    tcpServerConnection = tcpServer.nextPendingConnection(); // 取得已接受的客戶端連線
    connect(tcpServerConnection, SIGNAL(readyRead()),
            this, SLOT(updateServerProgress())); // 連接readyRead()訊號至updateServerProgress()槽函數
    connect(tcpServerConnection, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError))); // 連接error()訊號至displayError()槽函數
    serverStatusLabel->setText(tr("接受連線"));
    tcpServer.close(); // 暫停接受客戶端連線
}

void TcpFileServer::updateServerProgress()
{
    QDataStream in(tcpServerConnection);
    in.setVersion(QDataStream::Qt_4_6);

    // 檢查是否正在接收 `totalBytes` 和 `fileNameSize`
    if (byteReceived <= sizeof(qint64) * 2) {
        if (tcpServerConnection->bytesAvailable() >= sizeof(qint64) * 2 && fileNameSize == 0) {
            // 接收總大小和文件名大小
            in >> totalBytes >> fileNameSize;
            byteReceived += sizeof(qint64) * 2;

            // 驗證數據合法性
            if (totalBytes <= 0 || fileNameSize <= 0) {
                QMessageBox::critical(this, tr("錯誤"), tr("接收的檔案大小或名稱無效！"));
                tcpServerConnection->close();
                return;
            }
        }

        if (tcpServerConnection->bytesAvailable() >= fileNameSize && fileNameSize != 0) {
            // 接收文件名
            in >> fileName;
            byteReceived += fileNameSize;

            // 嘗試打開文件進行寫入
            localFile = new QFile(fileName);
            if (!localFile->open(QFile::WriteOnly)) {
                QMessageBox::warning(this, tr("應用程式"),
                                     tr("無法打開檔案 %1：\n%2.").arg(fileName)
                                         .arg(localFile->errorString()));
                tcpServerConnection->close();
                delete localFile;
                localFile = nullptr;
                return;
            }
        } else {
            return; // 等待更多數據到達
        }
    }

    // 處理文件數據接收
    if (byteReceived < totalBytes) {
        qint64 bytesToRead = tcpServerConnection->bytesAvailable();
        byteReceived += bytesToRead;

        inBlock = tcpServerConnection->read(bytesToRead);
        if (localFile) {
            localFile->write(inBlock);
        }
        inBlock.clear();
    }

    // 更新進度條
    serverProgressBar->setMaximum(totalBytes);
    serverProgressBar->setValue(byteReceived);

    serverStatusLabel->setText(tr("已接收 %1 Bytes").arg(byteReceived));

    // 檢查是否接收完成
    if (byteReceived >= totalBytes) {
        if (localFile) {
            localFile->close();
            delete localFile;
            localFile = nullptr;
        }
        tcpServerConnection->close();

        serverStatusLabel->setText(tr("接收成功"));
        startButton->setEnabled(true);
    }
}



void TcpFileServer::displayError(QAbstractSocket::SocketError socketError)
{
    QObject *server = qobject_cast<QObject *>(sender());
    if (server == tcpServerConnection) qDebug() << "Hi I am QTcpSocket";
    if (server == &tcpServer) qDebug() << "Hi I am QTcpServer";

    QMessageBox::information(this, tr("網絡錯誤"),
                             tr("產生如下錯誤: %1.")
                                 .arg(tcpServerConnection->errorString()));
    tcpServerConnection->close();
    serverProgressBar->reset();
    serverStatusLabel->setText(tr("伺服器就緒"));
    startButton->setEnabled(true);
}
