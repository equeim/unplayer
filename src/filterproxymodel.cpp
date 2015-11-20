#include "filterproxymodel.h"
#include "filterproxymodel.moc"

namespace Unplayer
{

void FilterProxyModel::classBegin()
{

}

void FilterProxyModel::componentComplete()
{
    setFilterRole(sourceModel()->roleNames().key(m_filterRoleName));
}

QByteArray FilterProxyModel::filterRoleName() const
{
    return m_filterRoleName;
}

void FilterProxyModel::setFilterRoleName(const QByteArray &newFilterRoleName)
{
    m_filterRoleName = newFilterRoleName;
}

int FilterProxyModel::count() const
{
    return rowCount();
}

int FilterProxyModel::proxyIndex(int sourceIndex) const
{
    return mapFromSource(sourceModel()->index(sourceIndex, 0)).row();
}

int FilterProxyModel::sourceIndex(int proxyIndex) const
{
    return mapToSource(index(proxyIndex, 0)).row();
}

}
