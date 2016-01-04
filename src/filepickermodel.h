/*
 * Unplayer
 * Copyright (C) 2015 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UNPLAYER_FILEPICKERMODEL_H
#define UNPLAYER_FILEPICKERMODEL_H

#include <QAbstractListModel>
#include <QQmlParserStatus>

namespace Unplayer
{

struct FilePickerFile;

class FilePickerModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)
    Q_PROPERTY(QString parentDirectory READ parentDirectory NOTIFY directoryChanged)
    Q_PROPERTY(QStringList nameFilters READ nameFilters WRITE setNameFilters)
public:
    ~FilePickerModel();
    void classBegin();
    void componentComplete();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QString directory() const;
    void setDirectory(QString newDirectory);

    QString parentDirectory() const;

    QStringList nameFilters() const;
    void setNameFilters(const QStringList &filters);
protected:
    QHash<int, QByteArray> roleNames() const;
private:
    void loadDirectory();
private:
    QList<FilePickerFile*> m_files;
    QString m_directory;
    QStringList m_nameFilters;
signals:
    void directoryChanged();
};

}

#endif // UNPLAYER_FILEPICKERMODEL_H
