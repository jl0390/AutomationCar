#ifndef WARNINGMANAGER_H
#define WARNINGMANAGER_H

#include <QObject>
#include <qnetworkreply.h>
#include <qnetworkaccessmanager.h>

#include "pstdthread.h"
#include "ce_infer.h"
#include "mplayerthread.h"

struct WarningSound {
    float time = 0;
    QString filename;
};

class WarningManager : public QObject
{
    Q_OBJECT

public:
    WarningManager(QObject *parent);
    ~WarningManager();

    bool init();
    void start();
    void stop();
    void checkCollisionTime(ce::InferList &infers);

signals:
    void soundChanged(QString fname);

    protected slots:
    void receiveReplyResult();

protected:
    int aborted();
    void threadFunc();
    int sendWarningMessage(ce::InferInfo &infer);
    int playWarningSound(double collisionTime);

    std::vector<WarningSound> m_warningSounds;
    float m_thresholdTime;
    QString m_serverUrl;

    std::mutex m_mutex;
    std::thread m_thread;
    int m_abort;
    ce::InferList m_infers;
    ce::InferList m_playList;

    //QtNetwork
    QNetworkReply* m_reply;
    QNetworkAccessManager *m_networkManager;
};

#endif // WARNINGMANAGE_H
