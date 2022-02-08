#include <qfileinfo.h>
#include <qdebug.h>
#include "pstdthread.h"
#include "mplayerthread.h"

static int s_playing = 0;

MPlayerThread::MPlayerThread(QObject *parent)
    : QThread(parent)
{
    m_player = new QMediaPlayer();
    m_duration = 0;
}

MPlayerThread::~MPlayerThread()
{
    delete m_player;
}

bool MPlayerThread::playEnabled()
{
    return s_playing ? false : true;
}

void MPlayerThread::setMedia(QString filename)
{
    m_filename = filename;
}

void MPlayerThread::run()
{
    QFileInfo fi(m_filename);
    if (!fi.exists())
        return;

    s_playing = 1;

    //qDebug() << "MPlayerThread:run " << m_filename;
    m_player->setMedia(QUrl::fromLocalFile(m_filename));
    m_player->play();
    m_duration = m_player->duration();
    connect(m_player, &QMediaPlayer::durationChanged, this, &MPlayerThread::changeDuration);

    msleep(MP_TIME);
    if (m_duration > MP_TIME) {
        //qDebug() << "MPlayerThread::run:msleep" << m_duration - MP_TIME;
        msleep(m_duration - MP_TIME);
    }
    s_playing = 0;
}

void MPlayerThread::changeDuration(qint64 d)
{
    m_duration = d;
}
