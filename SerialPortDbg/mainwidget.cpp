#include "mainwidget.h"
#include "ui_mainwidget.h"
#include "recvthread.h"
#include <QFontDialog>
#include <QColorDialog>
#include <QtSerialPort/QSerialPortInfo>
#include <QSerialPort>
#include <QMessageBox>
#include <QtDebug>
#include <QRegExp>

#ifdef _WIN32
#include <Windows.h>
#elif __linux__
#include <sys/stat.h>
#else
#error Unsupported operating system
#endif // _WIN32


MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);
    serialPort = nullptr;
    recvThread = nullptr;
    /* list serialport */
    QList<QSerialPortInfo> m_serialPortList = QSerialPortInfo::availablePorts();
    QStringList portNameList;
    foreach (const QSerialPortInfo &info, m_serialPortList) {
         portNameList.append(info.portName());
    }
	foreach (const QString portName, portNameList)
	{
		ui->serialPortSel->addItem(portName);
	}
    /* set baud rate */
    int baudRate[] = {9600,14400,19200,38400,57600,115200,230400,460800,921600};
    for(int rate:baudRate) {
        ui->baudRateSel->addItem(QString::number(rate));
    }
    ui->baudRateSel->setCurrentIndex(5);
    /* set data bits */
    int dataBits[] = {5,6,7,8};
    for(int bits:dataBits) {
        ui->dataBitSel->addItem(QString::number(bits));
    }
    ui->dataBitSel->setCurrentIndex(3);
    /* set stop bits */
    int stopBits[] = {1,2};
    for(int bits:stopBits) {
        ui->stopBitSel->addItem(QString::number(bits));
    }
    ui->stopBitSel->setCurrentIndex(0);
    /* set check type */
    ui->checkBitSel->addItem(QString(tr("NO")));
    ui->checkBitSel->addItem(QString(tr("Odd")));
    ui->checkBitSel->addItem(QString(tr("Even")));
    ui->checkBitSel->setCurrentIndex(0);

    ui->recvTextEdit->setStyleSheet(QString(tr("color:green;background-color:white;")));
	ui->recvTextEdit->setReadOnly(true);
    QFont font;
    font.setFamily(QString::fromUtf8("Times New Roman"));
    font.setPointSize(11);
	ui->recvTextEdit->setFont(font);
	ui->recvTextEdit->setText("我能吞下玻璃而不伤身。I can eat glass, it doesn't hurt me.");

    if(0 == ui->serialPortSel->count()) {
        ui->openSerialPortBtn->setEnabled(false);
        setWindowTitle(QString(tr("SerialPortDebugger - NO SerialPort")));
    }
    //connect(ui->fontBtn,SIGNAL(clicked()),this,SLOT(showFontDlg()));
    //connect(ui->backGrndBtn,SIGNAL(clicked()),this,SLOT(showColorDlg()));
    connect(ui->openSerialPortBtn,SIGNAL(clicked()),this,SLOT(openSerialPort()));
	connect(ui->sendBtn, SIGNAL(clicked()), this, SLOT(sendData()));
	qRegisterMetaType< QSerialPort::SerialPortError>("QSerialPort::SerialPortError");
}

MainWidget::~MainWidget()
{
    if(serialPort != nullptr) {
        if(serialPort->isOpen()) {
            serialPort->clear();
            serialPort->close();
        }
        delete serialPort;
        serialPort = nullptr;
    }
    delete ui;
}

void MainWidget::showFontDlg()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok);
    if(ok) {
		QTextEdit* recvDisplayEdit = ui->recvTextEdit;
        QTextCursor cursor = recvDisplayEdit->textCursor();
        cursor.select(QTextCursor::BlockUnderCursor);
        QTextCharFormat fmt;
        fmt.setFont(font);
        cursor.setCharFormat(fmt);
        //recvDisplayEdit->setFont(font);
        recvDisplayEdit->append("我能吞下玻璃而不伤身。I can eat glass, it doesn't hurt me.");
    }
}

