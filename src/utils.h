#ifndef UNPLAYER_UTILS_H
#define UNPLAYER_UTILS_H

#include <QObject>

namespace Unplayer
{

class Utils : public QObject
{
    Q_OBJECT
public:
    Utils();

    Q_INVOKABLE static QString mediaArt(const QString &artistName, const QString &albumTitle);
    Q_INVOKABLE static QString mediaArtForArtist(const QString &artistName);
    Q_INVOKABLE static QString randomMediaArt();

    Q_INVOKABLE static QString formatDuration(uint seconds);

    Q_INVOKABLE static QString escapeRegExp(const QString &string);
    Q_INVOKABLE static QString escapeSparql(QString string);
private:
    static QString mediaArtMd5(QString string);
private:
    static const QString m_mediaArtDirectoryPath;
};

}

#endif // UNPLAYER_UTILS_H
