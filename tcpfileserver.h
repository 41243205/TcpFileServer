#ifndef TCPFILESERVER_H
#define TCPFILESERVER_H

#include <QDialog>
#include <QtNetwork>
#include <QtWidgets>

class TcpFileServer : public QDialog
{
    Q_OBJECT

public:
    TcpFileServer(QWidget *parent = nullptr);
    ~TcpFileServer();

public slots:
    void start();                   // 啟動伺服器
    void acceptConnection();        // 接受客戶端連線
    void updateServerProgress();    // 更新伺服器的進度條
    void displayError(QAbstractSocket::SocketError socketError); // 處理網絡錯誤

private:
    // UI 控件
    QProgressBar     *serverProgressBar; // 進度條
    QLabel           *serverStatusLabel; // 狀態標籤
    QPushButton      *startButton;       // 啟動按鈕
    QPushButton      *quitButton;        // 退出按鈕
    QDialogButtonBox *buttonBox;         // 按鈕容器

    QLabel           *ipLabel;           // 顯示 "IP 位址" 標籤
    QLineEdit        *ipLineEdit;        // 用於輸入 IP 位址
    QLabel           *portLabel;         // 顯示 "埠號" 標籤
    QLineEdit        *portLineEdit;      // 用於輸入埠號

    // 網絡相關
    QTcpServer       tcpServer;          // TCP 伺服器
    QTcpSocket       *tcpServerConnection; // 伺服器連線
    qint64           totalBytes;         // 總接收的位元組數
    qint64           byteReceived;       // 已接收的位元組數
    qint64           fileNameSize;       // 檔案名稱的大小
    QString          fileName;           // 檔案名稱
    QFile            *localFile;         // 本地檔案指針
    QByteArray       inBlock;            // 接收的數據塊

    // 用戶輸入的 IP 地址和端口號
    QString          ipAddress;          // 存儲 IP 地址
    int              port;               // 存儲端口號
};

#endif // TCPFILESERVER_H
