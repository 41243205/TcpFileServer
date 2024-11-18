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
    serverProgressBar->setStyleSheet("QProgressBar { height: 20px; }"); // 設定進度條高度

    serverStatusLabel = new QLabel(tr("伺服器端就緒"));

    QLabel *ipLabel = new QLabel(tr("IP 地址:"));
    ipEdit = new QLineEdit("127.0.0.1"); // 預設 IP 地址
    QLabel *portLabel = new QLabel(tr("端口號:"));
    portEdit = new QSpinBox;
    portEdit->setRange(1, 65535);       // 合法端口範圍
    portEdit->setValue(16998);          // 預設端口號

    // 將 IP 和端口分別設置在上下兩行
    QVBoxLayout *ipPortLayout = new QVBoxLayout;
    ipPortLayout->addWidget(ipLabel);
    ipPortLayout->addWidget(ipEdit);
    ipPortLayout->addWidget(portLabel);
    ipPortLayout->addWidget(portEdit);

    startButton = new QPushButton(tr("接收"));
    quitButton = new QPushButton(tr("退出"));
    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(startButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    // 主佈局
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(ipPortLayout);       // 添加 IP 和端口的垂直佈局
    mainLayout->addWidget(serverProgressBar);  // 添加進度條
    mainLayout->addWidget(serverStatusLabel);  // 添加狀態標籤
    mainLayout->addStretch();                  // 添加空間拉伸
    mainLayout->addWidget(buttonBox);          // 添加按鈕盒
    setLayout(mainLayout);
    setWindowTitle(tr("接收檔案"));

    // 連接信號與槽
    connect(startButton, SIGNAL(clicked()), this, SLOT(start()));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(&tcpServer, SIGNAL(newConnection()), this, SLOT(acceptConnection()));
    connect(&tcpServer, SIGNAL(acceptError(QAbstractSocket::SocketError)), this,
            SLOT(displayError(QAbstractSocket::SocketError)));
}



TcpFileServer::~TcpFileServer()
{
}

void TcpFileServer::start()
{
    startButton->setEnabled(false);
    byteReceived = 0;
    fileNameSize = 0;

    // 使用界面中輸入的 IP 地址和端口號
    ipAddress = ipEdit->text();
    port = portEdit->value();

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
    if (byteReceived <= sizeof(qint64) * 2) {
        if ((fileNameSize == 0) && (tcpServerConnection->bytesAvailable() >= sizeof(qint64) * 2)) {
            in >> totalBytes >> fileNameSize;
            byteReceived += sizeof(qint64) * 2;
        }
        if ((fileNameSize != 0) && (tcpServerConnection->bytesAvailable() >= fileNameSize)) {
            in >> fileName;
            byteReceived += fileNameSize;
            localFile = new QFile(fileName);
            if (!localFile->open(QFile::WriteOnly)) {
                QMessageBox::warning(this, tr("應用程式"),
                                     tr("無法讀取檔案 %1：\n%2.").arg(fileName)
                                         .arg(localFile->errorString()));
                return;
            }
        } else {
            return;
        }
    }

    if (byteReceived < totalBytes) {
        byteReceived += tcpServerConnection->bytesAvailable();
        inBlock = tcpServerConnection->readAll();
        localFile->write(inBlock);
        inBlock.resize(0);
    }

    serverProgressBar->setMaximum(totalBytes);
    serverProgressBar->setValue(byteReceived);
    serverStatusLabel->setText(tr("已接收 %1 Bytes").arg(byteReceived));

    // 文件接收完成後更新狀態
    if (byteReceived == totalBytes) {
        tcpServerConnection->close();
        localFile->close();
        serverStatusLabel->setText(tr("傳送成功")); // 更新狀態為傳送成功
        startButton->setEnabled(true);           // 啟用按鈕
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
