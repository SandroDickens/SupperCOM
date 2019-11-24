#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWidget; }
QT_END_NAMESPACE

class QSerialPort;
class RecvThread;

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
private:
   QSerialPort* serialPort;
   RecvThread* recvThread;
private:
    Ui::MainWidget *ui;
};
#endif // MAINWIDGET_H
