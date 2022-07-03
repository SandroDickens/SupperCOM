#include <mutex>

#include "serial_port.h"

#ifdef __linux__

#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <termios.h>

#endif

std::mutex singletonMutex;

SerialPort *SerialPort::serialPort = nullptr;

SerialPort *SerialPort::getSerialPort()
{
	if (serialPort == nullptr)
	{
		singletonMutex.lock();
		if (serialPort == nullptr)
		{
			serialPort = new SerialPort();
		}
		singletonMutex.unlock();
	}
	return serialPort;
}

SerialPort::~SerialPort()
{
	if (fd != INVALID_FD)
	{
#ifdef _WIN32
		CloseHandle(fd);
#endif
#ifdef __linux__
		close(fd);
#endif
	}
}

SerialPort::SerialPort()
{
	fd = INVALID_FD;
}

bool SerialPort::openSerialPort(int port)
{
	if (fd != INVALID_FD)
	{
		return true;
	}
#ifdef _WIN32
	WCHAR szPort[15] = TEXT("\0");
	wsprintf(szPort, TEXT("COM%d"), port);
	fd = CreateFile(szPort, GENERIC_READ|GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
	                FILE_ATTRIBUTE_NORMAL, nullptr);
	if (fd == INVALID_HANDLE_VALUE)
	{
		printf("CreateFile failed with error %lu.\n", GetLastError());
		return false;
	}
	COMMTIMEOUTS CommTimeOuts;
	CommTimeOuts.ReadIntervalTimeout = MAXDWORD;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = MAXDWORD;
	CommTimeOuts.WriteTotalTimeoutConstant = MAXDWORD;
	SetCommTimeouts(fd, &CommTimeOuts);

	if (TRUE != SetCommMask(fd, EV_RXCHAR|EV_CTS|EV_DSR))
	{
		printf("SetCommMask failed with error %lu.\n", GetLastError());
		return false;
	}

	if (TRUE != SetupComm(fd, 819200, 819200))
	{
		auto error = GetLastError();
		printf("SetupComm failed with error %lu.\n", GetLastError());
		return false;
	}
#endif
#ifdef __linux__
	char ttySerial[12];
	sprintf(ttySerial, "/dev/ttyS%d", port);
	fd = open(ttySerial, O_RDWR|O_CLOEXEC);
	if (fd == -1)
	{
		int errnum = errno;
		printf("Open Serial Port %s failed, %s(%d).\n", ttySerial, strerror(errnum), errnum);
		return false;
	}
#endif
	return true;
}

void SerialPort::closePort()
{

}

int SerialPort::setBaudRate(SerialPort::BaudRate baudRate)
{
#ifdef _WIN32
	DCB dcb;
	dcb.DCBlength = sizeof(DCB);
	GetCommState(fd, &dcb);
	dcb.BaudRate = baudRate;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fNull = FALSE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fBinary = TRUE;

	if (TRUE != SetCommState(fd, &dcb))
	{
		printf("Set baud rate failed with error %lu.\n", GetLastError());
		return -1;
	}
	else
	{
		return 0;
	}
#endif
#ifdef __linux__
	termios options{};
	options.c_cflag = CLOCAL|CREAD;
	tcgetattr(fd, &options);
	cfsetospeed(&options, baudRate);
	cfsetispeed(&options, baudRate);
	return tcsetattr(fd, TCSANOW, &options);
#endif
}

SerialPort::BaudRate SerialPort::getBaudRate()
{
	return SerialPort::UnknownBaud;
}

int SerialPort::setDataBits(SerialPort::DataBits dataBits)
{
#ifdef _WIN32
	DCB dcb;
	dcb.DCBlength = sizeof(DCB);
	GetCommState(fd, &dcb);
	dcb.ByteSize = dataBits;
	if (TRUE != SetCommState(fd, &dcb))
	{
		printf("Set data-bits failed with error %lu.\n", GetLastError());
		return -1;
	}
	else
	{
		return 0;
	}
#endif
#ifdef __linux__
	return 0;
#endif
}

