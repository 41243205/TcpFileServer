#ifndef TCPFILESENDER_H
#define TCPFILESENDER_H

#include <QDialog>
#include <QtNetwork>
#include <QtWidgets>

class TcpFileSender : public QDialog
{
    Q_OBJECT

public:
    TcpFileSender(QWidget *parent = 0);
    ~TcpFileSender();
public slots:
    void start();
    void startTransfer();
    void updateClientProgress(qint64 numBytes);
    void openFile();

private:
    QProgressBar     *clientProgressBar; // 用於顯示傳輸進度
    QLabel           *clientStatusLabel; // 顯示狀態
    QLabel           *ipLabel;           // 顯示 "IP 位址" 標籤
    QLineEdit        *ipLineEdit;        // 用於輸入 IP 位址
    QLabel           *portLabel;         // 顯示 "埠號" 標籤
    QLineEdit        *portLineEdit;      // 用於輸入埠號
    QPushButton      *startButton;       // 開始按鈕
    QPushButton      *quitButton;        // 退出按鈕
    QPushButton      *openButton;        // 開檔按鈕
    QDialogButtonBox *buttonBox;         // 按鈕盒
    QTcpSocket       tcpClient;          // TCP 連接客戶端

    qint64           totalBytes;         // 總傳輸大小
    qint64           bytesWritten;       // 已寫入大小
    qint64           bytesToWrite;       // 剩餘待寫大小
    qint64           loadSize;           // 單次讀取大小
    QString          fileName;           // 檔案名稱
    QFile            *localFile;         // 檔案操作物件
    QByteArray       outBlock;           // 傳輸緩衝區
};

#endif // TCPFILESENDER_H
