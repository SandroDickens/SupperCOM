#include "recvthread.h"

#include <QSerialPort>
#include <QReadWriteLock>

RecvThread::RecvThread(QSerialPort* port)
{
    Q_ASSERT(port != nullptr);
    serialPort = port;

    rwLock = new QReadWriteLock();
    exited = false;
}

RecvThread::~RecvThread()
{
    if(serialPort != nullptr) {
        /* notif main thread */
    }
    if(rwLock != nullptr) {
        delete rwLock;
        rwLock = nullptr;
    }
}

void RecvThread::run()
{
    while(true) {
        if(serialPort->waitForReadyRead(1000)) {
            QByteArray recvArray = serialPort->readAll();
            emit newData(recvArray);
        }
        rwLock->lockForRead();
        bool _t_exit = exited;
        rwLock->unlock();
        if(_t_exit) {
            break;
        }
    }
}

void RecvThread::stop()
{
    rwLock->lockForWrite();
    exited = true;
    rwLock->unlock();
}
