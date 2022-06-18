#include "recvthread.h"

#include <QSerialPort>
#include <QSemaphore>

RecvThread::RecvThread(QSerialPort *port)
{
	Q_ASSERT(port != nullptr);
	serialPort = port;
	exited = false;
}

RecvThread::~RecvThread()
{
	this->stop();
	this->quit();
	this->wait();
}

void RecvThread::notifyDataIn()
{
	dataInSem.release();
}

void RecvThread::run()
{
	connect(serialPort, &QSerialPort::readyRead, this, &RecvThread::notifyDataIn);
	while (true)
	{
		dataInSem.acquire();
		QByteArray recvArray = serialPort->readAll();
		if (!recvArray.isEmpty())
		{
			emit dataIn(recvArray);
		}
		if (exited)
		{
			break;
		}
	}
}

void RecvThread::stop()
{
	disconnect(serialPort, &QSerialPort::readyRead, this, &RecvThread::notifyDataIn);
	exited = true;
}

void RecvThread::onPortClosed()
{
	disconnect(serialPort, &QSerialPort::readyRead, this, &RecvThread::notifyDataIn);
}
