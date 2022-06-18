#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QSerialPort>

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
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget() override;
private:
	void resFree();
private slots:
   void setFont();
   void setBGColor();
   void setFontColor();
   void openSerialPort();
   void recvData(const QByteArray &msgArray);
   void portErrorProc(QSerialPort::SerialPortError error);
   void sendData();
   void cleanDisplay();
   void cleanSend();
   void stopSend();
   void openFile();
   void sendFile();
   void sendOver(qint64 retVal);
   //void redetectPort(void);
   void timerChanged(int state);
   void trigSend();
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
