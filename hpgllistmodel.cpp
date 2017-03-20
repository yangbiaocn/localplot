#include "hpgllistmodel.h"

bool operator==(const file_uid& lhs, const file_uid& rhs)
{
    return(lhs.path == rhs.path && lhs.uid == rhs.uid);
}

hpglListModel::hpglListModel(QObject *parent)
    :QAbstractListModel(parent)
{
    hpglData.clear();
}

QVariant hpglListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    if (index.row() >= hpglData.length() || index.row() < 0)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        return hpglData.at(index.row())->name.filename;
    }
    return(QVariant());
}

bool hpglListModel::dataGroup(const QPersistentModelIndex index, QMutex *& retLocker,
                                   QGraphicsItemGroup *& itemGroup)
{
    if (!index.isValid())
    {
        return false;
    }
    if (index.row() >= hpglData.length() || index.row() < 0)
    {
        return false;
    }

    retLocker = &(hpglData[index.row()]->mutex);
    itemGroup = hpglData.at(index.row())->hpgl_items_group;

    return true;
}

bool hpglListModel::dataItemsGroup(const QPersistentModelIndex index, QMutex *& retLocker,
                                   QGraphicsItemGroup *& itemGroup, QVector<QGraphicsPolygonItem*> *& items)
{
    if (!index.isValid())
    {
        return false;
    }
    if (index.row() >= hpglData.length() || index.row() < 0)
    {
        return false;
    }

    retLocker = &(hpglData[index.row()]->mutex);
    itemGroup = hpglData.at(index.row())->hpgl_items_group;
    items = &(hpglData.at(index.row())->hpgl_items);
    return true;
}

bool hpglListModel::dataItems(const QPersistentModelIndex index, QMutex *& retLocker, QVector<QGraphicsPolygonItem*> *& items)
{
    if (!index.isValid())
    {
        return false;
    }
    if (index.row() >= hpglData.length() || index.row() < 0)
    {
        return false;
    }

    retLocker = &(hpglData[index.row()]->mutex);
    items = &(hpglData.at(index.row())->hpgl_items);
    return true;
}

bool hpglListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QVector<int> changedRoles;

    if (!index.isValid())
    {
        return false;
    }
    if (index.row() >= hpglData.length() || index.row() < 0)
    {
        return false;
    }

    if (role == Qt::DisplayRole)
    {
        QMutexLocker locker(&(hpglData[index.row()]->mutex));
        hpglData[index.row()]->name.filename = value.toString();
        changedRoles.push_back(Qt::DisplayRole);
        emit dataChanged(index, index, changedRoles);
        return true;
    }
    return false;
}

bool hpglListModel::setFileUid(const QModelIndex &index, const file_uid filename)
{
    if (index.isValid())
    {
        QMutexLocker locker(&(hpglData[index.row()]->mutex));
        hpglData.at(index.row())->name.filename = filename.filename;
        hpglData.at(index.row())->name.path = filename.path;
        return true;
    }
    return false;
}

bool hpglListModel::setGroupFlag(const QModelIndex &index, QGraphicsItem::GraphicsItemFlag flag, bool flagValue)
{
    if (index.row() >= 0 && index.row() < hpglData.length())
    {
        QMutexLocker locker(&(hpglData[index.row()]->mutex));
        hpglData[index.row()]->hpgl_items_group->setFlag(flag, flagValue);
        return true;
    }
    return false;
}

QModelIndex hpglListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row >= 0 && row < hpglData.length())
    {
        return(createIndex(row, column));
    }
    return (QModelIndex());
}

int hpglListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return hpglData.length();
}

bool hpglListModel::insertRow(int row, const QModelIndex &parent)
{
    return insertRows(row, 1, parent);
}

bool hpglListModel::removeRow(int row, const QModelIndex &parent)
{
    return removeRows(row, 1, parent);
}

bool hpglListModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (count < 0 || row < 0 || row > hpglData.length())
    {
        return false;
    }
    beginInsertRows(parent, row, (row + count - 1));
    for (int i = row; i < (row+count); ++i)
    {
        hpgl_file * newFile;
        QModelIndex index = createIndex(i, 0);
        newFile = new hpgl_file;
        QMutexLocker locker(&(newFile->mutex));
        newFile->hpgl_items_group = new QGraphicsItemGroup;
        newFile->hpgl_items_group->setData(QMODELINDEX_KEY, QPersistentModelIndex(index));
        newFile->name.filename = "NA";
        newFile->name.path = "NA";
        if (hpglData.length() == 0)
        {
            newFile->name.uid = 0;
        }
        else
        {
            newFile->name.uid = hpglData.last()->name.uid + 1;
        }
        newFile->hpgl_items.clear();
        newFile->cutoutBox = NULL;
        hpglData.insert(i, newFile);
    }
    endInsertRows();
    return true;
}

bool hpglListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    qDebug() << "removerows: " << row << count << parent << hpglData.length();
    if (count <= 0 || row < 0 || (row+count-1) >= hpglData.length())
    {
        return false;
    }
    beginRemoveRows(parent, row, (row + count - 1));
    for (int i = (row+count-1); i >= row; --i)
    {
        QMutexLocker locker(&(hpglData[i]->mutex));
//        delete hpglData[i]->hpgl_items_group;
        hpglData[i]->hpgl_items.clear();
        delete hpglData[i];
        hpglData.remove(i);
    }
    endRemoveRows();
    return true;
}

void hpglListModel::addPolygon(QPersistentModelIndex index, QGraphicsPolygonItem * poly)
{
    if (!index.isValid())
    {
        return;
    }

    QMutexLocker locker(&(hpglData[index.row()]->mutex));
    hpglData[index.row()]->hpgl_items.push_back(poly);
    hpglData[index.row()]->hpgl_items_group->addToGroup(static_cast<QGraphicsItem*>(poly));
}

void hpglListModel::constrainItems(QPointF bottomLeft, QPointF topLeft)
{
    int modCount;

    for (int i = 0; i < hpglData.length(); ++i)
    {
        QPointF pos;
        QRectF rect;
        QGraphicsItemGroup * itemGroup;
        modCount = 0;

        QMutexLocker locker(&(hpglData[i]->mutex));

        itemGroup = hpglData.at(i)->hpgl_items_group;
        pos = itemGroup->pos();
        rect = itemGroup->sceneBoundingRect();

        if (rect.x() < bottomLeft.x())
        {
            ++modCount;
            pos.setX(pos.x() + qFabs(rect.x()));
        }

        if (rect.y() < bottomLeft.y())
        {
            ++modCount;
            pos.setY(pos.y() + qFabs(rect.y()));
        }
        else if ((rect.y() + rect.height()) >
                 topLeft.y())
        {
            ++modCount;
            pos.setY(pos.y() - ((rect.y() + rect.height()) - topLeft.y()));
        }

        if (modCount)
        {
            itemGroup->setPos(pos);
        }
    }
}


































