/* user header files */
#include "mainwidget.h"
#include "ui_mainwidget.h"
#include "recvthread.h"
#include "sendthread.h"

/* c++ header files */
#include <algorithm>

/* Qt header files */
#include <QFontDialog>
#include <QColorDialog>
#include <QtSerialPort/QSerialPortInfo>
#include <QMessageBox>
#include <QtDebug>
#include <QFileDialog>
#include <QTimer>


#ifdef _WIN32
#include <Windows.h>
#elif __linux__
#include <unistd.h>
#include <errno.h>
#else
#error Unsupported operating system
#endif // _WIN32

static bool compareQString(const QString str0, const QString str1)
{
    return QString::compare(str1, str0);
}

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
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
		foreach(const QSerialPortInfo & info, serialPortList)
		{
			portNameList.append(info.portName());
		}
        std::sort(portNameList.begin(), portNameList.end(), compareQString);
		foreach(const QString portName, portNameList)
		{
			ui->serialPortSel->addItem(portName);
		}
	}

    /* set baud rate */
    int baudRate[] = {9600,14400,19200,38400,57600,115200,230400,460800,921600};
    for(int rate:baudRate)
	{
        ui->baudRateSel->addItem(QString::number(rate));
    }
    ui->baudRateSel->setCurrentIndex(5);
    /* set data bits */
    int dataBits[] = {5,6,7,8};
    for(int bits:dataBits)
	{
        ui->dataBitSel->addItem(QString::number(bits));
    }
    ui->dataBitSel->setCurrentIndex(3);
    /* set stop bits */
    int stopBits[] = {1,2};
    for(int bits:stopBits)
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

	ui->sendInterval->setValidator(new QIntValidator(1, 30));
    connect(ui->setFontBtn,SIGNAL(clicked()),this,SLOT(setFont()));
    connect(ui->setBGColorBtn,SIGNAL(clicked()),this,SLOT(setBGColor()));
	connect(ui->setFontColorBtn, SIGNAL(clicked()), this, SLOT(setFontColor()));
    connect(ui->openSerialPortBtn,SIGNAL(clicked()),this,SLOT(openSerialPort()));
	connect(ui->sendBtn, SIGNAL(clicked()), this, SLOT(sendData()));
	qRegisterMetaType< QSerialPort::SerialPortError>("QSerialPort::SerialPortError");
	connect(ui->cleanDisplayArea, SIGNAL(clicked()), this, SLOT(cleanDisplay()));
	connect(ui->cleanSendArea, SIGNAL(clicked()), this, SLOT(cleanSend()));
	connect(ui->stopSendBtn, SIGNAL(clicked()), this, SLOT(stopSend()));
	connect(ui->openFileBtn, SIGNAL(clicked()), this, SLOT(openFile()));
	connect(ui->sendFileBtn, SIGNAL(clicked()), this, SLOT(sendFile()));
    //connect(ui->serialPortSel, SIGNAL(clicked()), this, SLOT(redetectPort()));
	connect(ui->schedSend, &QCheckBox::stateChanged, this, &MainWidget::timerChanged);
}

MainWidget::~MainWidget()
{
	resFree();
    delete ui;
}

inline void MainWidget::resFree(void)
{
	if (recvThread != nullptr)
	{
        disconnect(recvThread, &RecvThread::newData, this, &MainWidget::recvData);
		recvThread->stop();
		recvThread->quit();
		recvThread->terminate();
		recvThread->wait();
		delete recvThread;
		recvThread = nullptr;
	}

	if (sendThread != nullptr)
	{
        disconnect(this, &MainWidget::startSendFile, sendThread, &SendThread::startSendFile);
        disconnect(sendThread, &SendThread::sendFinished, this, &MainWidget::sendOver);
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
        disconnect(serialPort, &QSerialPort::errorOccurred,this, &MainWidget::portErrorProc);
		if (serialPort->isOpen())
		{
			serialPort->clear();
			serialPort->close();
		}
		delete serialPort;
		serialPort = nullptr;
	}
}
void MainWidget::setFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok);
    if(ok)
	{
		QTextBrowser* textBrowser = ui->recvTextBrowser;
        textBrowser->setFont(font);
    }
}

