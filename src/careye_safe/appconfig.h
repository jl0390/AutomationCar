#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <QObject>
#include <qstring.h>
#include <qvariant.h>

#include <map>
#include <mutex>

#define CONF_APP_DIR	        "app_dir"
#define CONF_APP_NAME	        "app_name"

class AppConfig : public QObject
{
	Q_OBJECT

protected:
	AppConfig(QObject *parent=0);
	~AppConfig();

public:
	static AppConfig* instance();
	static void release();

	QVariant value(const QString &key);
	void setValue(const QString &key, const QVariant &value);

	void load();
	void save();

private:
	std::mutex m_mutex;
	std::map<QString, QVariant> m_items;
};
#define g_conf (static_cast<AppConfig *>(AppConfig::instance()))

#endif // APP_CONFIG_H
