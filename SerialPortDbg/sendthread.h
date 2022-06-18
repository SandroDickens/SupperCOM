#ifndef SENDTHREAD_H
#define SENDTHREAD_H

#include <QThread>

class QSerialPort;

class QReadWriteLock;

constexpr qint64 OPEN_FILE_ERROR = -1;
constexpr qint64 SEND_FILE_ERROR = -2;
constexpr qint64 SEND_FILE_SUCCESS = 0;

class SendThread : public QThread
{
Q_OBJECT
public:
	SendThread(QSerialPort *port);

	~SendThread() override;

	void run() override;

	void stop();

private:
	QSerialPort *serialPort;
	QReadWriteLock *rwLock;
	QString sendFileName;
	bool exited;
public slots:

	void startSendFile(const QString &fileName);

signals:

	void sendFinished(qint64 retVal);
};

#endif // SENDTHREAD_H
