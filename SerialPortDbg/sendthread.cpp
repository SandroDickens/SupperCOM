#include "sendthread.h"

/* C plusplus header file */
#include <memory>

/* Qt head file */
#include <QSerialPort>
#include <QReadWriteLock>
#include <QFile>
#include <QMessageBox>
#include <QScopedArrayPointer>

SendThread::SendThread(QSerialPort* port)
{
    Q_ASSERT(port != nullptr);
    serialPort = port;

    rwLock = new QReadWriteLock();
    exited = false;
}

SendThread::~SendThread()
{
    if(serialPort != nullptr) {
        /* notif main thread */
    }
    if(rwLock != nullptr) {
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
        QScopedArrayPointer<char> sendBuf(new char[4096]);
        while(!file.atEnd())
        {
            qint64 readLen = file.read(sendBuf.get(), 4096);
            if(readLen > 0)
            {
                sendCount += serialPort->write(sendBuf.get(), readLen);
            }
            /*rwLock->lockForRead();
            bool _t_exit = exited;
            rwLock->unlock();
            if(_t_exit) {
                break;
            }*/
        }

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
    rwLock->lockForWrite();
    exited = true;
    rwLock->unlock();
}