void MainWidget::setBGColor()
{
    QColor newColor = QColorDialog::getColor();
    if(newColor.isValid())
	{
		QTextBrowser* textBrowser = ui->recvTextBrowser;
		bgColor = newColor;
		QString colorStye = QString(tr("color:")) + fontColor.name() + QString(tr(";background-color:")) + bgColor.name() + QString(tr(";"));
        textBrowser->setStyleSheet(colorStye);
    }
}

void MainWidget::setFontColor()
{
	QColor newColor = QColorDialog::getColor();
	if (newColor.isValid())
	{
		QTextBrowser* textBrowser = ui->recvTextBrowser;
		fontColor = newColor;
		QString colorStye = QString(tr("color:")) + fontColor.name() + QString(tr(";background-color:")) + bgColor.name() + QString(tr(";"));
		textBrowser->setStyleSheet(colorStye);
	}
}

void MainWidget::openSerialPort(void)
{
    QString portName = ui->serialPortSel->currentText();
    QString baudRate = ui->baudRateSel->currentText();
    QString dataBits = ui->dataBitSel->currentText();
    QString stopBits = ui->stopBitSel->currentText();
    int chkBits = ui->checkBitSel->currentIndex();

    if(serialPort != nullptr)
	{
        /* turn off serialport */
        if(serialPort->isOpen())
		{
            serialPort->clear();
            serialPort->close();
        }
		resFree();
        ui->openSerialPortBtn->setText(QString(tr("打开串口")));
    }
    else
    {
        /* turn on serialport */
        serialPort = new QSerialPort();
        if(serialPort != nullptr)
		{
            serialPort->setPortName(portName);
            serialPort->setBaudRate(baudRate.toInt());
            serialPort->setDataBits(QSerialPort::DataBits(dataBits.toInt()));
            serialPort->setStopBits(QSerialPort::StopBits(stopBits.toInt()));

            QSerialPort::Parity parity = QSerialPort::Parity::NoParity;
            switch(chkBits)
            {
            case 0:
                parity = QSerialPort::Parity::NoParity;
                break;
            case 1:
                parity = QSerialPort::Parity::OddParity;
                break;
            case 2:
                parity = QSerialPort::Parity::EvenParity;
                break;
            default:
                parity =QSerialPort::Parity::NoParity;
                break;
            }

            serialPort->setParity(parity);
            serialPort->setFlowControl(QSerialPort::FlowControl::NoFlowControl);
            if(serialPort->open(QIODevice::ReadWrite))
            {
                serialPort->setDataTerminalReady(true);
                ui->openSerialPortBtn->setText(QString(tr("关闭串口")));
                recvThread = new RecvThread(serialPort);
                connect(recvThread, &RecvThread::newData,this, &MainWidget::recvData);
                connect(serialPort, &QSerialPort::errorOccurred,this, &MainWidget::portErrorProc);
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
						qDebug() << QString(tr("Wait for serial port to open ...\n")) << i << QString(tr("s\n"));
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

void MainWidget::recvData(const QByteArray &recvArray)
{
    if(serialPort!=nullptr)
	{
		QTextBrowser* textBrowser = ui->recvTextBrowser;
        if(ui->hexDisplay->checkState() != Qt::Unchecked)
		{
			QString recvStr = recvArray.toHex().toUpper();
			for (auto& x : recvStr)
			{
				textBrowser->insertPlainText(x);
			}
        }
		else
		{
			textBrowser->insertPlainText(QString(recvArray));
		}
    }
}

void MainWidget::portErrorProc(QSerialPort::SerialPortError error)
{
    bool fatal_error = false;
    QString err_str;
    switch (error)
    {
    case QSerialPort::NoError:
        err_str = QString(tr("No error."));
        break;
    case QSerialPort::DeviceNotFoundError:
        err_str = QString(tr("Device not found!"));
        fatal_error = true;
        break;
    case QSerialPort::PermissionError:
        err_str = QString(tr("Permission deny!"));
        fatal_error = true;
        break;
    case QSerialPort::OpenError:
        err_str = QString(tr("Device was already opened!"));
        fatal_error = true;
        break;
    case QSerialPort::ParityError:
        err_str = QString(tr("Parity error detected by the hardware while reading data!"));
        break;
    case QSerialPort::FramingError:
        err_str = QString(tr("Framing error detected by the hardware while reading data!"));
        break;
    case QSerialPort::BreakConditionError:
        err_str = QString(tr("Break condition detected by the hardware on the input line."));
        break;
    case QSerialPort::WriteError:
        err_str = QString(tr("Write error!"));
        break;
    case QSerialPort::ReadError:
        err_str = QString(tr("Read error!"));
        break;
    case QSerialPort::ResourceError:
        err_str = QString(tr("Device is unexpectedly removed from the system?"));
        fatal_error = true;
        break;
    case QSerialPort::UnsupportedOperationError:
        err_str = QString(tr("Operation is not supported or prohibited by the OS."));
        break;
    case QSerialPort::UnknownError:
        err_str = QString(tr("Unidentified error"));
        break;
    case QSerialPort::TimeoutError:
        err_str = QString(tr("Timeout"));
        break;
    case QSerialPort::NotOpenError:
        err_str = QString(tr("Please turn on the device first！"));
        fatal_error = true;
        break;
    }
    qDebug() << QString(tr("ERROR：")) + err_str + QString(tr("\n"));
    if(fatal_error)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(QString(tr("ERROR")));
        msgBox.setText(err_str);
        msgBox.setStandardButtons(QMessageBox::Ok);
        ui->openSerialPortBtn->click();
        msgBox.exec();
    }
}

void MainWidget::sendData()
{
	if (serialPort != nullptr)
	{
		QTextEdit* sendTextEdit = ui->sendTextEdit;
		QString sendMsg = sendTextEdit->toPlainText();
		QByteArray sendByteArray;
		if (Qt::Unchecked != ui->hexSend->checkState())
		{
			sendByteArray = QByteArray::fromHex(sendMsg.toLatin1());
		}
		else
		{
			sendByteArray = sendMsg.toLatin1();
			
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
		serialPort->write(sendByteArray);
	}
}

void MainWidget::cleanDisplay(void)
{
	QTextBrowser* textBrowser = ui->recvTextBrowser;
	textBrowser->clear();
}

void MainWidget::cleanSend(void)
{
	QTextEdit* textEdit = ui->sendTextEdit;
	textEdit->clear();
}

void MainWidget::stopSend(void)
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

void MainWidget::openFile(void)
{
    QString sendFileName = QFileDialog::getOpenFileName(this, QString(tr("打开文件")), QString(tr("%USERPROFILE%/")), QString(tr("任意文件(*.*)")));
    ui->sendFilePath->setText(sendFileName);
}

void MainWidget::sendFile(void)
{
	QString fileName = ui->sendFilePath->text();
	if (!fileName.isEmpty())
	{
		ui->sendFileBtn->setEnabled(false);
		ui->sendBtn->setEnabled(false);
		/* open file and create send thread */
        sendThread = new SendThread(serialPort);
		connect(this, &MainWidget::startSendFile, sendThread, &SendThread::startSendFile);
        connect(sendThread, &SendThread::sendFinished, this, &MainWidget::sendOver);
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

void MainWidget::sendOver(qint64 retVal)
{
	ui->sendFileBtn->setEnabled(true);
	ui->sendBtn->setEnabled(true);
    QMessageBox msgBox;
    switch(retVal)
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
        msgInfo.asprintf("%lld",retVal);
		msgInfo = QString(tr("File not sent completed, sent ")) + msgInfo;
		msgBox.setText(msgInfo);
        break;
    }
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

/*
void MainWidget::redetectPort(void)
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

void MainWidget::timerChanged(int state)
{
	QLineEdit* timerEdit = ui->sendInterval;
	QString timerInterval = timerEdit->text();
	bool intervalCheck = true;
	int inter = -1;
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
		else if(serialPort == nullptr)
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

void MainWidget::trigSend(void)
{
	sendData();
}
