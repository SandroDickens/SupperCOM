#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui
{
	class MainWindow;
}
QT_END_NAMESPACE

class RecvThread;

class SendThread;

class SerialPort;


class MainWindow : public QMainWindow
{
Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);

	~MainWindow() override;

private:
	void resFree();

private slots:

	void setFont();

	void setBGColor();

	void setFontColor();

	void openSerialPort();

	void recvData(unsigned long event);

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

	void portClosed();

private:
	std::shared_ptr<SerialPort> serialPort;
	RecvThread *recvThread;
	SendThread *sendThread;
	QTimer *trigTimer;
	QColor fontColor;
	QColor bgColor;
private:
	Ui::MainWindow *ui{};
};

#endif // MAINWINDOW_H
