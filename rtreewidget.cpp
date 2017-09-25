#include "rtreewidget.h"
#include <QMimeData>

RTreeWidget::RTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
    lightItem = new QTreeWidgetItem(this, QStringList(tr("Light")));
    biasItem = new QTreeWidgetItem(this, QStringList(tr("Bias")));
    darkItem = new QTreeWidgetItem(this, QStringList(tr("Dark")));
    flatItem = new QTreeWidgetItem(this, QStringList(tr("Flat")));


}

void RTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void RTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
    QModelIndex index = indexAt(event->pos());
    QTreeWidgetItem *treeItem = itemFromIndex(index);
    setCurrentItem(treeItem);
}

void RTreeWidget::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();

    QModelIndex index = indexAt(event->pos());
    // just in case the selection or mouse hover is off limit
    if (!index.isValid())
    {
        event->setDropAction(Qt::IgnoreAction);
        return;
     }

    QList<QUrl> urls = event->mimeData()->urls();
    QTreeWidgetItem *parentItem = itemFromIndex(index);

    dispatchUrls(parentItem, urls);
}

void RTreeWidget::dispatchUrls(QTreeWidgetItem *parentItem, QList<QUrl> urls)
{

    for (int i = 0 ; i < urls.size() ; i++)
    {
        QTreeWidgetItem *treeItem = new QTreeWidgetItem(parentItem);
        treeItem->setText(0, urls.at(i).fileName());
        qDebug() << "url =" << urls.at(i).toLocalFile();
    }

    QFileInfo filesInfo(urls.at(0).toLocalFile());

    if (parentItem == lightItem)
    {
        qDebug("RTreeWidget:: append to lights");
        lightUrls.append(urls);
        lightsDir = filesInfo.absoluteDir();
    }
    else if (parentItem == biasItem)
    {
        qDebug("RTreeWidget:: append to bias");
        biasUrls.append(urls);
        biasDir = filesInfo.absoluteDir();

    }
    else if (parentItem == darkItem)
    {
        qDebug("RTreeWidget:: append to dark");
        darkUrls.append(urls);
        darkDir = filesInfo.absoluteDir();
    }
    else if (parentItem == flatItem)
    {
        qDebug("RTreeWidget:: append to flat");
        flatUrls.append(urls);
        flatDir = filesInfo.absoluteDir();
        qDebug() << "RTreeWidget:: flatDir (path):" << flatDir.path();
        qDebug() << "RTreeWidget:: flatPath:" << flatDir.filePath(QString("masterFlat.fits"));
    }
}

void RTreeWidget::rMatFromLightRButton(QList<RMat*> rMatImageList)
{
    addItems(lightItem, rMatImageList);
    rMatLightList = rMatImageList;
    lightsDir = rMatImageList.at(0)->getFileInfo().absoluteDir();
}

void RTreeWidget::rMatFromBiasRButton(QList<RMat*> rMatImageList)
{
    addItems(biasItem, rMatImageList);
    rMatBiasList = rMatImageList;
    biasDir = rMatImageList.at(0)->getFileInfo().absoluteDir();

    double min = 0;
    double max = 0;
    cv::minMaxLoc(rMatBiasList.at(0)->matImage, &min, &max);
    qDebug("RTreeWidget::rMatFromBiasRButton:: min =%f , max =%f", min, max );
}

void RTreeWidget::rMatFromDarkRButton(QList<RMat*> rMatImageList)
{
    addItems(darkItem, rMatImageList);
    rMatDarkList = rMatImageList;
    darkDir = rMatImageList.at(0)->getFileInfo().absoluteDir();
}

void RTreeWidget::rMatFromFlatRButton(QList<RMat*> rMatImageList)
{
    addItems(flatItem, rMatImageList);
    rMatFlatList = rMatImageList;
    flatDir = rMatImageList.at(0)->getFileInfo().absoluteDir();
}

void RTreeWidget::removeItem(QTreeWidgetItem *treeItem)
{
    delete treeItem->parent()->takeChild(treeItem->parent()->indexOfChild(treeItem));
}

void RTreeWidget::addItems(QTreeWidgetItem *parentItem, QList<RMat*> rMatImageList)
{
    // This only add treeWidget items based on image titles.
    // It does not populate a global list of urls. This has caused trouble in batch processing
    // The images dropped in the QMdiArea aren't added to the list of urls!
    QList<QUrl> urls;

    for (int i = 0 ; i < rMatImageList.size() ; i++)
    {
        if (rMatImageList.at(i)->getItem() != NULL)
        {
            lightItem->removeChild(rMatImageList.at(i)->getItem());
            biasItem->removeChild(rMatImageList.at(i)->getItem());
            darkItem->removeChild(rMatImageList.at(i)->getItem());
            flatItem->removeChild(rMatImageList.at(i)->getItem());
        }

        QTreeWidgetItem *treeItem = new QTreeWidgetItem(parentItem);
        treeItem->setText(0, rMatImageList.at(i)->getImageTitle());
        rMatImageList[i]->setItem(treeItem);

        urls.append(rMatImageList.at(i)->getUrl());

    }

    QFileInfo filesInfo(urls.at(0).toLocalFile());

    if (parentItem == lightItem)
    {
        qDebug("RTreeWidget:: append to lights");
        lightUrls.append(urls);
        lightsDir = filesInfo.absoluteDir();
    }
    else if (parentItem == biasItem)
    {
        qDebug("RTreeWidget:: append to bias");
        biasUrls.append(urls);
        biasDir = filesInfo.absoluteDir();

    }
    else if (parentItem == darkItem)
    {
        qDebug("RTreeWidget:: append to dark");
        darkUrls.append(urls);
        darkDir = filesInfo.absoluteDir();
    }
    else if (parentItem == flatItem)
    {
        qDebug("RTreeWidget:: append to flat");
        flatUrls.append(urls);
        flatDir = filesInfo.absoluteDir();
    }
}


QList<QUrl> RTreeWidget::getLightUrls() const
{
    return lightUrls;
}

QList<QUrl> RTreeWidget::getBiasUrls() const
{
    return biasUrls;
}

QList<QUrl> RTreeWidget::getDarkUrls() const
{
    return darkUrls;
}

QList<QUrl> RTreeWidget::getFlatUrls() const
{
    return flatUrls;
}

QDir RTreeWidget::getLightsDir() const
{
    return lightsDir;
}

QDir RTreeWidget::getBiasDir() const
{
    return biasDir;
}

QDir RTreeWidget::getDarkDir() const
{
    return darkDir;
}

QDir RTreeWidget::getFlatDir() const
{
    return flatDir;
}

QTreeWidgetItem *RTreeWidget::getLightItem() const
{
    return lightItem;
}

QTreeWidgetItem *RTreeWidget::getBiasItem() const
{
    return biasItem;
}

QTreeWidgetItem *RTreeWidget::getDarkItem() const
{
    return darkItem;
}

QTreeWidgetItem *RTreeWidget::getFlatItem() const
{
    return flatItem;
}



