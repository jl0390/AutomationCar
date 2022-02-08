#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <QObject>
#include <qstring.h>
#include <qvariant.h>

#include <map>
#include <mutex>

#define CONF_DETECT_MODE_DIR	"detect_model_dir"
#define CONF_DETECT_PROTO       "detect_proto"
#define CONF_DETECT_MODEL       "detect_model"
#define CONF_DETECT_LABEL       "detect_label"
#define CONF_DETECT_THRESHOLD   "detect_threshold"

#if CE_USE_IMAGENET_FEATURE
#define CONF_CLASS_MODE_DIR     "class_model_dir"
#define CONF_CLASS_PROTO        "class_proto"
#define CONF_CLASS_MODEL        "class_model"
#define CONF_CLASS_LABEL        "class_label"
#define CONF_CLASS_THRESHOLD    "class_threshold"
#endif

#define CONF_CAMERA_DEV         "camera_dev"
#define CONF_CAMERA_PARAM       "camera_param"
#define CONF_CAMERA_WIDTH       "camera_width"
#define CONF_CAMERA_HEIGHT      "camera_height"

#define CONF_RECORD_DIR         "record_dir"
#define CONF_RECORD_TIME        "record_time"

#define CONF_VIDEO_FILE         "video_file"
#define CONF_FULL_SCREEN        "full_screen"

//waring
#define CONF_VELOCITY_SHOW      "velocity_show"
#define CONF_SERVER_URL         "server_url"
#define CONF_COLLISION_THRESHOLD    "collision_threshold"
#define CONF_COLLISION_SOUND_DIR    "collision_sound_dir"
#define CONF_COLLISION_SOUND_FILES  "collision_sound_files"

class AppConfig : public QObject
{
	Q_OBJECT

protected:
	AppConfig(QObject *parent=0);
	~AppConfig();

public:
	static AppConfig* instance(const char *fileName=0);
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
