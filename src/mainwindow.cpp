#include "mainwindow.h"
#include "./ui_mainwindow.h"

/* user header files */
#include "recvthread.h"
#include "sendthread.h"

/* c++ header files */
#include <algorithm>

/* Qt header files */
#include <QFontDialog>
#include <QColorDialog>
#include <QtSerialPort/QSerialPortInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>
#include <QTextCodec>


#ifdef _WIN32

#include <Windows.h>
#include <iostream>

#elif __linux__

#include <unistd.h>
#include <cerrno>
#include <iostream>

#else
#error Unsupported operating system
#endif // _WIN32

#include "serial_port.h"

static bool compareQString(const QString &str0, const QString &str1)
{
	return 0 == QString::compare(str1, str0);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	serialPort = nullptr;
	recvThread = nullptr;
	sendThread = nullptr;
	trigTimer = nullptr;
	/* list serialport */
	QList<QSerialPortInfo> serialPortList = QSerialPortInfo::availablePorts();
	if (serialPortList.empty())
	{
		ui->serialPortSel->addItem(QString(tr("N/A")));
	}
	else
	{
		QStringList portNameList;
		for (const QSerialPortInfo &info: serialPortList)
		{
			portNameList.append(info.portName());
		}
		std::sort(portNameList.begin(), portNameList.end(), compareQString);
		for (const QString &portName: portNameList)
		{
			ui->serialPortSel->addItem(portName);
		}
	}

	/* set baud rate */
	int baudRate[] = {9600, 14400, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
	for (int rate: baudRate)
	{
		ui->baudRateSel->addItem(QString::number(rate));
	}
	ui->baudRateSel->setCurrentIndex(5);
	/* set data bits */
	int dataBits[] = {5, 6, 7, 8};
	for (int bits: dataBits)
	{
		ui->dataBitSel->addItem(QString::number(bits));
	}
	ui->dataBitSel->setCurrentIndex(3);
	/* set stop bits */
	int stopBits[] = {1, 2};
	for (int bits: stopBits)
	{
		ui->stopBitSel->addItem(QString::number(bits));
	}
	ui->stopBitSel->setCurrentIndex(0);
	/* set check type */
	ui->checkBitSel->addItem(QString(tr("NO")));
	ui->checkBitSel->addItem(QString(tr("Odd")));
	ui->checkBitSel->addItem(QString(tr("Even")));
	ui->checkBitSel->setCurrentIndex(0);

	/* Default foreground and background colors */
	fontColor = QColor(0x00, 0xff, 0x00);
	bgColor = QColor(0x00, 0x00, 0x00);
	ui->recvTextBrowser->setStyleSheet(QString(tr("color:green;background-color:white;")));
	QFont font;
	font.setFamily(QString::fromUtf8("DejaVu Sans Mono"));
	font.setPointSize(11);
	ui->recvTextBrowser->setFont(font);
	ui->recvTextBrowser->setReadOnly(true);
	ui->recvTextBrowser->setLineWrapMode(QTextBrowser::LineWrapMode::WidgetWidth);

	ui->sendInterval->setValidator(new QIntValidator(1, 30));
	connect(ui->setFontBtn, SIGNAL(clicked()), this, SLOT(setFont()));
	connect(ui->setBGColorBtn, SIGNAL(clicked()), this, SLOT(setBGColor()));
	connect(ui->setFontColorBtn, SIGNAL(clicked()), this, SLOT(setFontColor()));
	connect(ui->openSerialPortBtn, SIGNAL(clicked()), this, SLOT(openSerialPort()));
	connect(ui->sendBtn, SIGNAL(clicked()), this, SLOT(sendData()));
	connect(ui->cleanDisplayArea, SIGNAL(clicked()), this, SLOT(cleanDisplay()));
	connect(ui->cleanSendArea, SIGNAL(clicked()), this, SLOT(cleanSend()));
	connect(ui->stopSendBtn, SIGNAL(clicked()), this, SLOT(stopSend()));
	connect(ui->openFileBtn, SIGNAL(clicked()), this, SLOT(openFile()));
	connect(ui->sendFileBtn, SIGNAL(clicked()), this, SLOT(sendFile()));
	//connect(ui->serialPortSel, SIGNAL(clicked()), this, SLOT(redetectPort()));
	connect(ui->schedSend, &QCheckBox::stateChanged, this, &MainWindow::timerChanged);
}

MainWindow::~MainWindow()
{
	delete ui;
}

inline void MainWindow::resFree()
{
	if (recvThread != nullptr)
	{
		disconnect(this, &MainWindow::portClosed, recvThread, &RecvThread::onPortClosed);
		disconnect(recvThread, &RecvThread::readyRead, this, &MainWindow::recvData);
		recvThread->stop();
		recvThread->quit();
		recvThread->terminate();
		recvThread->wait();
		delete recvThread;
		recvThread = nullptr;
	}

	if (sendThread != nullptr)
	{
		disconnect(this, &MainWindow::startSendFile, sendThread, &SendThread::startSendFile);
		disconnect(sendThread, &SendThread::sendFinished, this, &MainWindow::sendOver);
		sendThread->stop();
		sendThread->quit();
		sendThread->terminate();
		sendThread->wait();
		delete sendThread;
		sendThread = nullptr;
	}
	if (trigTimer != nullptr)
	{
		disconnect(trigTimer, SIGNAL(timeout()), this, SLOT(trigSend()));
		trigTimer->stop();
		delete trigTimer;
		trigTimer = nullptr;
	}
	if (serialPort != nullptr)
	{
		if (serialPort->isOpen())
		{
			serialPort->closePort();
		}
		delete serialPort;
		serialPort = nullptr;
	}
}

void MainWindow::setFont()
{
	bool ok;
	QFont font = QFontDialog::getFont(&ok);
	if (ok)
	{
		QTextBrowser *textBrowser = ui->recvTextBrowser;
		textBrowser->setFont(font);
	}
}

void MainWindow::setBGColor()
{
	QColor newColor = QColorDialog::getColor();
	if (newColor.isValid())
	{
		QTextBrowser *textBrowser = ui->recvTextBrowser;
		bgColor = newColor;
		QString colorStye =
				QString(tr("color:")) + fontColor.name() + QString(tr(";background-color:")) + bgColor.name() +
				QString(tr(";"));
		textBrowser->setStyleSheet(colorStye);
	}
}

void MainWindow::setFontColor()
{
	QColor newColor = QColorDialog::getColor();
	if (newColor.isValid())
	{
		QTextBrowser *textBrowser = ui->recvTextBrowser;
		fontColor = newColor;
		QString colorStye =
				QString(tr("color:")) + fontColor.name() + QString(tr(";background-color:")) + bgColor.name() +
				QString(tr(";"));
		textBrowser->setStyleSheet(colorStye);
	}
}

void MainWindow::openSerialPort()
{
	QString portName = ui->serialPortSel->currentText();
	QString baudRateStr = ui->baudRateSel->currentText();
	QString dataBits = ui->dataBitSel->currentText();
	QString stopBitsStr = ui->stopBitSel->currentText();
	int chkBits = ui->checkBitSel->currentIndex();

	if (serialPort != nullptr)
	{
		/* turn off serialport */
		if (serialPort->isOpen())
		{
			serialPort->closePort();
		}
		resFree();
		ui->openSerialPortBtn->setText(QString(tr("打开串口")));
	}
	else
	{
		/* turn on serialport */
		serialPort = SerialPort::getSerialPort();
		if (serialPort != nullptr)
		{
#ifdef _WIN32
			//COMxxx
			std::string devName(portName.toLatin1().constData());
#endif
#ifdef __linux__
			std::string name(portName.toLocal8Bit().constData());
			char devName[32];
			sprintf(devName, "/dev/%s", name.c_str());
#endif
			if (serialPort->openSerialPort(devName))
			{
				ui->openSerialPortBtn->setText(QString(tr("关闭串口")));
				recvThread = new RecvThread(serialPort);
				connect(recvThread, &RecvThread::readyRead, this, &MainWindow::recvData, Qt::BlockingQueuedConnection);
				for (int i = 0; i < 3; ++i)
				{
					if (serialPort->isOpen())
					{
						break;
					}
					else
					{
#ifdef _WIN32
						Sleep(1000);
#elif __linux__
						sleep(1);
#else
#error Unsupported operating system
#endif
						qDebug() << QString(tr("Wait for serial port to open ...")) << i << QString(tr("s\n"));
					}
				}
				if (!serialPort->isOpen())
				{
					QMessageBox msgBox;
					msgBox.setWindowTitle(QString(tr("ERROR")));
					msgBox.setText(QString(tr("failed to open serialport!")) + portName);
					msgBox.setStandardButtons(QMessageBox::Ok);
					ui->openSerialPortBtn->click();
				}
				else
				{
					int rate = baudRateStr.toInt();
					SerialPort::BaudRate baudRate = SerialPort::UnknownBaud;
					switch (rate)
					{
						case 1200:
							baudRate = SerialPort::Baud1200;
							break;
						case 2400:
							baudRate = SerialPort::Baud2400;
							break;
						case 4800:
							baudRate = SerialPort::Baud4800;
							break;
						case 9600:
							baudRate = SerialPort::Baud9600;
							break;
						case 19200:
							baudRate = SerialPort::Baud19200;
							break;
						case 38400:
							baudRate = SerialPort::Baud38400;
							break;
						case 57600:
							baudRate = SerialPort::Baud57600;
							break;
						case 115200:
							baudRate = SerialPort::Baud115200;
							break;
						default:
							break;
					}
					if (-1 == serialPort->setBaudRate(baudRate))
					{
						qDebug() << QString(tr("Set serial port baud rate failed!\n"));
						return;
					}
					if (-1 == serialPort->setDataBits(SerialPort::DataBits(dataBits.toInt())))
					{
						qDebug() << QString(tr("Set serial port data-bits failed!\n"));
						return;
					}
					SerialPort::StopBits stopBits;
					if (stopBitsStr == "1")
					{
						stopBits = SerialPort::StopBits::OneStop;
					}
					else if (stopBitsStr == "1.5")
					{
						stopBits = SerialPort::StopBits::OneAndHalfStop;
					}
					else if (stopBitsStr == "2")
					{
						stopBits = SerialPort::StopBits::TwoStop;
					}
					else
					{
						stopBits = SerialPort::StopBits::UnknownStopBits;
					}
					if (-1 == serialPort->setStopBits(SerialPort::StopBits(stopBits)))
					{
						qDebug() << QString(tr("Set serial port stop-bits failed!\n"));
						return;
					}

					SerialPort::Parity parity;
					switch (chkBits)
					{
						case 0:
							parity = SerialPort::Parity::NoParity;
							break;
						case 1:
							parity = SerialPort::Parity::OddParity;
							break;
						case 2:
							parity = SerialPort::Parity::EvenParity;
							break;
						default:
							parity = SerialPort::Parity::NoParity;
							break;
					}

					if (-1 == serialPort->setParity(parity))
					{
						qDebug() << QString(tr("Set serial port parity failed!\n"));
						return;
					}
					recvThread->start();
				}
			}
			else
			{
				qDebug() << QString(tr("can NOT open serialport ")) << portName << QString(tr("\n"));
				QMessageBox msgBox;
				msgBox.setWindowTitle(QString(tr("ERROR")));
#ifdef __linux__
				int tmp_errno = errno;
				msgBox.setText(QString(tr("can not open serialport!%1:%2")).arg(tmp_errno).arg(strerror(tmp_errno)));
#else
				msgBox.setText(QString(tr("can not open serialport!")));
#endif
				resFree();
				msgBox.setStandardButtons(QMessageBox::Ok);
				msgBox.exec();
			}
		}
		else
		{
			qDebug() << QString(tr("can NOT create serialport.\n"));
		}
	}
}

void MainWindow::recvData(unsigned long event)
{
	if (serialPort != nullptr)
	{
		char buffer[4096];
		auto read_size = serialPort->recv(buffer, sizeof(buffer));
		if (read_size == 0)
		{
			return;
		}
		buffer[read_size] = '\0';
#ifdef QT_DEBUG
		std::cout << buffer << std::flush;
#endif
		QTextBrowser *textBrowser = ui->recvTextBrowser;
		if (ui->hexDisplay->checkState() != Qt::Unchecked)
		{
			for (char i: buffer)
			{
				QString ch = QString::number(i, 10);
				textBrowser->insertPlainText(ch.toUpper());
				textBrowser->moveCursor(textBrowser->textCursor().End);
			}
		}
		else
		{
#ifdef _WIN32
			QTextCodec *textCodec = QTextCodec::codecForName("UTF-8");
			QString text = textCodec->toUnicode(buffer);
#endif
#ifdef __linux__
			QString text = QString::fromUtf8(buffer, read_size);
#endif
			textBrowser->insertPlainText(text);
			textBrowser->moveCursor(textBrowser->textCursor().End);
		}
	}
}

void MainWindow::sendData()
{
	if (serialPort != nullptr)
	{
		QTextEdit *sendTextEdit = ui->sendTextEdit;
		QTextCodec *textCodec = QTextCodec::codecForName("UTF-8");
		QString sendMsg = sendTextEdit->toPlainText();
		QByteArray sendByteArray;
		if (Qt::Unchecked != ui->hexSend->checkState())
		{
			sendByteArray = QByteArray::fromHex(textCodec->fromUnicode(sendMsg));
		}
		else
		{
			sendByteArray = textCodec->fromUnicode(sendMsg);
		}
		if (Qt::Unchecked != ui->sendNewLine->checkState())
		{
#ifdef _WIN32
			sendByteArray.append(0x0d);
			sendByteArray.append(0x0a);
#elif __linux__
			sendByteArray.append(0x0a);
#else
#error Unsupported operating system
#endif
		}
		const char *buff = sendByteArray.constData();
		unsigned long size = sendByteArray.size();
		serialPort->send(buff, size);
	}
}

void MainWindow::cleanDisplay()
{
	QTextBrowser *textBrowser = ui->recvTextBrowser;
	textBrowser->clear();
}

void MainWindow::cleanSend()
{
	QTextEdit *textEdit = ui->sendTextEdit;
	textEdit->clear();
}

void MainWindow::stopSend()
{
	if (trigTimer != nullptr)
	{
		trigTimer->stop();
		delete trigTimer;
		trigTimer = nullptr;
	}
	if (sendThread != nullptr)
	{
		sendThread->stop();
		sendThread->quit();
		sendThread->wait();
		delete sendThread;
		sendThread = nullptr;
	}
}

void MainWindow::openFile()
{
	QString sendFileName = QFileDialog::getOpenFileName(this, QString(tr("打开文件")), QString(tr("%USERPROFILE%/")),
	                                                    QString(tr("任意文件(*.*)")));
	ui->sendFilePath->setText(sendFileName);
}

void MainWindow::sendFile()
{
	QString fileName = ui->sendFilePath->text();
	if (!fileName.isEmpty())
	{
		ui->sendFileBtn->setEnabled(false);
		ui->sendBtn->setEnabled(false);
		/* open file and create send thread */
		sendThread = new SendThread(serialPort);
		connect(this, &MainWindow::startSendFile, sendThread, &SendThread::startSendFile);
		connect(sendThread, &SendThread::sendFinished, this, &MainWindow::sendOver);
		emit startSendFile(fileName);
	}
	else
	{
		QMessageBox msgBox;
		msgBox.setWindowTitle(QString(tr("ERROR")));
		msgBox.setText(QString(tr("Please enter a file name or open a file!")));
		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.exec();
	}
}

void MainWindow::sendOver(qint64 retVal)
{
	ui->sendFileBtn->setEnabled(true);
	ui->sendBtn->setEnabled(true);
	QMessageBox msgBox;
	switch (retVal)
	{
		case OPEN_FILE_ERROR:
			msgBox.setWindowTitle(QString(tr("ERROR")));
			msgBox.setText(QString(tr("Unable to open the specified file!")));
			break;
		case SEND_FILE_ERROR:
			msgBox.setWindowTitle(QString(tr("ERROR")));
			msgBox.setText(QString(tr("File failed to send!")));
			break;
		case SEND_FILE_SUCCESS:
			msgBox.setWindowTitle(QString(tr("INFO")));
			msgBox.setText(QString(tr("The file was sent successfully!")));
			break;
		default:
			msgBox.setWindowTitle(QString(tr("ERROR")));
			QString msgInfo;
			QString::asprintf("%lld", retVal);
			msgInfo = QString(tr("File not sent completed, sent ")) + msgInfo;
			msgBox.setText(msgInfo);
			break;
	}
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.exec();
}

/*
void MainWindow::redetectPort(void)
{
	ui->serialPortSel->clear();
	QList<QSerialPortInfo> serialPortList = QSerialPortInfo::availablePorts();
	if (serialPortList.empty())
	{
		ui->serialPortSel->addItem(QString(tr("No SerialPort")));
	}
	else
	{
		QStringList portNameList;
		foreach(const QSerialPortInfo & info, serialPortList)
		{
			portNameList.append(info.portName());
		}
		qSort(portNameList.begin(), portNameList.end());
		foreach(const QString portName, portNameList)
		{
			ui->serialPortSel->addItem(portName);
		}
	}
}
*/

void MainWindow::timerChanged(int state)
{
	QLineEdit *timerEdit = ui->sendInterval;
	QString timerInterval = timerEdit->text();
	bool intervalCheck = true;
	int inter;
	if (timerInterval.isEmpty())
	{
		intervalCheck = false;
	}
	else
	{
		bool ok;
		inter = timerInterval.toInt(&ok, 10);
		if ((!ok) || (inter > 30) || (inter < 1))
		{
			intervalCheck = false;
		}
	}
	switch (state)
	{
		case Qt::Unchecked:
			if (trigTimer != nullptr)
			{
				trigTimer->stop();
				delete trigTimer;
				trigTimer = nullptr;
			}
			break;
		default:
			if (!intervalCheck)
			{
				QMessageBox msgBox;
				msgBox.setWindowTitle(QString(tr("ERROR")));
				msgBox.setText(QString(tr("Time interval must be 1 to 30 seconds!")));
				msgBox.setStandardButtons(QMessageBox::Ok);
				msgBox.exec();
				ui->schedSend->setCheckState(Qt::Unchecked);
				timerEdit->setFocus();
			}
			else if (serialPort == nullptr)
			{
				QMessageBox msgBox;
				msgBox.setWindowTitle(QString(tr("ERROR")));
				msgBox.setText(QString(tr("Please open the serial port first!")));
				msgBox.setStandardButtons(QMessageBox::Ok);
				msgBox.exec();
				ui->schedSend->setCheckState(Qt::Unchecked);
			}
			else if (sendThread != nullptr)
			{
				if (sendThread->isRunning())
				{
					QMessageBox msgBox;
					msgBox.setWindowTitle(QString(tr("ERROR")));
					msgBox.setText(QString(tr("Please wait for the file being sent to finish!")));
					msgBox.setStandardButtons(QMessageBox::Ok);
					msgBox.exec();
					ui->schedSend->setCheckState(Qt::Unchecked);
				}
			}
			else
			{
				if (trigTimer != nullptr)
				{
					trigTimer->stop();
					delete trigTimer;
					trigTimer = nullptr;
				}
				trigTimer = new QTimer(this);
				connect(trigTimer, SIGNAL(timeout()), this, SLOT(trigSend()));
				trigTimer->start(1000 * inter);
			}
			break;
	}
}

void MainWindow::trigSend()
{
	sendData();
}
