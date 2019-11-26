#include "sendthread.h"

/* C plusplus header file */
#include <memory>

/* Qt head file */
#include <QSerialPort>
#include <QReadWriteLock>
#include <QFile>
#include <QMessageBox>

static const int MAX_WRITE_LEN = 4096;

SendThread::SendThread(QSerialPort* port)
{
    Q_ASSERT(port != nullptr);
    serialPort = port;

    rwLock = new QReadWriteLock();
    exited = false;
}

SendThread::~SendThread()
{
	this->stop();
	this->quit();
	this->wait();
    if(rwLock != nullptr)
	{
        delete rwLock;
        rwLock = nullptr;
    }
}
void SendThread::startSendFile(const QString &fileName)
{
    if(fileName.length() != 0)
    {
        sendFileName = fileName;
        this->start();
    }
}

void SendThread::run()
{
    QFile file(sendFileName);
    if(!file.open(QIODevice::ReadOnly))
    {
        emit sendFinished(OPEN_FILE_ERROR);
    }
    else
    {
        qint64 sendCount = 0;
		std::unique_ptr<char> sendBuf(new char[MAX_WRITE_LEN]);
		int wLen = 0;
        while(!file.atEnd())
        {
			memset(sendBuf.get(), 0, MAX_WRITE_LEN);
			while (!serialPort->waitForBytesWritten(1000))
			{
				rwLock->lockForRead();
				bool _t_exit = exited;
				rwLock->unlock();
				if (_t_exit)
				{
					break;
				}
			}
            qint64 readLen = file.read(sendBuf.get(), 4096);
            if(readLen > 0)
            {
                wLen = serialPort->write(sendBuf.get(), readLen);
				if (wLen != -1)
				{
					readLen += wLen;
				}
            }
            rwLock->lockForRead();
            bool _t_exit = exited;
            rwLock->unlock();
            if(_t_exit)
			{
                break;
            }
        }
		file.close();
        if(sendCount == 0)
        {
            emit sendFinished(SEND_FILE_ERROR);
        }
        else if(sendCount == file.size())
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
	if (rwLock != nullptr)
	{
		rwLock->lockForWrite();
		exited = true;
		rwLock->unlock();
	}
}
