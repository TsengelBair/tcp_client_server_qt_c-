#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "structs.h"

#include <QMainWindow>
#include <QTcpSocket>

class QPushButton;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void sendToServer(const MessageType::RequestType &requestType);
    void slotHandleServerResponse();
    void slotProccesBuffer();
    void slotSendCrcError();
    void slotHandleRegisterResponse(const QByteArray &packetData);
    void slotHandleAuthResponse(const QByteArray &packetData);

private:
    QPair<QString, QString> getDataFromUI();

private:
    Ui::MainWindow *ui;
    QTcpSocket* m_socket;
    QByteArray m_buffer;
};
#endif // MAINWINDOW_H
