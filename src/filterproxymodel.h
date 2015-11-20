#ifndef UNPLAYER_FILTERPROXYMODEL_H
#define UNPLAYER_FILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QQmlParserStatus>

namespace Unplayer
{

class FilterProxyModel : public QSortFilterProxyModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QByteArray filterRoleName READ filterRoleName WRITE setFilterRoleName)
public:
    void classBegin();
    void componentComplete();

    QByteArray filterRoleName() const;
    void setFilterRoleName(const QByteArray &newFilterRoleName);

    Q_INVOKABLE int count() const;
    Q_INVOKABLE int proxyIndex(int sourceIndex) const;
    Q_INVOKABLE int sourceIndex(int proxyIndex) const;
private:
    QByteArray m_filterRoleName;
};

}

#endif // UNPLAYER_FILTERPROXYMODEL_H
