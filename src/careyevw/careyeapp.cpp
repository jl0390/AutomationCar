#include <qstandardpaths.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qdebug.h>

#include "careyeapp.h"
#include "careyewnd.h"

CarEyeApp *g_app = NULL;

char configFileName[255]={0};

CarEyeApp::CarEyeApp(int &argc, char **argv)
	: QCoreApplication(argc, argv)
{
	g_app = this;
      m_engThread = NULL;
      m_wnd = NULL;
      if(argc >= 2)
    	    strcpy(configFileName, argv[1]);
      else
	    configFileName[0] = 0;

	setOrganizationName(APP_COMPANY);
	setApplicationName(APP_NAME);
}

CarEyeApp::~CarEyeApp()
{
    AppConfig::release();
}

bool CarEyeApp::init()
{
	if(configFileName[0] != 0)
		AppConfig::instance(configFileName);
	else
		AppConfig::instance();

    m_engThread = new CarEyeThread();

    return true;
}

#define SAFE_DELETE(obj) {\
    if (obj) { delete obj; obj=NULL; } \
}

void CarEyeApp::release()
{
    if (m_engThread)
        m_engThread->stop();
#if 0
    if (m_wnd)
        m_wnd->close();
    SAFE_DELETE(m_wnd);
#endif
    SAFE_DELETE(m_engThread);

	AppConfig::release();
}

int CarEyeApp::exec()
{
    if (m_engThread)
        m_engThread->start();

    m_wnd = new CarEyeWnd(this);
    m_wnd->start();
    connect(m_wnd, &CarEyeWnd::finished, this, QCoreApplication::quit);
    return QCoreApplication::exec();
}

void CarEyeApp::playSound(QString fname)
{
    if (MPlayerThread::playEnabled()) {
        MPlayerThread *p = new MPlayerThread(this);
        connect(p, &MPlayerThread::finished, p, &MPlayerThread::deleteLater);
        p->setMedia(fname);
        p->start();
    }
}

int main(int argc, char *argv[])
{
    CarEyeApp a(argc, argv);

    if (!a.init())
        return -1;

    int ret = a.exec();
    a.release();
    return ret;
}
