#include <QtCore/QCoreApplication>
#include "safemonitorthread.h"
#include "appconfig.h"

#include <csignal>
volatile int g_signalCtrlC = 0;
void signal_handler(int sig)
{
    signal(sig, signal_handler);
    g_signalCtrlC = 1;
    qApp->quit();
}

void initSignal()
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#ifdef SIGBREAK
    signal(SIGBREAK, signal_handler);
#endif
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    initSignal();

    SafeMonitorThread thread;
    thread.start();

    int res = a.exec();

    thread.stop();
    return res;
}
