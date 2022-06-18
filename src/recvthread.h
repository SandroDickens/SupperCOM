#ifndef RECVTHREAD_H
#define RECVTHREAD_H

#include <QThread>
#include <QSemaphore>

class QSerialPort;

class RecvThread : public QThread
{
Q_OBJECT
public:
	explicit RecvThread(QSerialPort *port);

	~RecvThread() override;

	void run() override;

	void stop();

private:
	QSerialPort *serialPort;
	QSemaphore dataInSem;
	bool exited;
public slots:
	void onPortClosed();
	void notifyDataIn();

signals:

	void dataIn(const QByteArray &msgArray);
};

#endif // RECVTHREAD_H
