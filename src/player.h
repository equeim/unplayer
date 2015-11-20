#ifndef UNPLAYER_PLAYER_H
#define UNPLAYER_PLAYER_H

#include <QObject>
#include <QQmlParserStatus>

#include <gst/gst.h>

namespace AudioResourceQt {
class AudioResource;
}
class QTimer;
class MprisPlayer;

namespace Unplayer
{

class Queue;

class Player : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)
    Q_PROPERTY(Queue* queue READ queue NOTIFY queueChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
public:
    Player();
    ~Player();

    Queue *queue() const;

    bool isPlaying() const;

    qint64 position() const;
    void setPosition(qint64 newPosition);

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
private:
    void setPlaying(bool playing);

    void onAudioResourceAcquiredChanged();
    static gboolean onGstBusMessage(GstBus*, GstMessage *message, gpointer user_data);

    void onQueueCurrentTrackChanged();
private:
    AudioResourceQt::AudioResource *m_audioResource;
    GstElement *m_pipeline;
    GstBus *m_bus;
    MprisPlayer *m_mprisPlayer;

    Queue *m_queue;
    bool m_playing;

    QTimer *m_positionTimer;
signals:
    void queueChanged();
    void positionChanged();
    void playingChanged();
};

}

#endif // UNPLAYER_PLAYER_H
