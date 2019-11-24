#ifndef RECVTHREAD_H
#define RECVTHREAD_H

#include <QThread>

class QSerialPort;
class QReadWriteLock;

class RecvThread : public QThread
{
    Q_OBJECT
public:
    RecvThread(QSerialPort* port);
    ~RecvThread() override;

    void run() override;
    void stop();
private:
    QSerialPort* serialPort;
    QReadWriteLock* rwLock;
    bool exited;
signals:
    void newData(const QByteArray &msgArray);
};

#endif // RECVTHREAD_H
