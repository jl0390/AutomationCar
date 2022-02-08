#ifndef PATT_DATA_EDITOR_H
#define PATT_DATA_EDITOR_H

#include <qcoreapplication.h>
#include <time.h>

#include "appconst.h"
#include "appconfig.h"
#include "careyethread.h"
#include "careyewnd.h"

class CarEyeApp : public QCoreApplication
{
	Q_OBJECT

public:
	CarEyeApp(int &argc, char **argv);
	~CarEyeApp();

    CarEyeThread* engineThread() {  return m_engThread; }

	bool init();
	void release();
	int exec();
    
    public slots:
    void playSound(QString fname);

protected:
    CarEyeThread *m_engThread;
    CarEyeWnd *m_wnd;

private:
};

extern CarEyeApp *g_app;

#endif // PATT_DATA_EDITOR_H
