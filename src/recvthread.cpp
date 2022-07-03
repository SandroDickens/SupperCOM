#include "recvthread.h"
#include "serial_port.h"

#ifdef __linux__

#include <sys/epoll.h>

#endif

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
#ifdef _WIN32
	while (!exited)
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
#endif
#ifdef __linux__
	int epl_fd = epoll_create(1);
	if (epl_fd < 0)
	{
		perror("epoll create failed");
		exit(errno);
	}

	epoll_event event{};
	event.events = EPOLLIN;
	event.data.ptr = nullptr;

	epoll_ctl(epl_fd, EPOLL_CTL_ADD, serialPort->getNativeHandle(), &event);

	epoll_event events{};
	int timeout = 500;
	while (!exited)
	{
		int wait_ret = epoll_wait(epl_fd, &events, 1, timeout);
		if ((wait_ret > 0) && (events.events & EPOLLIN))
		{
			emit readyRead(0);
		}
		else if (wait_ret == -1)
		{
			break;
		}
	}
#endif
}

void RecvThread::stop()
{
	exited = true;
}

void RecvThread::onPortClosed()
{
	exited = true;
}
