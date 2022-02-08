#ifndef CAREYEWND_H
#define CAREYEWND_H

#include <qthread.h>

class CarEyeWndPrivate;
class CarEyeWnd : public QThread
{
    Q_OBJECT
public:
    CarEyeWnd(QObject *parent=0);
    ~CarEyeWnd();

protected:
    virtual void run();
    CarEyeWndPrivate *m_d;

private:
};

#endif // CAREYEWND_H
