#ifndef MPLAYERTHREAD_H
#define MPLAYERTHREAD_H

#include <qmediaplayer.h>
#include <QThread>

#define MP_TIME     500

class MPlayerThread : public QThread
{
    Q_OBJECT

public:
    MPlayerThread(QObject *parent=0);
    ~MPlayerThread();
    
    static bool playEnabled();

    void setMedia(QString filename);

    protected slots:
    void changeDuration(qint64 d);

protected:
    void run();

    QString m_filename;
    QMediaPlayer *m_player;
    qint64 m_duration;
};

#endif // MPLAYERTHREAD_H
