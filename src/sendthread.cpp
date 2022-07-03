#include "sendthread.h"

/* C plusplus header file */
#include <memory>

/* Qt head file */
#include <QFile>
#include <QMessageBox>

#include "serial_port.h"

static const int MAX_WRITE_LEN = 4096;

SendThread::SendThread(SerialPort *port)
{
	Q_ASSERT(port != nullptr);
	serialPort = port;
	exited = false;
}

SendThread::~SendThread()
{
	this->stop();
	this->quit();
	this->wait();
}

void SendThread::startSendFile(const QString &fileName)
{
	if (fileName.length() != 0)
	{
		sendFileName = fileName;
		this->start();
	}
}

void SendThread::run()
{
	QFile file(sendFileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		emit sendFinished(OPEN_FILE_ERROR);
	}
	else
	{
		qint64 sendCount = 0;
		std::unique_ptr<char> sendBuf(new char[MAX_WRITE_LEN]);
		qint64 wLen;
		while (!file.atEnd())
		{
			memset(sendBuf.get(), 0, MAX_WRITE_LEN);
			qint64 readLen = file.read(sendBuf.get(), 4096);
			if (readLen > 0)
			{
				wLen = serialPort->send(sendBuf.get(), readLen);
				if (wLen != -1)
				{
					sendCount += wLen;
				}
			}
			if (exited)
			{
				break;
			}
		}
		file.close();
		if (sendCount == 0)
		{
			emit sendFinished(SEND_FILE_ERROR);
		}
		else if (sendCount == file.size())
		{
			emit sendFinished(SEND_FILE_SUCCESS);
		}
		else
		{
			emit sendFinished(sendCount);
		}
	}
}

void SendThread::stop()
{
	exited = true;
}
