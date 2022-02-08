#ifndef SAFEMONITORTHREAD_H
#define SAFEMONITORTHREAD_H

#include <qprocess.h>
#include "pqthread.h"

class SafeMonitorThread : public PQThread
{
    Q_OBJECT

public:
    SafeMonitorThread(QObject *parent=0);
    ~SafeMonitorThread();

protected:
    virtual void run();
    void restartApp();

private:
    QProcess *m_proc;
};

#endif // SAFEMONITORTHREAD_H
