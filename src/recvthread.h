#ifndef RECVTHREAD_H
#define RECVTHREAD_H

#include <QThread>
#include <QSemaphore>

class SerialPort;

class RecvThread : public QThread
{
Q_OBJECT
public:
	explicit RecvThread(const std::shared_ptr<SerialPort>& port);

	~RecvThread() override;

	void run() override;

	void stop();

private:
	std::shared_ptr<SerialPort> serialPort;
	bool exited;
public slots:

	void onPortClosed();

signals:

	void readyRead(unsigned long event);
};

#endif // RECVTHREAD_H
