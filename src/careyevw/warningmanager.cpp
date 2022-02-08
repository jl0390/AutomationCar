#include <qfileinfo.h>
#include <qdebug.h>
#include "appconfig.h"
#include "filesystem.h"
#include "warningmanager.h"

WarningManager::WarningManager(QObject *parent)
    : QObject(parent)
{
    m_networkManager = NULL;
    m_reply = NULL;
}

WarningManager::~WarningManager()
{
    stop();
}

bool WarningManager::init()
{
    m_thresholdTime = g_conf->value(CONF_COLLISION_THRESHOLD).toFloat();
    m_serverUrl = g_conf->value(CONF_SERVER_URL).toString();

    std::string dirname = g_conf->value(CONF_COLLISION_SOUND_DIR).toString().toStdString();
    QString files = g_conf->value(CONF_COLLISION_SOUND_FILES).toString();
    QStringList filenames = files.split(",");

    for (int i = 0; i < filenames.size(); i++) {
        QString filename = filenames[i].simplified();
        QString path = fs::locateFile(filename.toStdString(), dirname).c_str();

        if (path.isEmpty())
            continue;
        QFileInfo fileInfo(path);
        if (!fileInfo.exists())
            continue;

        WarningSound ws;
        int pos = filename.lastIndexOf(".");
        if (pos > 0) {
            QString second = filename.left(pos);
            ws.time = second.toFloat();
        }
        path = fileInfo.absoluteFilePath();
        ws.filename = path;
        qDebug() << "sound file:" << ws.time << "," << ws.filename;
        m_warningSounds.push_back(ws);
    }

    //sort
    if (m_warningSounds.size() > 1)  {
        for (int i = 0; i < (int)m_warningSounds.size() - 1; i++) {
            WarningSound ws = m_warningSounds[i];
            for (int j = i + 1; j < (int)m_warningSounds.size(); j++) {
                if (ws.time>m_warningSounds[j].time) {
                    m_warningSounds[i] = m_warningSounds[j];
                    m_warningSounds[j] = ws;
                    ws = m_warningSounds[i];
                }
            }
        }
    }
    return true;
}

void WarningManager::start()
{
    m_abort = 0;
    m_thread = std::thread(&WarningManager::threadFunc, this);
}

void WarningManager::stop()
{
    if (!aborted()) {
        m_mutex.lock();
        m_abort = 1;
        m_mutex.unlock();
        if (m_thread.joinable())
            m_thread.join();
    }
}

void WarningManager::checkCollisionTime(ce::InferList &infers)
{
    int index = -1;
    double minTime = 0;
    for (int i = 0; i < (int)infers.size(); i++) {
        if (infers[i].collisionTime > 0.0001 && infers[i].collisionTime <= m_thresholdTime) {
            if (minTime<0.0000001 || minTime>infers[i].collisionTime) {
                index = i;
                minTime = infers[i].collisionTime;
            }
        }
    }

    if (index >= 0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_infers.clear();
        m_infers.push_back(infers[index]);
    }
}

int WarningManager::aborted()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_abort;
}

void WarningManager::threadFunc()
{
    ce::InferInfo infer;

    while (!aborted()) {
        PStdThread::msleep(10);
        m_mutex.lock();
        if (m_infers.size() == 0) {
            m_mutex.unlock();
            continue;
        }
        infer = m_infers[0];
        m_infers.erase(m_infers.begin());
        m_mutex.unlock();

        //process
        //qDebug() << "process an warning " << infer.collisionTime;
        sendWarningMessage(infer);
        playWarningSound(infer.collisionTime);
    }
}

int WarningManager::sendWarningMessage(ce::InferInfo &infer)
{
    if (m_serverUrl.isEmpty())
        return -1;
    if (m_networkManager)
        return -1;

    QString reqstr = QString("%1?time=%2").arg(m_serverUrl).arg(infer.collisionTime);
    QUrl url = QUrl(reqstr);
    QNetworkRequest request(url);

    m_networkManager = new QNetworkAccessManager;
    m_reply = m_networkManager->get(request);
    connect(m_reply, &QNetworkReply::finished, this, &WarningManager::receiveReplyResult);

    return 0;
}

void WarningManager::receiveReplyResult()
{
    qDebug() << "receiveDiagResult: " << m_reply->error();
    if (m_reply->error() != QNetworkReply::NoError) {
        qDebug() << "Connection to Server Failed.";
    }
    else {
        QByteArray responseData = m_reply->readAll();
        qDebug() << responseData;
        qDebug() << "Send done!";
    }

    if (m_reply)
        m_reply->deleteLater();
    if (m_networkManager)
        m_networkManager->deleteLater();
    m_reply = NULL;
    m_networkManager = NULL;
}

int WarningManager::playWarningSound(double collisionTime)
{
    if (m_warningSounds.size() == 0)
        return -1;

    int index = (int)m_warningSounds.size() - 1;
    for (int i = 0; i < (int)m_warningSounds.size(); i++) {
        WarningSound &ws = m_warningSounds[i];
        if (collisionTime <= ws.time) {
            index = i;
            break;
        }
    }

    WarningSound &ws = m_warningSounds[index];
#if 0
    MPlayerThread *p = new MPlayerThread();
    connect(p, &MPlayerThread::finished, p, &MPlayerThread::deleteLater);
    p->setMedia(ws.filename);
    p->start();
    //qDebug() << "play sound : " << ws.filename;
#else
    emit soundChanged(ws.filename);
#endif
    return 0;
}
