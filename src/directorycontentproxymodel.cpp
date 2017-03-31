#include "directorycontentproxymodel.h"

namespace unplayer
{
    int DirectoryContentProxyModel::isDirectoryRole() const
    {
        return mIsDirectoryRole;
    }

    void DirectoryContentProxyModel::setIsDirectoryRole(int isDirectoryRole)
    {
        mIsDirectoryRole = isDirectoryRole;
    }

    bool DirectoryContentProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        const bool leftIsDirectory = left.data(mIsDirectoryRole).toBool();
        const bool rightIsDirectory = right.data(mIsDirectoryRole).toBool();
        if (leftIsDirectory != rightIsDirectory) {
            return leftIsDirectory;
        }
        return FilterProxyModel::lessThan(left, right);
    }
}
