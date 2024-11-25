#include "tcpfileserver.h"
#include "tcpfilesender.h"
#include <QApplication>
#include "QTabWidget"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    TcpFileSender *w1 = new TcpFileSender;
    TcpFileServer *w2 = new TcpFileServer;

    QTabWidget *s = new QTabWidget;
    s->addTab(w1, "客戶端");
    s->addTab(w2, "伺服端");

    // 設置主視窗屬性
    s->setAttribute(Qt::WA_DeleteOnClose);

    // 確保視窗關閉時退出程式
    QObject::connect(s, &QTabWidget::destroyed, &a, &QApplication::quit);

    s->show();
    return a.exec();
}
