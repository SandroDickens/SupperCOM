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
private:
	void resFree(void);
private slots:
   void setFont(void);
   void setBGColor(void);
   void setFontColor(void);
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
   /* Re-detect the serial port */
   void redetectPort(void);
   void timerChanged(int state);
   void trigSend(void);
signals:
   void startSendFile(const QString &fileName);
private:
   QSerialPort* serialPort;
   RecvThread* recvThread;
   SendThread* sendThread;
   QTimer* trigTimer;
   QColor fontColor;
   QColor bgColor;
private:
    Ui::MainWidget *ui;
};
#endif // MAINWIDGET_H
