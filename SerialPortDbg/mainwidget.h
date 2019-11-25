#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWidget; }
QT_END_NAMESPACE

class QSerialPort;
class RecvThread;
class SendThread;

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();
private slots:
   void showFontDlg(void);
   void showColorDlg(void);
   void openSerialPort(void);
   void recvData(const QByteArray &msgArray);
   void portDisconnect(void);
   void sendData(void);
   void cleanDisplay(void);
   void cleanSend(void);
   void stopSend(void);
   void openFile(void);
   void sendFile(void);
   void sendOver(qint64 retVal);
signals:
   void startSendFile(const QString &fileName);
private:
   QSerialPort* serialPort;
   RecvThread* recvThread;
   SendThread* sendThread;
private:
    Ui::MainWidget *ui;
};
#endif // MAINWIDGET_H
