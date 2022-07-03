#include "recvthread.h"
#include "serial_port.h"


RecvThread::RecvThread(SerialPort *port)
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


void RecvThread::run()
{
	while (true)
	{
		if (serialPort->waitReadyRead())
		{
			emit readyRead(0);
		}
		if (exited)
		{
			break;
		}
	}
}

void RecvThread::stop()
{
	exited = true;
}

void RecvThread::onPortClosed()
{
	exited = true;
}