void MainWidget::showColorDlg()
{
    QColor color = QColorDialog::getColor();
    if(color.isValid()) {
        QTextEdit* recvDisplayEdit = ui->recvTextEdit;
        recvDisplayEdit->setStyleSheet(QString(tr("color:")+color.name()));
        recvDisplayEdit->append("我能吞下玻璃而不伤身。I can eat glass, it doesn't hurt me.");
    }
}

void MainWidget::openSerialPort(void)
{
    QString portName = ui->serialPortSel->currentText();
    QString baudRate = ui->baudRateSel->currentText();
    QString dataBits = ui->dataBitSel->currentText();
    QString stopBits = ui->stopBitSel->currentText();
    int chkBits = ui->checkBitSel->currentIndex();

    if(serialPort != nullptr) {
        /* 关闭串口 */
        disconnect(serialPort, &QSerialPort::errorOccurred,this, &MainWidget::portDisconnect);
        if(serialPort->isOpen()) {
            serialPort->clear();
            serialPort->close();
            ui->openSerialPortBtn->setText(QString(tr("打开串口")));
        }
        disconnect(recvThread, &RecvThread::newData,this, &MainWidget::recvData);
		recvThread->stop();
		//recvThread->terminate();
		recvThread->wait();
        delete recvThread;
        recvThread = nullptr;
        delete serialPort;
        serialPort = nullptr;
    }
    else
    {
        /* 打开串口 */
        serialPort = new QSerialPort();
        if(serialPort != nullptr) {
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
                connect(serialPort, &QSerialPort::errorOccurred,this, &MainWidget::portDisconnect);
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
					msgBox.setWindowTitle(QString(tr("错误")));
					msgBox.setText(QString(tr("串口打开超时!")));
					msgBox.setStandardButtons(QMessageBox::Ok);
					ui->openSerialPortBtn->click();
				}
				else
				{
					recvThread->start();
				}
            }
            else {
                qDebug() << QString(tr("can NOT open serialport \'")) << portName << QString(tr("\'\n"));
                delete serialPort;
                serialPort = nullptr;
            }
        }
        else {
            qDebug() << QString(tr("can NOT create serialport.\n"));
        }
    }
}

void MainWidget::recvData(const QByteArray &recvArray)
{
    if(serialPort!=nullptr) {
        QTextEdit* recvDisplayEdit = ui->recvTextEdit;
        if(ui->hexDisplay->checkState() != Qt::Unchecked) {
            recvDisplayEdit->append(QString(recvArray.toHex()));
        }
        recvDisplayEdit->append(QString(recvArray));
    }
}

void MainWidget::portDisconnect()
{
	return;
    /*QMessageBox msgBox;
    msgBox.setWindowTitle(QString(tr("错误")));
    msgBox.setText(QString(tr("串口已断开连接!")));
    msgBox.setStandardButtons(QMessageBox::Ok);
    ui->openSerialPortBtn->click();
	msgBox.exec();*/
}

void MainWidget::sendData()
{
	if (serialPort != nullptr) {
		/*
		QRegExp hexRegExp(tr("[0-9a-fA-F]$"));
		QValidator* hexValid = new QRegExpValidator(hexRegExp, this);
		*/
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
		/*
		int pos = 0;
		QValidator::State state = hexValid->validate(sendMsg, pos);
		if (QValidator::State::Acceptable == state)
		{
			serialPort->write(QByteArray::fromHex(sendMsg.toLatin1()));
		}
		else
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle(QString(tr("提示")));
			msgBox.setText(QString(tr("非法字符!")));
			msgBox.setDetailedText(QString(tr("16进制发送时，输入字符必须是0-F")));
			msgBox.setStandardButtons(QMessageBox::Ok);
			sendTextEdit->moveCursor(QTextCursor::Start);
			QTextCursor tempCursor = sendTextEdit->textCursor();
			tempCursor.setPosition(pos);
			sendTextEdit->setTextCursor(tempCursor);
			msgBox.exec();
		}
		delete hexValid;
		*/
	}
}
