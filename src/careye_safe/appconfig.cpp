#include <qcoreapplication.h>
#include <qsettings.h>
#include "appconfig.h"
#include "common.h"

static AppConfig *s_appConf;

typedef struct AppConfigItem {
	QString name;
	QVariant val;
} AppConfigItem;

const AppConfigItem s_confList[] = {
    { CONF_APP_DIR, "" },
    { CONF_APP_NAME, "" },

    { "" },
};

AppConfig::AppConfig(QObject *parent)
	: QObject(parent)
{
}

AppConfig::~AppConfig()
{

}

AppConfig* AppConfig::instance()
{
	if (s_appConf)
		return s_appConf;

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
    std::string iniFile = locateFile("careye_safe.ini", "models/");
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
