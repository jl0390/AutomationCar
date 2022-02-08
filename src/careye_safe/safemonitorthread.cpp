#include <qdebug.h>
#include "common.h"
#include "appconfig.h"
#include "safemonitorthread.h"

SafeMonitorThread::SafeMonitorThread(QObject *parent)
    : PQThread(parent)
{
    m_proc = NULL;
}

SafeMonitorThread::~SafeMonitorThread()
{
    if (m_proc)
        m_proc->deleteLater();
    m_proc = NULL;
}

int getProcessList(QStringList &list)
{
    return list.size();
}

static bool findProcess(const QString &appname)
{
    QStringList processList;
    getProcessList(processList);

    bool found = false;
    for (int i = 0; i < processList.size(); i++) {
        QString procName = processList[i];
        if (procName.indexOf(appname) >= 0) {
            found = true;
            break;
        }
    }
    return found;
}

void SafeMonitorThread::run()
{
    qDebug() << "SafeMonitorThread started!!!";

    restartApp();

    while(!quited()) {
        msleep(2000);
        if (m_proc && m_proc->state() == QProcess::NotRunning)
            restartApp();
    }
    qDebug() << "SafeMonitorThread finished!!!";
}

void SafeMonitorThread::restartApp()
{
    if (m_proc)
        m_proc->deleteLater();

    QString appdir = g_conf->value(CONF_APP_DIR).toString();
    QString appname = g_conf->value(CONF_APP_NAME).toString();

    QString path;
    if (appdir.isEmpty())
        path = appname;
#ifdef WIN32
    else if (appdir.size() > 2 && appdir[1] == ':')
        path = appdir + "/" + appname;
#else
    else if (appdir[0] == '/')
        path = appdir + "/" + appname;
#endif
    else
        path = QString::fromStdString(locateFile(appname.toStdString(), appdir.toStdString()));

    m_proc = new QProcess(0);
    qDebug() << "app path=" << path;
    m_proc->execute(path);
}
