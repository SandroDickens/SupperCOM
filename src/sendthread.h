#ifndef SENDTHREAD_H
#define SENDTHREAD_H

#include "serial_port.h"

#include <QThread>

constexpr qint64 OPEN_FILE_ERROR = -1;
constexpr qint64 SEND_FILE_ERROR = -2;
constexpr qint64 SEND_FILE_SUCCESS = 0;

class SendThread : public QThread
{
Q_OBJECT
public:
	explicit SendThread(SerialPort *port);

	~SendThread() override;

	void run() override;

	void stop();

private:
	SerialPort *serialPort;
	QString sendFileName;
	bool exited;
public slots:

	void startSendFile(const QString &fileName);

signals:

	void sendFinished(qint64 retVal);
};

#endif // SENDTHREAD_H