SerialPort::DataBits SerialPort::getDataBits()
{
	return SerialPort::UnknownDataBits;
}

int SerialPort::setStopBits(SerialPort::StopBits stopBits)
{
#ifdef _WIN32
	DCB dcb;
	dcb.DCBlength = sizeof(DCB);
	GetCommState(fd, &dcb);
	dcb.StopBits = stopBits;
	if (TRUE != SetCommState(fd, &dcb))
	{
		printf("Set stop-bits failed with error %lu.\n", GetLastError());
		return -1;
	}
	else
	{
		return 0;
	}
#endif
#ifdef __linux__
	return 0;
#endif
}

SerialPort::StopBits SerialPort::getStopBits()
{
	return SerialPort::OneStop;
}

int SerialPort::setParity(SerialPort::Parity parity)
{
#ifdef _WIN32
	DCB dcb;
	dcb.DCBlength = sizeof(DCB);
	GetCommState(fd, &dcb);
	dcb.Parity = parity;
	if (TRUE != SetCommState(fd, &dcb))
	{
		printf("Set parity failed with error %lu.\n", GetLastError());
		return -1;
	}
	else
	{
		return 0;
	}
#endif
#ifdef __linux__
	return 0;
#endif
}

SerialPort::Parity SerialPort::getParity()
{
	return SerialPort::OddParity;
}

int SerialPort::setFlowControl(SerialPort::FlowControl flowControl)
{
#ifdef _WIN32
#endif
#ifdef __linux__
#endif
	return 0;
}

SerialPort::FlowControl SerialPort::getFlowControl()
{
	return SerialPort::UnknownFlowControl;
}

unsigned long SerialPort::read(void *buffer, unsigned long size)
{
	if (fd == INVALID_FD)
	{
		printf("Invalid serial port fd\n");
		return 0;
	}
#ifdef _WIN32
	DWORD dwBytesToRead = size, dwBytesRead = 0;
	if (FALSE == ReadFile(fd, buffer, dwBytesToRead, &dwBytesRead, nullptr))
	{
		dwBytesRead = 0;
	}
	return dwBytesRead;
#endif
#ifdef __linux__
	ssize_t len = read(fd, buffer, size);
	if (len == -1)
	{
		int errnum = errno;
		printf("Read from SerialPort failed, %s(%d).\n", strerror(errnum), errnum);
		len = 0;
	}
	return len;
#endif
}

unsigned long SerialPort::write(const char *buffer, unsigned long size)
{
	if (fd == INVALID_FD)
	{
		printf("Invalid serial port fd\n");
		return 0;
	}
#ifdef _WIN32
	DWORD dwErrorFlags = CE_BREAK|CE_FRAME|CE_OVERRUN|CE_RXOVER|CE_RXPARITY;
	COMSTAT ComStat;

	//ClearCommError(fd, &dwErrorFlags, nullptr);
	unsigned long bytesWritten = 0;
	if (TRUE != WriteFile(fd, buffer, size, &bytesWritten, nullptr))
	{
		bytesWritten = 0;
	}
	return bytesWritten;
#endif
#ifdef __linux__
	ssize_t len = write(fd, buffer, size);
	if (len == -1)
	{
		int errnum = errno;
		printf("Write to SerialPort failed, %s(%d).\n", strerror(errnum), errnum);
		len = 0;
	}
	return len;
#endif
}

bool SerialPort::isOpen() const
{
	return fd != INVALID_FD;
}

bool SerialPort::waitReadyRead()
{
	unsigned long dwEventMask;
	if (TRUE == WaitCommEvent(fd, &dwEventMask, nullptr))
	{
		if ((dwEventMask&EV_RXCHAR) != 0)
		{
			return true;
		}
	}
	return false;
}
