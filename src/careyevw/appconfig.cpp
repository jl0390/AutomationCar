#include <qcoreapplication.h>
#include <qsettings.h>
#include "appconst.h"
#include "appconfig.h"
#include "filesystem.h"
#include "careyewnd.h"
#include "careyeapp.h"


static AppConfig *s_appConf;

typedef struct AppConfigItem {
	QString name;
	QVariant val;
} AppConfigItem;

const AppConfigItem s_confList[] = {
	{ CONF_DETECT_MODE_DIR, "" },
	{ CONF_DETECT_PROTO, "" },
	{ CONF_DETECT_MODEL, "" },
    { CONF_DETECT_LABEL, "" },
    { CONF_DETECT_THRESHOLD, 0.0 },

#if CE_USE_IMAGENET_FEATURE
    { CONF_CLASS_MODE_DIR, "" },
    { CONF_CLASS_PROTO, "" },
    { CONF_CLASS_MODEL, "" },
    { CONF_CLASS_LABEL, "" },
    { CONF_CLASS_THRESHOLD, 0.0 },
#endif
#if 0 && CE_USE_RANGE_FINDER
    { CONF_CAMERA_DIST, 6.0 },
    { CONF_CAMERA_FOCUS, 500.0 },
    { CONF_CAMERA_DELTA, -72.5 },
#endif

    { CONF_CAMERA_DEV, 0 },
    { CONF_CAMERA_PARAM, "" },
    { CONF_CAMERA_WIDTH, 640 },
    { CONF_CAMERA_HEIGHT, 480 },

    { CONF_RECORD_DIR, "" },
    { CONF_RECORD_TIME, 0 },

    { CONF_VIDEO_FILE, "" },
    { CONF_FULL_SCREEN, 1 },

    //collision
    { CONF_VELOCITY_SHOW, 0 },
    { CONF_SERVER_URL, "" },
    { CONF_COLLISION_THRESHOLD, 3.0 },
    { CONF_COLLISION_SOUND_DIR, "" },
    { CONF_COLLISION_SOUND_FILES, "" },

    { "" },
};

std::string _fileName = "careye.ini";

AppConfig::AppConfig(QObject *parent)
	: QObject(parent)
{
}

AppConfig::~AppConfig()
{

}

AppConfig* AppConfig::instance(const char *fileName)
{
	if (s_appConf)
		return s_appConf;
	if(fileName!=NULL)
		_fileName = fileName;
	s_appConf = new AppConfig(qApp);
	s_appConf->load();
	return s_appConf;
}

void AppConfig::release()
{
	if (s_appConf) {
		s_appConf->save();
		delete s_appConf;
		s_appConf = NULL;
	}
}

QVariant AppConfig::value(const QString &key)
{
	std::lock_guard<std::mutex> locker(m_mutex);
	if (m_items.count(key) > 0)
		return m_items.at(key);
	return QVariant();
}

void AppConfig::setValue(const QString &key, const QVariant &value)
{
    std::lock_guard<std::mutex> locker(m_mutex);
    m_items[key] = value;
}

static std::string iniFileName()
{
    std::string iniFile = fs::locateFile(_fileName.c_str(), "models/");
    std::cout << "configFileName:"<< iniFile  << std::endl;

    //std::string iniFile = fs::locateFile("careye.ini", "models/");
    return iniFile;
}

void AppConfig::load()
{
    std::string iniFile = iniFileName();
    QSettings s(iniFile.c_str(),QSettings::IniFormat);

    s.beginGroup("Global");
	for (int i = 0; !s_confList[i].name.isEmpty(); i ++) {
		QVariant val = s.value(s_confList[i].name, s_confList[i].val);
		setValue(s_confList[i].name, val);
	}
    s.endGroup();
}

void AppConfig::save()
{
#if 0
    std::string iniFile = iniFileName();
    QSettings s(iniFile.c_str());

    s.beginGroup("Global");
    for (int i = 0; !s_confList[i].name.isEmpty(); i ++) {
		QVariant val = value(s_confList[i].name);
		s.setValue(s_confList[i].name, val);
	}
    s.endGroup();
#endif
}
