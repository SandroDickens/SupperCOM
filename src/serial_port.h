#ifndef _SERIAL_PORT_H_
#define _SERIAL_PORT_H_

#include <string>
#include <memory>

#ifdef _WIN32

#include <Windows.h>

#define INVALID_FD    INVALID_HANDLE_VALUE
typedef HANDLE SERIAL_PORT_FD;
#endif
#ifdef __linux__

#include <termios.h>

#define INVALID_FD    (-1)
typedef int SERIAL_PORT_FD;
#endif

class SerialPort
{
public:
	enum DataBits
	{
		Data5 = 5, Data6 = 6, Data7 = 7, Data8 = 8, UnknownDataBits = -1
	};

#ifdef _WIN32
	enum BaudRate
	{
		Baud1200 = 1200,
		Baud2400 = 2400,
		Baud4800 = 4800,
		Baud9600 = 9600,
		Baud19200 = 19200,
		Baud38400 = 38400,
		Baud57600 = 57600,
		Baud115200 = 115200,
		UnknownBaud = -1
	};

	enum StopBits
	{
		OneStop = ONESTOPBIT, OneAndHalfStop = ONE5STOPBITS, TwoStop = TWOSTOPBITS, UnknownStopBits = -1
	};

	enum Parity
	{
		NoParity = NOPARITY,
		EvenParity = EVENPARITY,
		OddParity = ODDPARITY,
		SpaceParity = SPACEPARITY,
		MarkParity = MARKPARITY,
		UnknownParity = -1
	};

	enum FlowControl
	{
		NoFlowControl, HardwareControl, SoftwareControl, UnknownFlowControl = -1
	};
#else
	enum BaudRate
	{
		Baud1200 = B1200,
		Baud2400 = B2400,
		Baud4800 = B4800,
		Baud9600 = B9600,
		Baud19200 = B19200,
		Baud38400 = B38400,
		Baud57600 = B57600,
		Baud115200 = B115200,
		UnknownBaud = -1
	};

	enum StopBits
	{
		OneStop = 1, OneAndHalfStop = 3, TwoStop = 2, UnknownStopBits = -1
	};

	enum Parity
	{
		NoParity = 0, EvenParity = 2, OddParity = 3, SpaceParity = 4, MarkParity = 5, UnknownParity = -1
	};

	enum FlowControl
	{
		NoFlowControl, HardwareControl, SoftwareControl, UnknownFlowControl = -1
	};
#endif

public:
	virtual ~SerialPort();

	static std::shared_ptr<SerialPort> getSerialPort();

	/* Open SerialPort */
	bool openSerialPort(const std::string &port);

	/* Close SerialPort */
	void closePort();

	[[nodiscard]] int setBaudRate(SerialPort::BaudRate baudRate) const;

	[[nodiscard]] int setDataBits(SerialPort::DataBits dataBits) const;

	[[nodiscard]] int setStopBits(SerialPort::StopBits stopBits) const;

	[[nodiscard]] int setParity(SerialPort::Parity parity) const;

	/* Read data from SerialPort */
	unsigned long recv(void *buffer, unsigned long size) const;

	/* Send data through SerialPort */
	unsigned long send(const char *buffer, unsigned long size) const;

	[[nodiscard]] bool isOpen() const;

	bool waitReadyRead();

	[[nodiscard]] SERIAL_PORT_FD getNativeHandle() const;

private:
	SerialPort();

	SERIAL_PORT_FD fd;
	static std::shared_ptr<SerialPort> serialPort;
};

#endif