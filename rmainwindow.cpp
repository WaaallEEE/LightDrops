#include <QMdiSubWindow>
#include <QDropEvent>
#include <QFileDialog>

#include "rmainwindow.h"
#include "ui_rmainwindow.h"
#include "rsubwindow.h"

//QCustomPlot
#include <qcustomplot/qcustomplot.h>


using namespace std;

RMainWindow::RMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RMainWindow),
    currentROpenGLWidget(NULL),
    resultROpenGLWidget(NULL),
    limbFittingROpenGLWidget(NULL),
    graphicsView(NULL),
    limbRegisterSubWindow(NULL),
    currentSubWindow(NULL), plotSubWindow(NULL), customPlot(NULL), tempSubWindow(NULL),
    toneMappingGraph(NULL), previewQImage(NULL)
{
    ui->setupUi(this);

    //QRect rec = QApplication::desktop()->screenGeometry();

    this->showMaximized();
    setCentralWidget(ui->mdiArea);
    processing = new RProcessing(this);

    vertLineHigh = NULL;
    defaultWindowSize = QSize(512, 512);

    sliderScale = 1.0;
    sliderToScaleMinimum = 0;

    limbSliderScale = 1.0;
    limbSliderToScaleMinimum = 0;

    gammaScale = 0.1;
    gammaMin = ui->doubleSpinBoxGamma->minimum();
    gamma = 1.0f;

    sliderScaleWB = 0.01;
    frameIndex = 0;

    setupSubImage();

//    tabifyDockWidget(ui->currentFrameDock, ui->settingsDock);
//    this->setTabPosition(Qt::DockWidgetArea::BottomDockWidgetArea, QTabWidget::TabPosition::South);

    //this->setTabShape(QTabWidget::Triangular);

    connect(this, SIGNAL(tempMessageSignal(QString,int)), this->statusBar(), SLOT(showMessage(QString,int)));

    /// Connect the change of slider value to the number displayed in the spinBox
    connect(ui->sliderHigh, SIGNAL(valueChanged(int)), this, SLOT(updateDoubleSpinBox(int)));
    connect(ui->sliderLow, SIGNAL(valueChanged(int)), this, SLOT(updateDoubleSpinBox(int)));
    connect(ui->sliderGamma, SIGNAL(valueChanged(int)), this, SLOT(updateDoubleSpinBox(int)));
    /// Limb sliders
    connect(ui->limbSliderHigh, SIGNAL(valueChanged(int)), this, SLOT(updateDoubleSpinBox(int)));
    connect(ui->limbSliderLow, SIGNAL(valueChanged(int)), this, SLOT(updateDoubleSpinBox(int)));
    connect(ui->limbSliderGamma, SIGNAL(valueChanged(int)), this, SLOT(updateDoubleSpinBox(int)));
    /// Connect the change of the number in the spinBox to the change of slider value
    ///Use editingFinished() instead of valueChanged(double()) to avoid unwanted feedback on the slider.
    connect(ui->doubleSpinBoxHigh, SIGNAL(editingFinished()), this, SLOT(updateSliderValueSlot()));
    connect(ui->doubleSpinBoxLow, SIGNAL(editingFinished()), this, SLOT(updateSliderValueSlot()));
    connect(ui->doubleSpinBoxGamma, SIGNAL(editingFinished()), this, SLOT(updateSliderValueSlot()));

    /// Connect the change of slider value to the image linear scaling and gamma scaling.
    connect(ui->sliderHigh, SIGNAL(valueChanged(int)), this, SLOT(scaleImageSlot(int)));
    connect(ui->sliderLow, SIGNAL(valueChanged(int)), this, SLOT(scaleImageSlot(int)));
    connect(ui->sliderGamma, SIGNAL(valueChanged(int)), this, SLOT(gammaScaleImageSlot(int)));

    connect(ui->limbSliderHigh, SIGNAL(valueChanged(int)), this, SLOT(scaleImageSlot(int)));
    connect(ui->limbSliderLow, SIGNAL(valueChanged(int)), this, SLOT(scaleImageSlot(int)));
    connect(ui->limbSliderGamma, SIGNAL(valueChanged(int)), this, SLOT(gammaScaleImageSlot(int)));

    /// Connect the white balance sliders
    connect(ui->redSlider, SIGNAL(valueChanged(int)), this, SLOT(updateWB(int)));
    connect(ui->blueSlider, SIGNAL(valueChanged(int)), this, SLOT(updateWB(int)));
    connect(ui->greenSlider, SIGNAL(valueChanged(int)), this, SLOT(updateWB(int)));

    /// Connect the pushButton and lineEdit of the ui for creating the masters in the processing class
    connect(ui->makeMasterButton, SIGNAL(pressed()), processing, SLOT(createMasters()));
    connect(ui->masterMeanRButton, SIGNAL(clicked(bool)), processing, SLOT(setupMasterWithMean(bool)));
    connect(ui->MasterSigmaClipRButton, SIGNAL(clicked(bool)), processing, SLOT(setupMasterWithSigmaClip(bool)));

    connect(ui->calibrateDirLineEdit, SIGNAL(textChanged(QString)), processing, SLOT(setExportCalibrateDir(QString)));

    /// Connect the sliderFrame and playback buttons
    connect(ui->sliderFrame, SIGNAL(valueChanged(int)), this, SLOT(updateFrameInSeries(int)));
    connect(ui->forwardButton, SIGNAL(released()), this, SLOT(stepForward()));
    connect(ui->backwardButton, SIGNAL(released()), this, SLOT(stepBackward()));
    connect(ui->playButton, SIGNAL(released()), this, SLOT(playForward()));
    connect(ui->stopButton, SIGNAL(released()), this, SLOT(stopButtonPressed()));

    /// Connect processing outputs to the main ui.
    connect(processing, SIGNAL(tempMessageSignal(QString,int)), this->statusBar(), SLOT(showMessage(QString,int)));
    connect(processing, SIGNAL(resultSignal(RMat*)), this, SLOT(createNewImage(RMat*)));
    connect(processing, SIGNAL(resultSignal(cv::Mat, bool, instruments)), this, SLOT(createNewImage(cv::Mat, bool, instruments)));
    connect(processing, SIGNAL(resultSignal(cv::Mat, bool, instruments, QString)), this, SLOT(createNewImage(cv::Mat, bool, instruments, QString)));
    connect(processing, SIGNAL(listResultSignal(QList<RMat*>)), this, SLOT(createNewImage(QList<RMat*>)));

    /// Connect ui radio button to tree widget to send out window titles.
    connect(ui->lightRButton, SIGNAL(clicked(bool)), this, SLOT(radioRMatSlot()));
    connect(ui->biasRButton, SIGNAL(clicked(bool)), this, SLOT(radioRMatSlot()));
    connect(ui->darkRButton, SIGNAL(clicked(bool)), this, SLOT(radioRMatSlot()));
    connect(ui->flatRButton, SIGNAL(clicked(bool)), this, SLOT(radioRMatSlot()));

    // Connect the radioLight, radioBias,... signals to the treeWidget
    connect(this, SIGNAL(radioLightImages(QList<RMat*>)), ui->treeWidget, SLOT(rMatFromLightRButton(QList<RMat*>)));
    connect(this, SIGNAL(radioBiasImages(QList<RMat*>)), ui->treeWidget, SLOT(rMatFromBiasRButton(QList<RMat*>)));
    connect(this, SIGNAL(radioDarkImages(QList<RMat*>)), ui->treeWidget, SLOT(rMatFromDarkRButton(QList<RMat*>)));
    connect(this, SIGNAL(radioFlatImages(QList<RMat*>)), ui->treeWidget, SLOT(rMatFromFlatRButton(QList<RMat*>)));

    // Connect QImage scenes
    connect(processing, SIGNAL(resultQImageSignal(QImage&)), this, SLOT(createNewImage(QImage&)));

    //connect(processing, SIGNAL(ellipseSignal(cv::RotatedRect)), this, SLOT(addEllipseToScene(cv::RotatedRect)));

    // Connect Fit stats button
    connect(ui->plotFitStatsButton, SIGNAL(released()), this, SLOT(showLimbFitStats()));
    // Connect Tone mapping button
    connect(ui->toneMappingButton, SIGNAL(released()), this, SLOT(setupToneMappingCurve()));
    connect(ui->toneMappingSlider1, SIGNAL(valueChanged(int)), this, SLOT(updateToneMappingSlot()));
    connect(ui->toneMappingSlider2, SIGNAL(valueChanged(int)), this, SLOT(updateToneMappingSlot()));
    connect(ui->toneMappingSlider3, SIGNAL(valueChanged(int)), this, SLOT(updateToneMappingSlot()));
    connect(ui->applyToneMappingCheckBox, SIGNAL(toggled(bool)), this, SLOT(applyToneMappingCurve()));
    connect(ui->HAlphaPButton, SIGNAL(released()), this, SLOT(hAlphaToneMappingSlot()));
    connect(ui->scaleLimbCheckBox, SIGNAL(toggled(bool)), this, SLOT(applyScaleLimbSlot()));

    // Fourier Filters
   qDebug() << "mdiArea->size =" << ui->mdiArea->size();

   connect(ui->sharpenSliderW1, SIGNAL(sliderMoved(int)), this, SLOT(sharpenSliderSlot()));

//   connect(ui->sharpenPButton, SIGNAL(released()), this, SLOT(sharpenImageSlot()));

}

RMainWindow::~RMainWindow()
{

    delete processing;
    delete ui;
}

void RMainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();

    if (!mimeData->hasUrls())
        return;

    event->acceptProposedAction();

    RListImageManager *newRListImageManager = new RListImageManager(mimeData->urls());
    if (newRListImageManager->getRMatImageList().empty())
    {
        qDebug("Nothing to load: wrong file extention?");
        return;
    }

    createNewImage(newRListImageManager);

}

void RMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void RMainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void RMainWindow::closeEvent(QCloseEvent *event)
{

    ui->mdiArea->closeAllSubWindows();
    event->accept();
}

void RMainWindow::createNewImage(RListImageManager *newRListImageManager)
{
    currentROpenGLWidget = new ROpenGLWidget(newRListImageManager, this);
    currentROpenGLWidget->setDisplayOnlyFirst(ui->displayFirstCheckBox->isChecked());
    addImage(currentROpenGLWidget);
    displayPlotWidget(currentROpenGLWidget);
    currentSubWindow->show();
    setupSliders(currentROpenGLWidget);
    autoScale(currentROpenGLWidget);

    //dispatchRMatImages(currentROpenGLWidget->getRMatImageList());

    //Default is to add as Lights
    processing->rMatLightList = newRListImageManager->getRMatImageList();
    // It can be argued that both below should be linked together. For flexibility with non-url images (e.g. a result of processing)
    // I keep it separate for now. One can create a function that encompasses these two of course.
    ui->treeWidget->rMatFromLightRButton(newRListImageManager->getRMatImageList());
    //ui->treeWidget->dispatchUrls(ui->treeWidget->getLightItem(), newRListImageManager->getUrlList());
    // Connect it to the removal of that item when the ROpenGLWidget is closed

    //RMat* currentRMat = newRListImageManager->getRMatImageList().at(0);
    //connect(currentRMat, SIGNAL(itemSignal(QTreeWidgetItem*)), this, SLOT(removeTreeWidgetItems(QTreeWidgetItem*)));


    ///--- Test of Fourier Filtering ---///
//    cv::Mat matImage = currentROpenGLWidget->getRMatImageList().at(0)->matImage.clone();
//    cv::Mat matImageHPF = processing->makeImageHPF(matImage, 30);
//    createNewImage(matImageHPF, false, instruments::generic, QString("Filtered"));
//    currentROpenGLWidget->setAttribute(Qt::WA_DeleteOnClose, true);
    currentSubWindow->setAttribute(Qt::WA_DeleteOnClose, true);

    connect(currentROpenGLWidget, SIGNAL(treeWidgetSignal(QList<RMat*>)), ui->treeWidget, SLOT(cleanup(QList<RMat*>)));
}



void RMainWindow::createNewImage(QList<RMat*> newRMatImageList)
{

    currentROpenGLWidget = new ROpenGLWidget(newRMatImageList, this);
    addImage(currentROpenGLWidget);
    displayPlotWidget(currentROpenGLWidget);
    currentSubWindow->show();
    setupSliders(currentROpenGLWidget);
    //currentROpenGLWidget->setAttribute(Qt::WA_DeleteOnClose, true);

    // Testing interpolation routine


}

void RMainWindow::createNewImage(RMat *rMatImage)
{
    currentROpenGLWidget = new ROpenGLWidget(rMatImage, this);
    addImage(currentROpenGLWidget);
    displayPlotWidget(currentROpenGLWidget);
    currentSubWindow->show();
    setupSliders(currentROpenGLWidget);
    autoScale(currentROpenGLWidget);
}


void RMainWindow::createNewImage(cv::Mat cvImage, bool bayer, instruments instrument, QString imageTitle)
{
    RMat *rMatImage = new RMat(cvImage, bayer, instrument);
    rMatImage->setImageTitle(imageTitle);
    createNewImage(rMatImage);
}

void RMainWindow::createNewImage(QImage &image, ROpenGLWidget *rOpenGLWidget, bool inverted)
{
    //QSize currentSize = ui->mdiArea->currentSubWindow()->size();
    QPixmap pixMap = QPixmap::fromImage(image);
    QGraphicsScene *newScene = new QGraphicsScene;
    newScene->addPixmap(pixMap);

    graphicsView= new QGraphicsView(newScene);

    if (rOpenGLWidget != NULL)
    {
        float scale = rOpenGLWidget->getResizeFac();
        if (inverted)
        {
            graphicsView->scale(scale, -scale);
        }
        else
        {
            graphicsView->scale(scale, scale);
        }

    }

    QMdiSubWindow *newSubWindow = new QMdiSubWindow;
    newSubWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    //newSubWindow->setAttribute(Qt::WA_AcceptTouchEvents, false);
    newSubWindow->setWidget(graphicsView);
    newSubWindow->setWindowTitle(QString("QImage"));
    ui->mdiArea->addSubWindow(newSubWindow);

    newSubWindow->show();

}



void RMainWindow::displayQImage(QImage &image, RGraphicsScene *scene, QMdiSubWindow *subWindow, QString windowTitle)
{
    QPixmap pixMap = QPixmap::fromImage(image);

    qDebug("RMainWindow::createNewImage(QImage &image):: pixMap.size() = [%i ; %i]", pixMap.size().width(), pixMap.size().height());
    //QPainter *painter = new QPainter(&image);

    //if (graphicsView == NULL)
    //{
        QSize lastSize = ui->mdiArea->currentSubWindow()->size();
        QPoint lastPos = ui->mdiArea->currentSubWindow()->pos();

        pixMapItem = scene->addPixmap(pixMap);
        // QPointF QGraphicsSceneMouseEvent::lastScenePos()
        //scene->mouseGrabberItem()

        QGraphicsView *newGraphicsView= new QGraphicsView(scene);
        //graphicsView->scale(1, -1);
        newGraphicsView->setDragMode(QGraphicsView::NoDrag);
        newGraphicsView->scale(currentROpenGLWidget->getResizeFac(), currentROpenGLWidget->getResizeFac());
        newGraphicsView->setCursor(Qt::CrossCursor);

        subWindow->setWidget(newGraphicsView);
        ui->mdiArea->addSubWindow(subWindow);
        subWindow->show();
        subWindow->resize(lastSize);
        subWindow->move(lastPos);
        subWindow->setWindowTitle(windowTitle);
        currentSubWindow = subWindow;

    //}
//    else
//    {
//        QSize lastSize = ui->mdiArea->currentSubWindow()->size();
//        QPoint lastPos = ui->mdiArea->currentSubWindow()->pos();

//        pixMapItem->setPixmap(pixMap);

//        QMdiSubWindow *sceneSubWindow = new QMdiSubWindow;
//        sceneSubWindow->setWidget(graphicsView);
//        ui->mdiArea->addSubWindow(sceneSubWindow);
//        sceneSubWindow->show();
//        sceneSubWindow->resize(lastSize);
//        sceneSubWindow->move(lastPos);
//        sceneSubWindow->setWindowTitle(windowTitle);
//    }


}

void RMainWindow::selectROI(bool isSquare, int blkSize)
{
    if (currentROpenGLWidget == NULL)
    {
        ui->actionROISelect->setChecked(false);
        return;
    }


    lastROpenGLWidget = currentROpenGLWidget;
    currentROpenGLWidget->setRoiSelected(true);

//    /// Use copy constructor of the RMat to copy the current RMat image.
//    RMat tempRMat(*currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value()));
//    // Setup image display with QImage and not openGL to allow easier drawings
//    //cv::Mat matImage = processing->normalizeByThresh(tempRMat.matImageGray, tempRMat.getIntensityLow(), tempRMat.getIntensityHigh(), tempRMat.getDataRange());
//    cv::Mat matImageROI;// = matImage; //(ROI);
//    tempRMat.matImageGray.convertTo(matImageROI, CV_8U, 255.0f/tempRMat.getDataMax());
//    QImage imageROI((const uchar *) matImageROI.data, matImageROI.cols, matImageROI.rows, matImageROI.step, QImage::Format_Grayscale8);
//    QImage targetImage = imageROI.mirrored(false, true);
//    // Instantiate an RGraphicsScene. It allows to draw rectangles with the mouse for ROI selection
//    RGraphicsScene *roiScene = new RGraphicsScene();
//    if (isSquare)
//    {
//        roiScene->squareMode = isSquare;
//        roiScene->blkSize = blkSize;
//    }
//    QMdiSubWindow *roiSubWindow = new QMdiSubWindow();
//    roiSubWindow->setAttribute(Qt::WA_DeleteOnClose, true);
//    displayQImage(targetImage, roiScene, roiSubWindow, QString("Select ROI"));

//    connect(roiScene, SIGNAL(ROIsignal(QRect)), this, SLOT(setRect(QRect)));
//    connect(roiSubWindow, SIGNAL(destroyed(QObject*)), this, SLOT(disableROIaction()));

    // the ROpenGLWidget should send the selection rectangle once the selection is done
    connect(currentROpenGLWidget, SIGNAL(roiSignal(QRect)), this, SLOT(setRect(QRect)));

}



void RMainWindow::setRect(QRect rect)
{
    //qDebug("RMainWindow::setRect() rect.x(), rect.y(), rect.width(), rect.height() = [%i, %i, %i, %i]", rect.x(), rect.y(), rect.width(), rect.height());
    this->rect = rect;
    cv::Rect cvRectROI(rect.x(), rect.y(), rect.width(), rect.height());
    processing->setCvRectROI(cvRectROI);
    ui->xROIBox->setValue(rect.x());
    ui->yROIBox->setValue(rect.y());
    ui->widthROIBox->setValue(rect.width());
    ui->heightROIBox->setValue(rect.height());

}

void RMainWindow::extractNewImageROI()
{

    if (currentROpenGLWidget == NULL)
    {
        ui->actionROIExtract->setCheckable(false);
        return;
    }

    if (rect.isEmpty())
    {
        ui->actionROIExtract->setCheckable(false);
        return;
    }

    currentROpenGLWidget = lastROpenGLWidget;
    cv::Rect cvRectROI(rect.x(), rect.y(), rect.width(), rect.height());

    QList<RMat *> roiList;
    for (int i=0; i < currentROpenGLWidget->getRMatImageList().size(); i++)
    {
        cv::Mat matImage = currentROpenGLWidget->getRMatImageList().at(i)->matImage;
        cv::Mat matImageROI = matImage(cvRectROI);
        // Bayer status of the extracted image should be the same as the original image
        // Actually, one cannot really extract a bayer image. It has to be debayerized otherwise we mess up the bayer matrix (cut through it)
        RMat *tempRMat = new RMat(matImageROI, currentROpenGLWidget->getRMatImageList().at(i)->isBayer(), currentROpenGLWidget->getRMatImageList().at(i)->getInstrument());
        tempRMat->setImageTitle(QString("ROI "));
        roiList << tempRMat;
    }

    createNewImage(roiList);
}

void RMainWindow::disableROIaction()
{
    ui->actionROISelect->setChecked(false);
}

//void RMainWindow::removeTreeWidgetItems(QTreeWidgetItem* item)
//{
//    ui->treeWidget->removeItem(item);
//}

//void RMainWindow::addEllipseToScene(cv::RotatedRect rect)
//{
////    QGraphicsEllipseItem * QGraphicsScene::addEllipse(qreal x, qreal y, qreal w, qreal h, const QPen & pen = QPen(), const QBrush & brush = QBrush())
//    QPen pen;
//    pen.setColor(Qt::green);
//    QGraphicsEllipseItem *ellipseItem = new QGraphicsEllipseItem(0, 0, rect.size.width, rect.size.height);
//    ellipseItem->setPen(pen);
//    //ellipseItem->setScale(-currentROpenGLWidget->getResizeFac());
//    scene->addItem(ellipseItem);
//    qDebug("RMainWindow::addEllipseToScene():: rect.size.width = %f", rect.size.width);
//}

void RMainWindow::dispatchRMatImages(QList<RMat*> rMatList)
{
    emit radioLightImages(rMatList);
    ui->treeWidget->rMatLightList = rMatList;
    processing->rMatLightList = rMatList;

    ui->treeWidget->rMatFromLightRButton(rMatList);
//    // Try to guess the calibration type (Light, Bias, Dark, or Flat)
//    // with the file names, or directory name, etc... and "radio" it out as if we were pressing the corresponding radio button
//    // to assign it to the corresponding QTreeWidgetItem in the treeWidget.

//    bool noBias = !(rMatList.at(0)->getImageTitle().contains(QString("bias"), Qt::CaseInsensitive) |
//                   rMatList.at(0)->getImageTitle().contains(QString("offset"), Qt::CaseInsensitive) |
//                   rMatList.at(0)->getFileInfo().absolutePath().contains(QString("bias"), Qt::CaseInsensitive));

//    bool noDark = !(rMatList.at(0)->getImageTitle().contains(QString("dark"), Qt::CaseInsensitive) |
//                   rMatList.at(0)->getFileInfo().absolutePath().contains(QString("dark"), Qt::CaseInsensitive));

//    bool noFlat = !(rMatList.at(0)->getImageTitle().contains(QString("flat"), Qt::CaseInsensitive) |
//                   rMatList.at(0)->getFileInfo().absolutePath().contains(QString("flat"), Qt::CaseInsensitive));

//    bool isAmbiguous = rMatList.at(0)->getFileInfo().absolutePath().contains(QString("light"), Qt::CaseInsensitive);

//    bool isBias = (!noBias & noDark & noFlat & !isAmbiguous) | rMatList.at(0)->getImageTitle().contains(QString("bias"), Qt::CaseInsensitive) |  rMatList.at(0)->getImageTitle().contains(QString("offset"), Qt::CaseInsensitive);
//    bool isDark = (!noDark & noBias & noFlat & !isAmbiguous) | rMatList.at(0)->getImageTitle().contains(QString("dark"), Qt::CaseInsensitive) ;
//    bool isFlat = (!noFlat & noDark & noBias & !isAmbiguous) | rMatList.at(0)->getImageTitle().contains(QString("flat"), Qt::CaseInsensitive) ;


//    if (isBias)
//    {
//        ui->biasRButton->click();
//    }
//    else if (isDark)
//    {
//        ui->darkRButton->click();
//    }
//    else if (isFlat)
//    {
//        ui->flatRButton->click();
//    }

}

void RMainWindow::addImage(ROpenGLWidget *rOpenGLWidget)
{

    ui->sliderFrame->blockSignals(true);

    ui->sliderFrame->setRange(0, rOpenGLWidget->getRMatImageList().size()-1);
    ui->sliderFrame->setValue(0);
    ui->imageLabel->setText(QString("1") + QString("/") + QString::number(rOpenGLWidget->getRMatImageList().size()));

    currentScrollArea = new QScrollArea();
    currentScrollArea->setWidget(rOpenGLWidget);
    //currentScrollArea->setAttribute(Qt::WA_DeleteOnClose, true);
    /// The size of the scrollArea must be set to the size of whatever is needed for the ROpenGLWidget
    /// to display the whole FOV in the central widget when adding the height of the scroll bars.
    /// Note that the "height" of the scrollBar always represent the dimension perpendicular to the direction
    /// of the slider.
    resizeScrollArea(rOpenGLWidget, currentScrollArea); // Resize scrollArea only
    loadSubWindow(currentScrollArea); // also resize the current ROpenGLWidget. Poor design choice.
    rOpenGLWidget->update();


    processing->setCurrentROpenGLWidget(rOpenGLWidget);

    ui->sliderFrame->blockSignals(false);

    connect(rOpenGLWidget, SIGNAL(gotSelected(ROpenGLWidget*)), this, SLOT(changeROpenGLWidget(ROpenGLWidget*)));
    connect(rOpenGLWidget, SIGNAL(sendSubQImage(QImage*,float,int,int)), this, SLOT(updateSubFrame(QImage*,float,int,int)));

    // Update stats
    updateStats(0);
}


void RMainWindow::resizeScrollArea(ROpenGLWidget *rOpenGLWidget, QScrollArea *scrollArea)
{

    /// Need to leave just enough space for the scroll bars. So we have to remove the scrollBarHeight from the
    /// the size of ROpenGLWidget to make sure the latter fits tightly in the QScrollArea.
    int naxis1 = rOpenGLWidget->getRMatImageList().at(0)->matImage.cols;
    int naxis2 = rOpenGLWidget->getRMatImageList().at(0)->matImage.rows;

    float widthFac = (float) ((this->centralWidget()->width() - scrollArea->horizontalScrollBar()->height()) / (float) naxis1);
    float heightFac = (float) ((this->centralWidget()->height() - scrollArea->horizontalScrollBar()->height()) / (float) naxis2);
    float resizeFac = 1;

    if (widthFac > 1 && heightFac > 1)
    {
        qDebug("RMainWindow::resizeScrollArea():: Keeping original image size");
        //then default size = image size
        oglSize = QSize(naxis1, naxis2);
    }
    else // set other default size, smaller than original image size
    {
        qDebug("RMainWindow::resizeScrollArea():: resizing image");
        resizeFac = std::min(widthFac, heightFac);
        oglSize = QSize((int) ((float) naxis1 * resizeFac), (int) ((float) naxis2 * resizeFac));
    }

    rOpenGLWidget->setResizeFac(resizeFac);
    scrollArea->resize(oglSize + QSize(5, 25));


//    oglSize = QSize(naxis1, naxis2);
//    rOpenGLWidget->setResizeFac(1);
//    scrollArea->resize(QSize(600, 600) + QSize(5, 25));

}

void RMainWindow::loadSubWindow(QScrollArea *scrollArea)
{
    currentSubWindow = new QMdiSubWindow;
    //currentSubWindow->setAttribute(Qt::WA_DeleteOnClose, true);
    currentSubWindow->setWidget(scrollArea);
    ui->mdiArea->addSubWindow(currentSubWindow);
    currentSubWindow->setWindowTitle(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getImageTitle() + QString(" #%1 ").arg(frameIndex+1));

    /// ROpenGLWidget can only be resized once it has been assigned to the QMdiSubWindow.
    /// Otherwise the shaders will not be bound.
    currentROpenGLWidget->resize(oglSize);
    currentSubWindow->resize(scrollArea->size());
    //currentSubWindow->setAttribute(Qt::WA_DeleteOnClose, true);

}



void RMainWindow::updateDoubleSpinBox(int value)
{
    /// The value of sliders cannot be passed-through "as is"
    /// to the SpinBox. The latter need to show numbers in
    /// the "units" of the image intensity.

    if (this->sender() == ui->sliderHigh)
    {
        double valueF = (double) convertSliderToScale(value);
        ui->doubleSpinBoxHigh->setValue(valueF);
        qDebug("updateDoubleSpinBox:: (high) valueF = %f", valueF);
    }
    else if (this->sender() == ui->sliderLow)
    {
        float valueF = convertSliderToScale(value);
        ui->doubleSpinBoxLow->setValue(valueF);
    }
    if (this->sender() == ui->sliderGamma)
    {
        float valueF = convertSliderToGamma(value);
        ui->doubleSpinBoxGamma->setValue(valueF);
    }
    /// Limb sliders
    if (this->sender() == ui->limbSliderHigh)
    {
        float valueF = convertLimbSliderToScale(value);
        ui->limbDoubleSpinBoxHigh->setValue(valueF);
        qDebug("updateDoubleSpinBox:: (high) valueF = %f", valueF);
    }
    else if (this->sender() == ui->limbSliderLow)
    {
        float valueF = convertLimbSliderToScale(value);
        ui->limbDoubleSpinBoxLow->setValue(valueF);
    }
    else if (this->sender() == ui->limbSliderGamma)
    {
        float valueF = convertSliderToGamma(value);
        ui->limbDoubleSpinBoxGamma->setValue(valueF);
    }


}

void RMainWindow::scaleImageSlot(int value)
{
    std::cout<< "scaleImageSlot" << std::endl;

    if (currentROpenGLWidget == NULL)
    {
        std::cout<< "currentROpenGLWidget is NULL. Returning..." << std::endl;
        return;
    }

    if (this->sender() == ui->sliderHigh)
    {
        std::cout<< "scaleImageSlot from sliderHigh" << std::endl;
        currentROpenGLWidget->setNewMax(convertSliderToScale(value));
    }
    else if (this->sender() == ui->sliderLow)
    {
        currentROpenGLWidget->setNewMin(convertSliderToScale(value));
    }
    else if (this->sender() == ui->limbSliderHigh)
    {   /// Limb
        currentROpenGLWidget->setLimbNewMax(convertSliderToScale(value));
    }
    else if (this->sender() == ui->limbSliderLow)
    {   /// Limb
        currentROpenGLWidget->setLimbNewMin(convertSliderToScale(value));
    }
    else
    {
        std::cout<< "scaleImageSlot sender not recognized" << std::endl;
    }

    if (currentROpenGLWidget->getNewMin() == currentROpenGLWidget->getNewMax())
    {
        std::cout<< "currentROpenGLWidget->newMin == newMax. Returning... " << std::endl;
        return;
    }
    else if ((this->sender() == ui->limbSliderHigh || this->sender() == ui->limbSliderLow) && currentROpenGLWidget->getLimbNewMin() == currentROpenGLWidget->getLimbNewMax())
    {
        return;
    }

    std::cout<< "updateCurrentROpenGLWidget()" << std::endl;
    updateCurrentROpenGLWidget();
}


void RMainWindow::gammaScaleImageSlot(int value)
{
    if (currentROpenGLWidget == NULL)
        return;

    gamma = convertSliderToGamma(value);

    if (this->sender() == ui->sliderGamma)
    {
        qDebug("RMainWindow::gammaScaleImageSlot() value =%i, gamma = %f", value, gamma);
        currentROpenGLWidget->setGamma(gamma);
        currentROpenGLWidget->update();
    }
    else if (this->sender() == ui->limbSliderGamma)
    {
        currentROpenGLWidget->setLimbGamma(gamma);
        currentROpenGLWidget->update();
    }

}

void RMainWindow::setupSliders(ROpenGLWidget* rOpenGLWidget)
{
    // The slider range of integer values must be consistent with the maximum data range.
    // For USET for example, it is 4096.
    // We also need to define the number of decimals in the spinBox High and Low.
    int decimals = 0;
//    ui->sliderHigh->blockSignals(true);
//    ui->sliderLow->blockSignals(true);
//    ui->limbSliderHigh->blockSignals(true);
//    ui->limbSliderLow->blockSignals(true);

    int sliderMax = ui->sliderHigh->maximum();
    int sliderMin = ui->sliderHigh->minimum();
    sliderRange =  sliderMax - sliderMin + 1;

    float dataMin = processing->fetchRMatSeriesMin(rOpenGLWidget->getRMatImageList());
    float dataMax = processing->fetchRMatSeriesMax(rOpenGLWidget->getRMatImageList());
    float dataRange = dataMax - dataMin + 1;
//    std::cout << "dataMin = " << dataMin << std::endl;
//    std::cout << "dataMax = " << dataMax << std::endl;

    // Affine equation y = p + s*x (y: scaled value, s: slope, p:intercept, x: slider value)
    // <=>   scaledValue = sliderToScaleMininum + sliderScale * "slider Value"
    // Slope:

      sliderScale = dataRange / sliderRange;
      sliderToScaleMinimum = dataMin - sliderScale*sliderMin;

//    std::cout << "sliderScale = " << sliderScale << std::endl;
//    std::cout << "sliderToScaleMinimum = " << sliderToScaleMinimum << std::endl;

//    std::cout << "sliderScale2 = " << sliderScale2 << std::endl;
//    std::cout << "sliderToScaleMinimum2 = " << sliderToScaleMinimum2 << std::endl;

    if ( rOpenGLWidget->getRMatImageList().at(0)->getInstrument() == instruments::DSLR &&
        (rOpenGLWidget->getRMatImageList().at(0)->matImage.type() != CV_32F && rOpenGLWidget->getRMatImageList().at(0)->matImage.type() != CV_32FC3) )
    {

        ui->limbSliderHigh->setRange(1, sliderRange);
        ui->limbSliderLow->setRange(1, sliderRange);

        limbSliderScale = 1.0;
        limbSliderToScaleMinimum = 0;

    }
    else
    {
        /// Here the image is assumed to be seen as scientific data
        /// for which scaling needs to be tightly set around the min and max

        //sliderRange = 65536;
//        ui->sliderHigh->setRange(1, sliderRange);
//        ui->sliderLow->setRange(1, sliderRange);

//        float dataMax = rOpenGLWidget->getRMatImageList().at(0)->getDataMax();
//        float dataMin = rOpenGLWidget->getRMatImageList().at(0)->getDataMin();
//        float dataRange = dataMax - dataMin;
//        sliderScale = (float) dataRange / sliderRange;
//        sliderToScaleMinimum = dataMin;



        limbSliderScale = 1.0;
        limbSliderToScaleMinimum = dataMin;

        /// update the number of decimals in the spinBox High and Low
        /// This is just to remind visually that we're dealing with floating points
        /// and not integers.
        decimals = 2;
    }


    ui->sliderHigh->blockSignals(false);
    ui->sliderLow->blockSignals(false);

    ui->limbSliderHigh->blockSignals(false);
    ui->limbSliderLow->blockSignals(false);

    ui->doubleSpinBoxHigh->blockSignals(true);
    ui->doubleSpinBoxLow->blockSignals(true);

//    ui->doubleSpinBoxHigh->setMaximum(convertSliderToScale(ui->sliderHigh->maximum()));
//    ui->doubleSpinBoxHigh->setMinimum(convertSliderToScale(ui->sliderHigh->minimum()));
    ui->doubleSpinBoxHigh->setDecimals(decimals);
//    ui->doubleSpinBoxLow->setMaximum(convertSliderToScale(ui->sliderLow->maximum()));
//    ui->doubleSpinBoxLow->setMinimum(convertSliderToScale(ui->sliderLow->minimum()));
    ui->doubleSpinBoxLow->setDecimals(decimals);

    ui->doubleSpinBoxHigh->blockSignals(false);
    ui->doubleSpinBoxLow->blockSignals(false);

    /// setup limbSliderHigh and limbSliderLow

    ui->limbDoubleSpinBoxHigh->blockSignals(true);
    ui->limbDoubleSpinBoxLow->blockSignals(true);

    ui->limbDoubleSpinBoxHigh->setMaximum(convertLimbSliderToScale(ui->limbSliderHigh->maximum()));
    ui->limbDoubleSpinBoxHigh->setMinimum(convertLimbSliderToScale(ui->limbSliderHigh->minimum()));
    ui->limbDoubleSpinBoxHigh->setDecimals(decimals);
    ui->limbDoubleSpinBoxLow->setMaximum(convertLimbSliderToScale(ui->limbSliderLow->maximum()));
    ui->limbDoubleSpinBoxLow->setMinimum(convertLimbSliderToScale(ui->limbSliderLow->minimum()));
    ui->limbDoubleSpinBoxLow->setDecimals(decimals);

    ui->limbDoubleSpinBoxHigh->blockSignals(false);
    ui->limbDoubleSpinBoxLow->blockSignals(false);

}

void RMainWindow::setupSubImage()
{
    subQImage = new QImage(ui->subFrame->getNaxis1(), ui->subFrame->getNaxis2(), QImage::Format_ARGB32);
    subQImage->fill(QColor(125, 125, 125, 125));

    ui->subFrame->setImage(subQImage);
    ui->subFrame->setDrawCross(true);
    ui->subFrame->update();

    //subPaintWidget = new PaintWidget(subQImage, subMagnification);
    //subPaintWidget->setDrawCross(true);


}

void RMainWindow::updateInvGaussianParams()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    double fac1=1.0;
    double fac2=1.0;
    double fac3=1.0;

    int matType = currentROpenGLWidget->getRMatImageList().at(0)->matImage.type();
    if (matType == CV_32F || matType == CV_16U)
    {
        fac1 = 0.5 / (double) ui->toneMappingSlider1->maximum();
        fac2 = 5.0 / (double) ui->toneMappingSlider2->maximum();
        fac3 = 1.0 / (double) ui->toneMappingSlider3->maximum();
    }
    else if (matType == CV_8U)
    {
        fac1 = 10000.0 / (double) ui->toneMappingSlider1->maximum();
        fac2 = 2000.0 / (double) ui->toneMappingSlider2->maximum();
        fac3 = 255.0 / (double) ui->toneMappingSlider3->maximum();
    }

    iMax = (double) ui->toneMappingSlider1->value() * fac1;
    lambda = (double) ui->toneMappingSlider2->value() * fac2;
    mu = (double) ui->toneMappingSlider3->value() * fac3;
}

void RMainWindow::initPreviewQImage(bool status)
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    if (status)
    {
        cv::Mat matImage = convertTo8Bit(currentROpenGLWidget->getRMatImageList().at(frameIndex));

        if (matImage.channels() == 1)
        {
           qDebug("RMainWindow::initPreviewQImage() Image is gray scaled");
           previewQImage = QImage(matImage.data, matImage.cols, matImage.rows, QImage::Format_Grayscale8);
        }
        else
        {
            if (currentROpenGLWidget->getRMatImageList().at(frameIndex)->flipUD)
            {
                qDebug("RMainWindow::initPreviewQImage() Image is RGB and TIFF. Flipping.");
                cv::flip(matImage, matImage, 0);
            }
            previewQImage = QImage((const uchar *) matImage.data, matImage.cols, matImage.rows, matImage.step, QImage::Format_RGB888);
        }

        createNewImage(previewQImage, currentROpenGLWidget, true);
        previewSubWindow = ui->mdiArea->activeSubWindow();
    }
    else
    {
        previewSubWindow->close();
        previewSubWindow = NULL;
    }


}


void RMainWindow::solarLimbFit()
{
//    if (ui->treeWidget->rMatLightList.isEmpty() && ui->treeWidget->getLightUrls().empty())
//    {
//        ui->statusBar->showMessage(QString("No lights for limb detection"), 3000);
//        return;
//    }

    if (currentROpenGLWidget->getRMatImageList().isEmpty())
    {
        ui->statusBar->showMessage(QString("No lights for limb detection"), 3000);
        return;
    }

    processing->setShowContours(ui->contoursCheckBox->isChecked());
    processing->setShowLimb(ui->limbCheckBox->isChecked());
    processing->setBlurSigma(ui->blurSpinBox->value());
    processing->setUseHPF(ui->hpfCheckBox->isChecked());
    processing->setHPFSigma(ui->hpfSpinBox->value());


    if (ui->treeWidget->rMatLightList.isEmpty())
    {
        return;
    }

    processing->setShowContours(ui->contoursCheckBox->isChecked());

    /// test limb fitting without smooth
    bool success2 = processing->wernerLimbFit(currentROpenGLWidget->getRMatImageList(), false);
    fittedLimbList2 = processing->getCircleOutList();

    /// With smooth
    int blurSize = ui->blurSizeSpinBox->value();
    bool success = processing->wernerLimbFit(currentROpenGLWidget->getRMatImageList(), true, blurSize);
    fittedLimbList  = processing->getCircleOutList();

    if (!success)
    {
        return;
    }
    /// Results need to be displayed as ROpenGLWidget as we will, de facto,
    /// deal with time series
    //createNewImage(processing->getContoursRMatList());

    // Display results in UI
    updateLimbResults();

    currentROpenGLWidget = new ROpenGLWidget(processing->getContoursRMatList(), this);
    addImage(currentROpenGLWidget);
    setupSliders(currentROpenGLWidget);

    autoScale(currentROpenGLWidget);
    currentSubWindow->show();
    displayPlotWidget(currentROpenGLWidget);


}

void RMainWindow::solarLimbRegisterSlot()
{
    processing->setShowContours(ui->contoursCheckBox->isChecked());
    processing->setShowLimb(ui->limbCheckBox->isChecked());

    bool success = processing->solarLimbRegisterSeries(currentROpenGLWidget->getRMatImageList());

    if (!success)
    {
        return;
    }


    createNewImage(processing->getLimbFitResultList1());

    //rangeScale();
    autoScale();
    currentROpenGLWidget->setRadius(processing->getMeanRadius());

}

void RMainWindow::updateLimbResults()
{
    if (fittedLimbList.empty())
    {
        tempMessageSignal("No limb fit data available");
        return;
    }

    ui->centerXLabel->setText(QString::number(fittedLimbList.at(0).a, 'f', 2));
    ui->centerYLabel->setText(QString::number(fittedLimbList.at(0).b, 'f', 2));
    ui->radiusLabel->setText(QString::number(processing->getMeanRadius(), 'f', 2));
}



void RMainWindow::normalizeCurrentSeries()
{

//    QList<RMat*> normalizedRMatList = processing->normalizeSeriesByStats(currentROpenGLWidget->getRMatImageList());
    QList<RMat*> normalizedRMatList = processing->normalizeSeriesToXposure(currentROpenGLWidget->getRMatImageList());
    createNewImage(normalizedRMatList);
    autoScale();
}

void RMainWindow::convert14to16bitSeriesSlot()
{
    QList<RMat*> stretchedRMatList = processing->stretchSeries14to16bit(currentROpenGLWidget->getRMatImageList());
    createNewImage(stretchedRMatList);
    autoScale();
}


void RMainWindow::previewMatImageHPFSlot()
{
    if (ui->treeWidget->rMatLightList.empty())
    {
        emit tempMessageSignal(QString("No image"));
        return;
    }

    double sigma = (double) ui->hpfSpinBox->value();
    cv::Mat matImage = ui->treeWidget->rMatLightList.at(ui->sliderFrame->value())->matImage;
    matImageHPFPreview = processing->makeImageHPF(matImage, (double) sigma);

    createNewImage(matImageHPFPreview, false, instruments::generic, QString("High-pass filtered image, sigma = %1").arg(ui->hpfSpinBox->value()));
}

void RMainWindow::stackSlot()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    processing->setStackWithMean(ui->stackMeanRButton->isChecked());
    processing->setStackWithSigmaClip(ui->stackSigmaClipRButton->isChecked());

    processing->stack(currentROpenGLWidget->getRMatImageList());

}

void RMainWindow::binningSlot()
{
    int index = ui->sliderFrame->value();
    int binning;
    if (this->sender() == ui->binning2PButton)
    {
        binning = 2;
    }

    if (this->sender() == ui->binning4PButton)
    {
        binning = 4;
    }

    RMat *binnedRMatImage = new RMat();
    processing->rebin(currentROpenGLWidget->getRMatImageList().at(index), binnedRMatImage, binning);
    createNewImage(binnedRMatImage);
    autoScale();

}

void RMainWindow::HDRSlot()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }
    processing->createHDRMat(currentROpenGLWidget->getRMatImageList());
}

void RMainWindow::blockProcessingSlot()
{
    /// Let's get the UI input parameters
    setupLuckyImaging();

    /// Run the block processing according to the selected method

    if (ui->luckyLocalRButton->isChecked())
    { /// Local method, aligning blocks with each other, no fixed global search image
        processing->blockProcessingLocal(currentROpenGLWidget->getRMatImageList());
        createNewImage(processing->getResultList());
        autoScale();
    }


    if (ui->luckyGlobalRButton->isChecked())
    {
        if (currentROpenGLWidget != NULL)
        {
            if (ui->LuckyMetricsComboBox->currentText() != QString("Laplace"))
            {
                processing->blockProcessingGlobalGradients(currentROpenGLWidget->getRMatImageList());
            }
            else
            {
                processing->blockProcessingGlobalLaplace(currentROpenGLWidget->getRMatImageList());
            }
        }
        else // Batch processing off screen.
        {
            if (ui->treeWidget->getLightUrls().empty())
            {
                tempMessageSignal(QString("Nothing to process"));
                return;
            }
            processing->loadRMatLightList(ui->treeWidget->getLightUrls());

            if (ui->LuckyMetricsComboBox->currentText() != QString("Laplace"))
            {
                processing->blockProcessingGlobalGradients(currentROpenGLWidget->getRMatImageList());
            }
            else
            {
                processing->blockProcessingGlobalLaplace(currentROpenGLWidget->getRMatImageList());
            }

        }

        // Display final result
        createNewImage(processing->getResultList());
        autoScale();
    }

}

void RMainWindow::extractLuckySampleSlot()
{
    /// This slot is called by the "Extract" button in the lucky image panel (ui).
    /// It requires the selection of a square ROI using the "ROI" button next to it.
    if (rect.isEmpty())
    {

        int x = ui->xROIBox->value();
        int y = ui->yROIBox->value();

        QList<RMat *> blockList2;
        setupLuckyImaging();
        processing->extractLuckySample2(currentROpenGLWidget->getRMatImageList(), blockList2, x, y, false);
        // Display results
        //    createNewImage(blockList1);
        //    autoScale();
        createNewImage(blockList2);
        autoScale();

    }
    else
    {
        // Retrieve the mouse coordinates of the center of the ROI
        int x = this->rect.center().x();
        int y = this->rect.center().y();

        setupLuckyImaging();

        // Extract block using gradient-quality metric.
//        QList<RMat *> blockList1;
//        processing->extractLuckySample(currentROpenGLWidget->getRMatImageList(), blockList1, x, y, true);
        /// Extract block using Laplacian-quality metric.
        QList<RMat *> blockList2;
        processing->extractLuckySample2(currentROpenGLWidget->getRMatImageList(), blockList2, x, y, true);
        // Display results
    //    createNewImage(blockList1);
    //    autoScale();
        createNewImage(blockList2);
        autoScale();
    }



}

void RMainWindow::luckyROISlot()
{
    int blkSize = ui->luckyBlkSizeSpinBox->value();
    selectROI(true, blkSize);
}

void RMainWindow::setupLuckyImaging()
{
    /// Let's get the UI input parameters
    int binning = 1* (int)ui->luckyBin0RButton->isChecked() + 2 * (int)ui->luckyBin2RButton->isChecked() + 4 * (int)ui->luckyBin4RButton->isChecked();
    int blkSize = ui->luckyBlkSizeSpinBox->value();
    int nBest = ui->nBestSpinBox->value();
    QString qualityMetric = ui->LuckyMetricsComboBox->currentText();
    processing->setBinning(binning);
    processing->setBlkSize(blkSize);
    processing->setNBest(nBest);
    processing->setQualityMetric(qualityMetric);
}

void RMainWindow::showLimbFitStats()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    if (processing->getCircleOutList().empty())
    {
        return;
    }

    QPoint windowPos(1,1);

    bool addWindow = false;

    if (plotSubWindow == NULL)
    {
        plotSubWindow = new QMdiSubWindow();
        addWindow = true;
    }
    else
    {
        // Restore last used position
        windowPos = plotSubWindow->pos();
    }

    // Prepare the plot data

    int nFrames = currentROpenGLWidget->getRMatImageList().size();

    QVector<double> x(nFrames);
    QVector<double> y(nFrames);
    QVector<double> y2(nFrames);
    vector<double> yVector(nFrames);
    for (int i = 0 ; i < nFrames ; ++i)
    {
        x[i]  = i+1;
        y[i]  = (double)  fittedLimbList.at(i).r;
        yVector[i]  = (double) fittedLimbList.at(i).r;
        y2[i] = y[i] - fittedLimbList2.at(i).r;
    }
    cv::Scalar meanDiff, stdDiff, meanRadius, stdRadius, meanRadius2, stdRadius2;
    cv::meanStdDev(y2.toStdVector(), meanDiff, stdDiff);
    cv::meanStdDev(y.toStdVector(), meanRadius, stdRadius);
    cv::meanStdDev(yVector, meanRadius2, stdRadius2);


    limbFitPlot = new QCustomPlot();
    limbFitPlot->legend->setVisible(true);
    // by default, the legend is in the inset layout of the main axis rect. So this is how we access it to change legend placement:
    limbFitPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom|Qt::AlignRight);
    limbFitPlot->addGraph();
    limbFitPlot->plottable(0)->setPen(QPen(Qt::red));
    limbFitPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc));
    limbFitPlot->graph(0)->setName(QString("Radius: ") + QString("mean = %1; std = %2").arg(meanRadius[0], 0, 'f', 2).arg(stdRadius[0], 0, 'f', 2));
    limbFitPlot->yAxis->setLabel(QString("Fitted limb radius (px)"));
    limbFitPlot->graph(0)->setData(x, y);
    limbFitPlot->graph(0)->rescaleAxes();

    //limbFitPlot->xAxis->setAutoTickStep(false);
    //limbFitPlot->xAxis->setTickStep(1);
    limbFitPlot->xAxis->setRange(limbFitPlot->xAxis->range().lower, limbFitPlot->xAxis->range().upper + 1);
    limbFitPlot->xAxis->setLabel(QString("Image #"));

    limbFitPlot->yAxis->setRange(890, 920);
    limbFitPlot->yAxis->setAutoTickStep(false);
    limbFitPlot->yAxis->setTickStep(5);
    limbFitPlot->yAxis->setSubTickCount(4);
    limbFitPlot->yAxis->grid()->setPen(Qt::DashDotDotLine);

    // setup for graph 2: key axis top, value axis right
    limbFitPlot->addGraph(limbFitPlot->xAxis, limbFitPlot->yAxis2);
    limbFitPlot->graph(1)->setPen(QPen(Qt::black));
    limbFitPlot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc));
    limbFitPlot->graph(1)->setName(QString("Difference with - without smooth: ") + QString("mean = %1; std = %2").arg(meanDiff[0], 0, 'f', 2).arg(stdDiff[0], 0, 'f', 2));
    limbFitPlot->yAxis2->setLabel(QString("Radius difference (px)"));
    limbFitPlot->graph(1)->setData(x, y2);
    limbFitPlot->graph(1)->rescaleAxes();
    // activate right axis, which is invisible by default
    limbFitPlot->yAxis2->setVisible(true);
    limbFitPlot->yAxis2->setRange(-4, 4);
    limbFitPlot->yAxis2->grid()->setVisible(true);
    limbFitPlot->yAxis2->grid()->setPen(Qt::DotLine);


    limbFitPlot->setInteraction(QCP::iSelectPlottables, true);
    limbFitPlot->setInteraction(QCP::iSelectAxes , true);
    limbFitPlot->setInteraction(QCP::iSelectItems , true);
    limbFitPlot->xAxis->setSelectableParts(QCPAxis::spAxis | QCPAxis::spTickLabels);
    limbFitPlot->yAxis->setSelectableParts(QCPAxis::spAxis | QCPAxis::spTickLabels);
    limbFitPlot->yAxis2->setSelectableParts(QCPAxis::spAxis | QCPAxis::spTickLabels);

    // Allow the user to zoom in / zoom out and scroll horizontally
    limbFitPlot->setInteraction(QCP::iRangeDrag, true);
    limbFitPlot->setInteraction(QCP::iRangeZoom, true);
//    limbFitPlot->axisRect(0)->setRangeDrag(Qt::Horizontal);
//    limbFitPlot->axisRect(0)->setRangeZoom(Qt::Horizontal | Qt::Vertical);
    limbFitPlot->axisRect(0)->setRangeDrag(Qt::Vertical);
    limbFitPlot->axisRect(0)->setRangeZoom(Qt::Vertical);

    connect(limbFitPlot->xAxis , SIGNAL(selectionChanged(QCPAxis::SelectableParts)), this, SLOT(changeZoomAxisSlot()));
    connect(limbFitPlot->yAxis , SIGNAL(selectionChanged(QCPAxis::SelectableParts)), this, SLOT(changeZoomAxisSlot()));
    connect(limbFitPlot->yAxis2 , SIGNAL(selectionChanged(QCPAxis::SelectableParts)), this, SLOT(changeZoomAxisSlot()));


    plotSubWindow->setWidget(limbFitPlot);
    if (addWindow)
    {
        ui->mdiArea->addSubWindow(plotSubWindow);
    }
    plotSubWindow->resize(defaultWindowSize);
    plotSubWindow->move(windowPos);
    plotSubWindow->show();

}

void RMainWindow::setupToneMappingCurve()
{
    plotSubWindow = new QMdiSubWindow();
    ui->mdiArea->addSubWindow(plotSubWindow);

    int nBins = (int) currentROpenGLWidget->getRMatImageList().at(0)->getDataRange();

    QVector<double> x(nBins);
    QVector<double> yRef(nBins);
    QVector<double> y(nBins);
    for (int i = 0 ; i < nBins ; ++i)
    {
        x[i] = (double) i / (double) nBins;
        yRef[i] = (double) x.at(i);
        y[i] = (double) x.at(i);

    }

    toneMappingPlot = new QCustomPlot();
    QCPGraph *toneMappingGraphRef = new QCPGraph(toneMappingPlot->xAxis, toneMappingPlot->yAxis);
    toneMappingGraphRef->setData(x, yRef);

    toneMappingGraph = new QCPGraph(toneMappingPlot->xAxis, toneMappingPlot->yAxis);
    toneMappingGraph->setData(x, y);


    toneMappingPlot->addPlottable(toneMappingGraphRef);
    toneMappingPlot->graph(0)->setPen(QPen(QColor(0, 0, 255, 100)));

    toneMappingPlot->addPlottable(toneMappingGraph);
    toneMappingPlot->graph(1)->setPen(QPen(QColor(255, 0, 0, 100)));

    toneMappingPlot->rescaleAxes();

    /// Setup axes according to image type
    if (currentROpenGLWidget->getRMatImageList().at(0)->matImage.type() == CV_8U)
    {
        toneMappingPlot->xAxis->setRange(0, 260);
        toneMappingPlot->yAxis->setRange(0, 260);
    }
    else
    {
        toneMappingPlot->xAxis->setAutoTickStep(false);
        toneMappingPlot->yAxis->setAutoTickStep(false);
        toneMappingPlot->xAxis->setTickStep(0.2);
        toneMappingPlot->yAxis->setTickStep(0.2);
        toneMappingPlot->xAxis->setRange(0, 1.05);
        toneMappingPlot->yAxis->setRange(0, 1.05);
    }
    /// X axis
    toneMappingPlot->xAxis->setLabel(QString("Intensity"));
    toneMappingPlot->xAxis->setScaleRatio(toneMappingPlot->xAxis, 1.0);
    /// Y axis
    toneMappingPlot->yAxis->setLabel(QString("Re-scaled intensity"));
    plotSubWindow->resize(defaultWindowSize);

//    else
//    {
//        qDebug("Updating plot");
//        toneMappingPlot->graph(0)->setData(x, y);
//        toneMappingPlot->replot();
//        //toneMappingPlot->update();
//    }

    plotSubWindow->setWidget(toneMappingPlot);
    plotSubWindow->show();

    updateToneMappingSlot();
}

void RMainWindow::updateToneMappingSlot()
{

    updateInvGaussianParams();

    if (toneMappingGraph == NULL)
    {
        return;
    }

    std::cout << "applyToneMapping =" << ui->applyToneMappingCheckBox->isChecked() << std::endl;
    std::cout << "iMax =" << iMax << std::endl;
    std::cout << "lambda =" << lambda << std::endl;
    std::cout << "mu =" << mu << std::endl;


    int nBins = (int) currentROpenGLWidget->getRMatImageList().at(0)->getDataRange();
    double mu2 = pow(mu, 2.0);

    QVector<double> x(nBins);
    QVector<double> y(nBins);

    if (ui->InverseGaussianRButton->isChecked())
    {
        for (int i = 0 ; i < nBins ; ++i)
        {
            x[i] = (double) i / (double) nBins;
            y[i] = iMax * sqrt(lambda / (2.0 * 3.1415 * pow(x[i], 3.0))) * exp(-lambda * pow( (x[i] -  mu), 2.0) / (2.0 * mu2 * x[i]) ) + x[i];
        }
        toneMappingPlot->graph(1)->setData(x, y);
        toneMappingPlot->replot();

        applyToneMappingCurve();
    }

}

void RMainWindow::applyToneMappingCurve()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    updateInvGaussianParams();

    currentROpenGLWidget->setApplyToneMapping(ui->applyToneMappingCheckBox->isChecked());
    currentROpenGLWidget->setUseInverseGausssian(ui->InverseGaussianRButton->isChecked());
    currentROpenGLWidget->setImax(iMax);
    currentROpenGLWidget->setLambda(lambda);
    currentROpenGLWidget->setMu(mu);
    currentROpenGLWidget->update();
}

void RMainWindow::hAlphaToneMappingSlot()
{
    if (currentROpenGLWidget->getRMatImageList().at(0)->matImage.type() == CV_8U)
    {
//        ui->toneMappingSlider1->setValue(4500);
//        ui->toneMappingSlider2->setValue(1200);
//        ui->toneMappingSlider3->setValue(45);

//        ui->spinBox_3->setValue(4500);
//        ui->spinBox_4->setValue(1200);
//        ui->spinBox_5->setValue(45);
    }

}

void RMainWindow::applyScaleLimbSlot()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    currentROpenGLWidget->setScaleLimb(ui->scaleLimbCheckBox->isChecked());
    currentROpenGLWidget->update();
}

void RMainWindow::displayPlotWidget(ROpenGLWidget* rOpenGLWidget)
{
    ui->histoDock->setWidget(rOpenGLWidget->fetchCurrentCustomPlot());
    rOpenGLWidget->updateCustomPlotLineItems();
    ui->scaleLimbCheckBox->setChecked(rOpenGLWidget->getScaleLimb());
}

void RMainWindow::addHeaderWidget()
{
    int frameIndex = ui->sliderFrame->value();

    if (currentROpenGLWidget == NULL)
    {
        qDebug("RMainWindow::addHeaderWidget():: currentROpenGLWidget == NULL");
        return;
    }
    else if (currentROpenGLWidget->getRListImageManager()->getUrlList().empty())
    {
      qDebug("RMainWindow::addHeaderWidget():: currentROpenGLWidget->getRListImageManager() empty");
      return;
    }

    qDebug("Showing headerWidget");

    ui->mdiArea->addSubWindow(currentROpenGLWidget->getTableRSubWindow());
    currentROpenGLWidget->getTableRSubWindow()->show();
    currentROpenGLWidget->getTableRSubWindow()->resize(currentROpenGLWidget->getRListImageManager()->getTableWidgetList().at(0)->size());

    //Connect the slider value changed with the header dock change
    //QObject::connect(ui->sliderFrame, SIGNAL(valueChanged(int)), this, SLOT(updateTableWidget(int)));

    connect(ui->sliderFrame, SIGNAL(valueChanged(int)), currentROpenGLWidget, SLOT(setupTableWidget(int)));
    connect(currentROpenGLWidget->getTableRSubWindow(), SIGNAL(gotHidden()), this, SLOT(uncheckActionHeaderState()));
}

void RMainWindow::updateSubFrame(QImage *image, float intensity, int x, int y)
{
    char format = 'f';
    int precision = 0;

    if (std::abs(currentROpenGLWidget->getRMatImageList().at(0)->getDataMax()) <= 255)
    {
        format = 'g';
        precision = 3;
    }

    QString intensityQStr = QString::number(intensity, format, precision);

    ui->subFrame->setImage(image);
    ui->subFrame->setCursorText(intensityQStr);
    ui->subFrame->setImageCoordX(x);
    ui->subFrame->setImageCoordY(y);
    ui->subFrame->update();
}

QString RMainWindow::checkExistingDir()
{
    QString openDir = QString();
    if (!ui->calibrateDirLineEdit->text().isEmpty())
    {
        openDir = ui->calibrateDirLineEdit->text();
    }
    else
    {
        openDir = QDir::homePath();
    }

    return openDir;
}

void RMainWindow::autoScale()
{
    /// This sends new Minimum and Maximum values to the ROpenGLWidget, depending on the instrument.
    /// Then it moves the slider according to these values.
    /// Finally, by moving the sliders, who are connected to updateCurrentROpenGLwidget(), the display
    /// gets updated.

    if (currentROpenGLWidget == NULL)
        return;


    if (currentROpenGLWidget->getRMatImageList().at(0)->matImage.type() == CV_8U ||
             currentROpenGLWidget->getRMatImageList().at(0)->matImage.type() == CV_8UC3)
    {
        float tempMax = 255;
        float tempMin = 0;
        currentROpenGLWidget->setNewMax(tempMax);
        currentROpenGLWidget->setNewMin(tempMin);

        sliderValueHigh = convertScaleToSlider(tempMax);
        sliderValueLow = convertScaleToSlider(tempMin);
    }
    else
    {
        currentROpenGLWidget->setNewMax(currentROpenGLWidget->getRMatImageList().at(0)->getIntensityHigh());
        currentROpenGLWidget->setNewMin(currentROpenGLWidget->getRMatImageList().at(0)->getIntensityLow());

        /// Make the slider use the histogram autoscaling values.
        sliderValueHigh = convertScaleToSlider(currentROpenGLWidget->getRMatImageList().at(0)->getIntensityHigh());
        sliderValueLow = convertScaleToSlider(currentROpenGLWidget->getRMatImageList().at(0)->getIntensityLow());
    }

    gamma = 1.0f;
    sliderValueGamma = convertGammaToSlider(gamma);
    // Below, the slider ignore the input if it is equal to the current state, but it prevents from going through
    // scaleImageSlot subsequently. So we have to tickle it a bit.
    ui->sliderHigh->setValue(sliderValueHigh-1);
    ui->sliderHigh->setValue(sliderValueHigh);
    ui->sliderLow->setValue(sliderValueLow +1);
    ui->sliderLow->setValue(sliderValueLow);
    ui->sliderGamma->setValue(sliderValueGamma);

    /// Limb sliders
    ui->limbSliderHigh->setValue(sliderValueHigh-1);
    ui->limbSliderHigh->setValue(sliderValueHigh);
    ui->limbSliderLow->setValue(sliderValueLow +1);
    ui->limbSliderLow->setValue(sliderValueLow);

    qDebug("RMainWindow::autoScale() sliderValueHigh = %i , sliderValueLow = %i", sliderValueHigh, sliderValueLow);

}

void RMainWindow::autoScale(ROpenGLWidget *rOpenGLWidget)
{
    if (rOpenGLWidget == NULL)
        return;

    if (rOpenGLWidget->getRMatImageList().at(0)->matImage.type() == CV_8U ||
        rOpenGLWidget->getRMatImageList().at(0)->matImage.type() == CV_8UC3)
    {
        rOpenGLWidget->setNewMax(255);
        rOpenGLWidget->setNewMin(0);

        sliderValueHigh = convertScaleToSlider(rOpenGLWidget->getNewMax());
        sliderValueLow = convertScaleToSlider(rOpenGLWidget->getNewMin());

    }
    else
    {
        /// Make the slider use the histogram autoscaling values.
        rOpenGLWidget->setNewMax(rOpenGLWidget->getRMatImageList().at(0)->getIntensityHigh());
        rOpenGLWidget->setNewMin(rOpenGLWidget->getRMatImageList().at(0)->getIntensityLow());

        sliderValueHigh = convertScaleToSlider(rOpenGLWidget->getNewMax());
        sliderValueLow = convertScaleToSlider(rOpenGLWidget->getNewMin());

    }

    gamma = 1.0f;
    sliderValueGamma = convertGammaToSlider(gamma);
    // Below, the slider ignore the input if it is equal to the current state, but it prevents from going through
    // scaleImageSlot subsequently. So we have to tickle it a bit.
    ui->sliderHigh->setValue(sliderValueHigh-1);
    ui->sliderHigh->setValue(sliderValueHigh);
    ui->sliderLow->setValue(sliderValueLow +1);
    ui->sliderLow->setValue(sliderValueLow);
    ui->sliderGamma->setValue(sliderValueGamma);

    qDebug("RMainWindow::autoScale() intensityHigh = %f , intensityLow = %f", rOpenGLWidget->getRMatImageList().at(0)->getIntensityHigh(), rOpenGLWidget->getRMatImageList().at(0)->getIntensityLow());
    qDebug("RMainWindow::autoScale() newMax = %f , newMin = %f", rOpenGLWidget->getNewMax(), rOpenGLWidget->getNewMin());
    qDebug("RMainWindow::autoScale() sliderValueHigh = %i , sliderValueLow = %i", sliderValueHigh, sliderValueLow);

    /// Limb sliders
    ui->limbSliderHigh->setValue(sliderValueHigh-1);
    ui->limbSliderHigh->setValue(sliderValueHigh);
    ui->limbSliderLow->setValue(sliderValueLow +1);
    ui->limbSliderLow->setValue(sliderValueLow);

}

void RMainWindow::minMaxScale()
{
    if (currentROpenGLWidget == NULL)
        return;

    gamma = 1.0f;
    sliderValueGamma = convertGammaToSlider(gamma);

    sliderValueHigh = convertScaleToSlider(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getDataMax());
    sliderValueLow = convertScaleToSlider(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getDataMin());

    ui->sliderHigh->setValue(sliderValueHigh-1);
    ui->sliderHigh->setValue(sliderValueHigh);
    ui->sliderLow->setValue(sliderValueLow +1);
    ui->sliderLow->setValue(sliderValueLow);
    ui->sliderGamma->setValue(sliderValueGamma);

    qDebug("RMainWindow::autoScale() sliderValueHigh = %i , sliderValueLow = %i", sliderValueHigh, sliderValueLow);

    /// Limb sliders
    ui->limbSliderHigh->setValue(sliderValueHigh-1);
    ui->limbSliderHigh->setValue(sliderValueHigh);
    ui->limbSliderLow->setValue(sliderValueLow +1);
    ui->limbSliderLow->setValue(sliderValueLow);

    processing->showMinMax(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->matImage);
    processing->showMinMax(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->matImageGray);
    //showMinMax(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value()));
}

void RMainWindow::rangeScale()
{

    if (currentROpenGLWidget == NULL)
        return;

    instruments intrument = currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getInstrument();
    int dataType = currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->matImage.type();
    if ( intrument == instruments::USET)
    {
        sliderValueHigh = convertScaleToSlider(4095.0);
        sliderValueLow = convertScaleToSlider(0.0);

    }
    else if (dataType == CV_32F || dataType == CV_32FC3)
    {
        sliderValueHigh = convertScaleToSlider(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getDataMax());
        sliderValueLow = convertScaleToSlider(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getDataMin());
    }
    else if (dataType == CV_16U || dataType == CV_16UC3)
    {
        sliderValueHigh = convertScaleToSlider(65535.0);
        sliderValueLow = convertScaleToSlider(0.0);

    }
    else if (dataType == CV_8U || dataType == CV_8UC3)
    {
        sliderValueHigh = convertScaleToSlider(255.0);
        sliderValueLow = convertScaleToSlider(0.0);
    }
    else
    {
        qDebug("Unrecognized data type");
        exit(1);
    }

    ui->sliderHigh->setValue(sliderValueHigh-1);
    ui->sliderHigh->setValue(sliderValueHigh);
    ui->sliderLow->setValue(sliderValueLow +1);
    ui->sliderLow->setValue(sliderValueLow);

    /// Limb sliders
    ui->limbSliderHigh->setValue(sliderValueHigh-1);
    ui->limbSliderHigh->setValue(sliderValueHigh);
    ui->limbSliderLow->setValue(sliderValueLow +1);
    ui->limbSliderLow->setValue(sliderValueLow);

}

void RMainWindow::updateCurrentROpenGLWidget()
{
    float dataRange = (float) (currentROpenGLWidget->getNewMax() - currentROpenGLWidget->getNewMin()) ;
    qDebug() << "RMainWindow::updateCurrentROpenGLWidget()   dataRange =" << dataRange;
    alpha = 1.0f / dataRange;
    beta = (float) (- currentROpenGLWidget->getNewMin() / dataRange);

    /// Limb scaling
    float dataRangeLimb = (float) (currentROpenGLWidget->getLimbNewMax() - currentROpenGLWidget->getLimbNewMin()) ;
    qDebug() << "RMainWindow::updateCurrentROpenGLWidget()   dataRange =" << dataRange;
    float alphaLimb = 1.0f / dataRangeLimb;
    float betaLimb = (float) (- currentROpenGLWidget->getLimbNewMin() / dataRangeLimb);

    currentROpenGLWidget->setAlpha(alpha);
    currentROpenGLWidget->setBeta(beta);
    /// Limb
    currentROpenGLWidget->setAlphaLimb(alphaLimb);
    currentROpenGLWidget->setBetaLimb(betaLimb);

    currentROpenGLWidget->update();
    //currentROpenGLWidget->updateSubQImage();
    //currentROpenGLWidget->updateCustomPlotLineItems();


}

void RMainWindow::updateSliderValueSlot()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    if (this->sender() == ui->doubleSpinBoxHigh)
    {
        double valueD = ui->doubleSpinBoxHigh->value();
        sliderValueHigh = convertScaleToSlider((float) valueD);
        ui->sliderHigh->setValue(sliderValueHigh);
    }
    else if (this->sender() == ui->doubleSpinBoxLow)
    {
        double valueD = ui->doubleSpinBoxLow->value();
        sliderValueLow = convertScaleToSlider((float) valueD);
        ui->sliderLow->setValue(sliderValueLow);
    }
    else if (this->sender() == ui->doubleSpinBoxGamma)
    {
        double valueD = ui->doubleSpinBoxGamma->value();
        sliderValueGamma = convertGammaToSlider((float) valueD);
        qDebug("RMainWindow::updateSliderValueSlot() valueD = %f , sliderValueGamma = %i", valueD, sliderValueGamma);
        ui->sliderGamma->setValue(sliderValueGamma);
    }
}

void RMainWindow::updateWB(int value)
{

    double colorFactor = (double) (value * sliderScaleWB);

    if (this->sender() == ui->redSlider)
    {
        ui->redSpinBox->setValue(colorFactor);
        if (currentROpenGLWidget == NULL)
        {
            return;
        }
        currentROpenGLWidget->setWbRed(colorFactor);
    }

    if (this->sender() == ui->greenSlider)
    {
        ui->greenSpinBox->setValue(colorFactor);
        if (currentROpenGLWidget == NULL)
        {
            return;
        }
        currentROpenGLWidget->setWbGreen(colorFactor);
    }

    if (this->sender() == ui->blueSlider)
    {
        ui->blueSpinBox->setValue(colorFactor);
        if (currentROpenGLWidget == NULL)
        {
            return;
        }
        currentROpenGLWidget->setWbBlue(colorFactor);
    }

    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    currentROpenGLWidget->update();
    currentROpenGLWidget->updateSubQImage();
}

int RMainWindow::convertGammaToSlider(float gamma)
{
    int sliderValueGamma = (int) std::round((gamma - gammaMin)/gammaScale + 1.0f);
    qDebug("RMainWindow::convertGammaToSlider()  gammaMin = %f, gammaScale = %f , gamma = %f ,sliderValueGamma = %i", gammaMin, gammaScale, gamma, sliderValueGamma);

    return sliderValueGamma;
}


float RMainWindow::convertSliderToScale(int value)
{
    float scaledValue = 0.0f;
    if (currentROpenGLWidget == NULL)
    {
        qDebug("currentROpenGLWidget is NULL.");
        return (float) scaledValue;
    }
    scaledValue = sliderToScaleMinimum + sliderScale * ((float) value) ;
    qDebug("RMainWindow::convertSliderToScale:: sliderToScaleMinimum = %f ; sliderScale = %f ; value = %i ; scaledValue = %f", sliderToScaleMinimum, sliderScale, value, scaledValue);
    return scaledValue;
}

int RMainWindow::convertScaleToSlider(float valueF)
{

    int sliderValue = 1;

    if (currentROpenGLWidget->getRMatImageList().empty())
    {
        qDebug("currentROpenGLWidget->getRMatImageList() is empty.");
        return sliderValue;
    }


    sliderValue = (int) std::round((valueF - sliderToScaleMinimum)/sliderScale);
    // If the slider Value is greater than the range of the slider, it is ignored instead of clipped.
    // So we need to clip to the max value of the slider which is the slider range.
    //sliderValue = std::min(sliderValue, ui->sliderHigh->maximum());

    return sliderValue;
}

float RMainWindow::convertLimbSliderToScale(int value)
{
    float scaledValue = 0.0f;
    if (currentROpenGLWidget == NULL)
    {
        qDebug("currentROpenGLWidget is NULL.");
        return (float) scaledValue;
    }
    scaledValue = limbSliderToScaleMinimum + limbSliderScale * ((float) (value-1)) ;
    qDebug("RMainWindow::convertSliderToScale:: sliderToScaleMinimum = %f ; sliderScale = %f ; value = %i ; scaledValue = %f", sliderToScaleMinimum, sliderScale, value, scaledValue);
    return scaledValue;
}




float RMainWindow::convertSliderToGamma(int value)
{
    float valueF = (float) (gammaMin +  gammaScale * (value -1));
    return valueF;
}


void RMainWindow::changeROpenGLWidget(ROpenGLWidget *rOpenGLWidget)
{

    currentROpenGLWidget = rOpenGLWidget;

    // Restore slider values
    qDebug("RMainWindow::changeROpenGLWidget:: rOpenGLWidget->getNewMin() = %f", rOpenGLWidget->getNewMin());
    setupSliders(rOpenGLWidget);
    /// Scaling
    qDebug("RMainWindow::changeROpenGLWidget:: rOpenGLWidget->getNewMin() = %f", rOpenGLWidget->getNewMin());
    ui->sliderHigh->setValue(convertScaleToSlider(currentROpenGLWidget->getNewMax()));
    ui->sliderLow->setValue(convertScaleToSlider(currentROpenGLWidget->getNewMin()));

    ui->limbSliderHigh->setValue(convertScaleToSlider(currentROpenGLWidget->getLimbNewMax()));
    ui->limbSliderLow->setValue(convertScaleToSlider(currentROpenGLWidget->getLimbNewMin()));


    qDebug("RMainWindow::changeROpenGLWidget:: currentROpenGLWidget->getNewMin() = %f", currentROpenGLWidget->getNewMin());
    qDebug("RMainWindow::changeROpenGLWidget:: rOpenGLWidget->getNewMin() = %f", rOpenGLWidget->getNewMin());

    ui->sliderGamma->setValue(convertGammaToSlider(currentROpenGLWidget->getGamma()));
    /// White balance
    ui->redSlider->setValue((int) std::round(currentROpenGLWidget->getWbRed() / sliderScaleWB));
    ui->greenSlider->setValue((int) std::round(currentROpenGLWidget->getWbGreen() / sliderScaleWB));
    ui->blueSlider->setValue((int) std::round(currentROpenGLWidget->getWbBlue() / sliderScaleWB));
    /// Time series
    ui->sliderFrame->blockSignals(true);
    ui->sliderFrame->setRange(0, currentROpenGLWidget->getRMatImageList().size()-1);
    ui->sliderFrame->setValue(currentROpenGLWidget->getFrameIndex());
    ui->sliderFrame->blockSignals(false);
    ui->imageLabel->setText(QString::number(currentROpenGLWidget->getFrameIndex()+1) + QString("/") + QString::number(currentROpenGLWidget->getRMatImageList().size()));

    displayPlotWidget(rOpenGLWidget);

    // Refresh state of QAction buttons in toolbar
    if (currentROpenGLWidget->getTableRSubWindow() != NULL && currentROpenGLWidget->getTableRSubWindow()->isVisible())
    {
        ui->actionHeader->setChecked(true);
    }
    else
    {
        ui->actionHeader->setChecked(false);
    }

    // Update stats
    updateStats(frameIndex);

    processing->setCurrentROpenGLWidget(rOpenGLWidget);

}

void RMainWindow::updateFrameInSeries(int frameIndex)
{
    if (ui->displayFirstCheckBox->isChecked())
    {
        return;
    }
    /// Updates the display of the currentROpenGLWidget: change current image in the series, subImage, and histogram
    // The sliderValue minimum is 1. The frameIndex minimum is 0;
    ui->imageLabel->setText(QString::number(frameIndex+1) + QString("/") + QString::number(currentROpenGLWidget->getRMatImageList().size()));
    currentROpenGLWidget->changeFrame(frameIndex);
    ui->mdiArea->currentSubWindow()->setWindowTitle(currentROpenGLWidget->getRMatImageList().at(frameIndex)->getImageTitle() + QString(" #%1 ").arg(frameIndex+1));
    displayPlotWidget(currentROpenGLWidget);

    // Update stats
    updateStats(frameIndex);

    this->frameIndex = frameIndex;


}


void RMainWindow::setupExportCalibrateDir()
{
    QString dir = QFileDialog::getExistingDirectory(
                            0,
                            "Select directory to export Masters",
                            checkExistingDir());

    ui->calibrateDirLineEdit->setText(dir);

    emit tempMessageSignal(QString(" Exporting calibrated files to: ") + dir, 0);
}

void RMainWindow::exportMastersToFits()
{
    processing->exportMastersToFits();
}

void RMainWindow::exportFrames()
{
    if (currentROpenGLWidget != NULL)
    {
        if (ui->fitsExportCheckBox->isChecked())
        {
            exportFramesToFits();
        }

        if (ui->jpegExportCheckBox->isChecked())
        {
            exportFramesToJpeg();
        }

        if (ui->tiffExport->isChecked())
        {
            exportFramesToTiff();
        }
    }
    else
    {
        // Do a batch conversion to FITS off screen using the URLs in the treeWidget
        emit tempMessageSignal(QString("Exporting frames... "));
        // Lights
        processing->batchExportToFits(ui->treeWidget->getLightUrls(), ui->exportDirEdit->text());
        // Bias
        processing->batchExportToFits(ui->treeWidget->getBiasUrls(), ui->exportDirEdit->text());
        // Darks
        processing->batchExportToFits(ui->treeWidget->getDarkUrls(), ui->exportDirEdit->text());
        //Flats
        processing->batchExportToFits(ui->treeWidget->getFlatUrls(), ui->exportDirEdit->text());
        emit tempMessageSignal(QString("Exporting frames completed! "));
    }

}

QString RMainWindow::makeFilePath(QString basename, int frameNumber)
{
    QDir exportDir(ui->exportDirEdit->text());
    QString indexQStr = QString("%1").arg(frameNumber, 5, 10, QChar('0'));
    QString fileName(basename + indexQStr);
    if (ui->baseNameCheckBox->isChecked())
    {
        QString baseName = currentROpenGLWidget->getRMatImageList().at(frameNumber)->getFileInfo().baseName();
        fileName = baseName + QString("_C");
    }
    QString filePath = exportDir.filePath(fileName);
    return filePath;
}


void RMainWindow::exportFramesToFits()
{
    QDir exportDir;
    if (ui->exportDirEdit->text().isEmpty())
    {
        tempMessageSignal(QString("Export directory empty. Using input directory"));
        exportDir = currentROpenGLWidget->getRMatImageList().at(0)->getFileInfo().absoluteDir();
        exportDir = ui->treeWidget->getLightsDir();

    }
    else
    {
        exportDir = QDir(ui->exportDirEdit->text());
    }

    processing->exportFramesToFits(currentROpenGLWidget->getRMatImageList(), exportDir, ui->baseNameCheckBox->isChecked());
    tempMessageSignal(QString("Files exported."), 0);
}

void RMainWindow::exportFramesToJpeg()
{
    for (int i = 0 ; i < currentROpenGLWidget->getRMatImageList().size() ; i++)
    {
        QString imageTitle = currentROpenGLWidget->getRMatImageList().at(i)->getImageTitle() + QString("_");
        QString filePath = makeFilePath(QString("image_") + imageTitle, i);
        processing->exportToJpeg(currentROpenGLWidget->getRMatImageList().at(i), filePath);
    }
}

void RMainWindow::exportFramesToTiff()
{
    for (int i = 0 ; i < currentROpenGLWidget->getRMatImageList().size() ; i++)
    {
        QString imageTitle = currentROpenGLWidget->getRMatImageList().at(i)->getImageTitle() + QString("_");
        QString filePath = makeFilePath(QString("image_") + imageTitle, i);
        processing->exportToTiff(currentROpenGLWidget->getRMatImageList().at(i), filePath);
    }
}



void RMainWindow::convertTo8Bit()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    QList<RMat*> rMat8bitList;

    for (int i = 0 ; i < currentROpenGLWidget->getRMatImageList().size() ; ++i)
    {
        cv::Mat mat8bit = currentROpenGLWidget->getRMatImageList().at(i)->matImage.clone();
        float dataMax = (float) currentROpenGLWidget->getRMatImageList().at(i)->getDataMax();
        float fac = 255.0/ dataMax;
//        mat8bit.convertTo(mat8bit, CV_8U, 256.0f / currentROpenGLWidget->getRMatImageList().at(i)->getDataRange());
        mat8bit.convertTo(mat8bit, CV_8U, fac);

        RMat *rMat8bit = new RMat(mat8bit.clone(), false);

        rMat8bit->setImageTitle(QString("8-bit image # %1").arg(i));
        rMat8bit->setDate_time(currentROpenGLWidget->getRMatImageList().at(i)->getDate_time());
        rMat8bitList << rMat8bit;
    }

    createNewImage(rMat8bitList);
}

cv::Mat RMainWindow::convertTo8Bit(RMat *rMatImage)
{
    cv::Mat matImage;
    double fac = 255.0 / rMatImage->getDataMax();
    if (rMatImage->matImage.channels() == 1)
    {
        matImage = rMatImage->matImage.clone();
    }
    else
    {
        matImage = rMatImage->matImageRGB.clone();
    }
    matImage.convertTo(matImage, CV_8U, fac);

    return matImage;
}

void RMainWindow::convertToNeg()
{
    int idx = ui->sliderFrame->value();
    cv::Mat matNeg = currentROpenGLWidget->getRMatImageList().at(idx)->matImage.clone();
    matNeg = currentROpenGLWidget->getRMatImageList().at(idx)->getDataMax() - matNeg;

    RMat *negRMat = new RMat(matNeg, false);
    negRMat->setImageTitle(QString("Negative"));
    negRMat->setInstrument(currentROpenGLWidget->getRMatImageList().at(idx)->getInstrument());
    createNewImage(negRMat);
}

void RMainWindow::solarColorizeSeriesSlot()
{
    QList<RMat*> coloredSeries;

    if (this->sender() == ui->wHAlphaPButton)
    {
        coloredSeries = processing->wSolarColorizeSeries(currentROpenGLWidget->getRMatImageList(), 'H');
    }

    createNewImage(coloredSeries);
}

void RMainWindow::initSharpenImageSlot(bool status)
{
    initPreviewQImage(status);

//    if (status)
//    {
//        sharpenLiveSlot();
//    }
}

void RMainWindow::sharpenImageSlot()
{
    float weight1 = ui->doubleSpinBoxW1->value();
    QList<RMat*> rMatSharpList = processing->sharpenSeries(currentROpenGLWidget->getRMatImageList(), weight1);

    createNewImage(rMatSharpList);
    autoScale();
}

void RMainWindow::sharpenSliderSlot()
{

    if (this->sender() == ui->sharpenSliderW1)
    {
        ui->doubleSpinBoxW1->setValue(ui->sharpenSliderW1->value()/10.0);
    }

}

void RMainWindow::changeZoomAxisSlot()
{
    if (limbFitPlot->selectedAxes().isEmpty()) { return; }

    if (limbFitPlot->selectedAxes().at(0) == limbFitPlot->xAxis)
    {
        limbFitPlot->axisRect(0)->setRangeZoomAxes(limbFitPlot->xAxis, limbFitPlot->yAxis);
        limbFitPlot->axisRect(0)->setRangeDragAxes(limbFitPlot->xAxis, limbFitPlot->yAxis);
        limbFitPlot->axisRect(0)->setRangeDrag(Qt::Horizontal);
        limbFitPlot->axisRect(0)->setRangeZoom(Qt::Horizontal);
    }
    else if (limbFitPlot->selectedAxes().at(0) == limbFitPlot->yAxis)
    {
        limbFitPlot->axisRect(0)->setRangeZoomAxes(limbFitPlot->xAxis, limbFitPlot->yAxis);
        limbFitPlot->axisRect(0)->setRangeDragAxes(limbFitPlot->xAxis, limbFitPlot->yAxis);
        limbFitPlot->axisRect(0)->setRangeDrag(Qt::Vertical);
        limbFitPlot->axisRect(0)->setRangeZoom(Qt::Vertical);
    }
    else if (limbFitPlot->selectedAxes().at(0) == limbFitPlot->yAxis2)
    {
        limbFitPlot->axisRect(0)->setRangeZoomAxes(limbFitPlot->xAxis2, limbFitPlot->yAxis2);
        limbFitPlot->axisRect(0)->setRangeDragAxes(limbFitPlot->xAxis2, limbFitPlot->yAxis2);
        limbFitPlot->axisRect(0)->setRangeDrag(Qt::Vertical);
        limbFitPlot->axisRect(0)->setRangeZoom(Qt::Vertical);
    }
    else
    {
        limbFitPlot->axisRect(0)->setRangeDrag(Qt::Vertical);
        limbFitPlot->axisRect(0)->setRangeZoom(Qt::Vertical);
    }

}

QVector<double> RMainWindow::extractTemperatureFromSeries()
{
    /// Not all ROpenGLWidget have temperature data, will need to account for this.
    QList<QTableWidget*> tableWidgetList = currentROpenGLWidget->getRListImageManager()->getTableWidgetList();
    QVector<double> temperatureSeries;
    for (int i = 0 ; i < tableWidgetList.size(); i++)
    {
        const QTableWidget* tableWidget = tableWidgetList.at(i);
        QTableWidgetItem * temperatureItem = tableWidget->findItems(QString("Temperature"), Qt::MatchFixedString).at(0);
        double temperature = tableWidgetList.at(i)->item(temperatureItem->row(), 1)->text().toDouble();
        temperatureSeries << temperature;
    }

    return temperatureSeries;
}

QVector<double> RMainWindow::extractMeanFromSeries()
{
    QVector<double> meanSeries = currentROpenGLWidget->getRListImageManager()->getMeanSeries();
    return meanSeries;
}

void RMainWindow::displayTemperatureSeries()
{
    QVector<double> temperatureSeries = extractTemperatureFromSeries();
    plotData(temperatureSeries, QString("Frame #"), QString("Temperature (Celsius)"));
}

void RMainWindow::displayMeanSeries()
{
    QVector<double> meanSeries = extractMeanFromSeries();
    plotData(meanSeries, QString("Frame #"), QString("Mean"));

}

void RMainWindow::plotData(QVector<double> data, QString xLabel, QString yLabel)
{
    QPoint windowPos(1,1);
    bool addWindow = false;

    if (plotSubWindow == NULL)
    {
        plotSubWindow = new QMdiSubWindow();
        addWindow = true;
    }
    else
    {
        // Restore last used position
        windowPos = plotSubWindow->pos();
    }

    // Calculate mean and median (commented for now )

    const auto mean = std::accumulate(data.begin(), data.end(), .0) / data.size();
    double yRangeMin = (double) 0.9*mean;
    double yRangeMax = (double) 1.1*mean;
    std::cout << "mean = " << mean << std::endl;
    std::cout << "yRangeMin = " << yRangeMin << std::endl;
    std::cout << "yRangeMax = " << yRangeMin << std::endl;
//    std::sort(data.begin(), data.end());
//        const auto median = data.size()%2
//            ? data[data.size() / 2]
//            : ((double)data[data.size() / 2 - 1] + data[data.size() / 2]) * .5;



    QVector<double> x(data.size());
    for (int i = 0 ; i < data.size() ; ++i)
    {
        x[i]  = i+1;
    }

    customPlot = new QCustomPlot();
    customPlot->addGraph();
    customPlot->plottable(0)->setPen(QPen(Qt::blue));
    customPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc));
    customPlot->xAxis->setLabel(xLabel);
    customPlot->xAxis->setAutoTicks(false);
    customPlot->xAxis->setTickVector(x);
    customPlot->yAxis->setLabel(yLabel);
    customPlot->graph(0)->setData(x, data);
    customPlot->graph(0)->rescaleAxes();
    customPlot->yAxis->setRange(yRangeMin, yRangeMax);

    // Allow the user to zoom in / zoom out and scroll horizontally
    customPlot->setInteraction(QCP::iRangeDrag, true);
    customPlot->setInteraction(QCP::iRangeZoom, true);
    customPlot->axisRect(0)->setRangeDrag(Qt::Vertical);
    customPlot->axisRect(0)->setRangeZoom(Qt::Vertical);

    plotSubWindow->setWidget(customPlot);
    if (addWindow)
    {
        ui->mdiArea->addSubWindow(plotSubWindow);
    }
    plotSubWindow->resize(defaultWindowSize);
    plotSubWindow->move(windowPos);
    plotSubWindow->show();

}


void RMainWindow::calibrateOffScreenSlot()
{
    processing->calibrateOffScreen();
    autoScale();
}

void RMainWindow::registerSlot()
{
    /// WHAT A MESS!!!!!!
    processing->setUseROI(ui->roiRadioButton->isChecked());
    processing->setApplyMask(ui->applyMaskCheckBox->isChecked());

    if (this->sender() != ui->phaseCorrPushB)
    {
        if (ui->limbFitCheckBox->isChecked())
        {
            processing->registerSeriesOnLimbFit();
            createNewImage(processing->getLimbFitResultList2());
            autoScale();
        }
        else if (ui->SADCheckBox->isChecked())
        {
            processing->registerSeriesCustomPropagate();
            createNewImage(processing->getResultList());
            autoScale();
        }
        else if (ui->templateMatchCheckBox->isChecked())
        {
            if (ui->propagateCheckBox->isChecked())
            {
                processing->registerSeriesByTemplateMatchingPropagate();
            }
            else
            {
                processing->registerSeriesByTemplateMatching();
            }

            createNewImage(processing->getResultList());
            autoScale();
        }
        else
        {
            if (ui->propagateCheckBox->isChecked())
            {
                // If displayFirstCheckBox is checked, we can as well load from the treeWidget.
                // This enables batch processing by adding as many different urls in there as we want
                // e.g coming from different directories <-> different drag'n'drops movements.
                std::cout << "ui->batchProcessCheckBox = " << ui->batchProcessCheckBox << std::endl;
                processing->setUseUrlsFromTreeWidget(ui->batchProcessCheckBox->isChecked());
                processing->registerSeriesXCorrPropagate(ui->roiRadioButton->isChecked(), ui->normalizeExposureCheckBox->isChecked());
            }
            else
            {
                processing->registerSeries();
            }
            createNewImage(processing->getResultList());
            autoScale();
        }
    }
    else
    {
        processing->registerSeriesByPhaseCorrelation();
        createNewImage(processing->getResultList());
        autoScale();
    }


}


void RMainWindow::stepForward()
{   
    int newValue = ui->sliderFrame->value() + 1;
    if (newValue > ui->sliderFrame->maximum())
    {
        newValue = 0;
    }
    qDebug() << "Slider frame Value = " << newValue;
    ui->sliderFrame->setValue(newValue);
    updateFrameInSeries(newValue);
}

void RMainWindow::stepBackward()
{
    int newValue = ui->sliderFrame->value() - 1;
    if (newValue < ui->sliderFrame->minimum())
    {
        newValue = ui->sliderFrame->maximum();
    }

    ui->sliderFrame->setValue(newValue);
    updateFrameInSeries(newValue);
}

void RMainWindow::playForward()
{
// Need to use QThread for using interface buttons. Otherwise intercept some keyboard key.
    stopButtonStatus = false;
    int fps = ui->spinBoxFps->value();
    ulong delay = (ulong) std::round( 1000.0f / (float) fps );
    while (stopButtonStatus == false)
    {
        qApp->processEvents();
        stepForward();
        QThread::msleep(delay);
        fps = ui->spinBoxFps->value();
        delay = (ulong) std::round( 1000.0f / (float) fps );
    }

}

void RMainWindow::increaseFps()
{
    int fps = ui->spinBoxFps->value();

    if (fps < 5)
    {
        fps = 5;
    }
    else
    {
        fps += 5;
    }

    ui->spinBoxFps->setValue(fps);
}

void RMainWindow::decreaseFps()
{
    int fps = std::max(1, ui->spinBoxFps->value() -5);
    ui->spinBoxFps->setValue(fps);
}

void RMainWindow::updateStats(int frameNumber)
{
    // Update stats
    ui->minStatLabel->setText(QString::number(currentROpenGLWidget->getRMatImageList().at(frameNumber)->getDataMin()));
    ui->maxStatLabel->setText(QString::number(currentROpenGLWidget->getRMatImageList().at(frameNumber)->getDataMax()));
    ui->meanStatLabel->setText(QString::number(currentROpenGLWidget->getRMatImageList().at(frameNumber)->getMean()));
    ui->sigmaStatLabel->setText(QString::number(currentROpenGLWidget->getRMatImageList().at(frameNumber)->getStdDev()));
    ui->medianStatLabel->setText(QString::number(currentROpenGLWidget->getRMatImageList().at(frameNumber)->getMedian()));
}

void RMainWindow::stopButtonPressed()
{
    stopButtonStatus = true;
    qDebug("Stop Button pressed");
}


void RMainWindow::radioRMatSlot()
{
    if (currentROpenGLWidget->getRMatImageList().isEmpty())
    {
        qDebug("nothing to radio out");
        return;
    }

    if (this->sender() == ui->lightRButton)
    {
        emit radioLightImages(currentROpenGLWidget->getRMatImageList());
    }

    if (this->sender() == ui->biasRButton)
    {
        emit radioBiasImages(currentROpenGLWidget->getRMatImageList());
    }

    if (this->sender() == ui->darkRButton)
    {
        emit radioDarkImages(currentROpenGLWidget->getRMatImageList());
    }

    if (this->sender() == ui->flatRButton)
    {
        emit radioFlatImages(currentROpenGLWidget->getRMatImageList());
    }
}



void RMainWindow::tileView()
{
    ui->mdiArea->setViewMode(QMdiArea::SubWindowView);
    ui->mdiArea->tileSubWindows();
}



void RMainWindow::on_actionHeader_toggled(bool arg1)
{
    if (arg1)
    {
        currentROpenGLWidget->setupTableWidget(ui->sliderFrame->value());
        addHeaderWidget();
    }
    else
    {
        if (currentROpenGLWidget->getTableRSubWindow() != NULL && currentROpenGLWidget->getTableRSubWindow()->isVisible())
        {
            currentROpenGLWidget->getTableRSubWindow()->hide();
        }
    }
}

void RMainWindow::uncheckActionHeaderState()
{
    ui->actionHeader->setChecked(false);
}



void RMainWindow::on_actionTileView_triggered()
{
    ui->mdiArea->tileSubWindows();
}

void RMainWindow::on_actionROISelect_triggered()
{

}

void RMainWindow::on_actionROISelect_toggled(bool arg1)
{
    if (arg1)
    {
        selectROI();
    }
    else
    {
        currentROpenGLWidget->setRoiSelected(false);
        currentROpenGLWidget->update();
        currentSubWindow->update();
    }

    //connect(currentROpenGLWidget, SIGNAL(roiSignal(QList<QRect>)), processing, SLOT(setupROILists(QList<QRect>)));
}

void RMainWindow::on_actionROIExtract_triggered()
{
    extractNewImageROI();
}


void RMainWindow::on_actionHeader_triggered()
{

}

void RMainWindow::on_actionTemperature_triggered()
{

}

void RMainWindow::on_actionTemperature_toggled(bool arg1)
{
    if (arg1)
    {
        if (currentROpenGLWidget != NULL)
        {
            displayTemperatureSeries();
        }
    }
    else
    {
        if (plotSubWindow != NULL)
        {
            plotSubWindow->hide();
        }
    }
}



void RMainWindow::on_actionCircle_toggled(bool arg1)
{
    if (arg1)
    {
        currentROpenGLWidget->setCircleSelected(true);
        connect(currentROpenGLWidget, SIGNAL(circleSignal(int,int,int)), processing, SLOT(setupMaskingCircle(int,int,int)));
        ui->actionROISelect->setChecked(false);
        ui->actionMultiROI->setChecked(false);
        currentROpenGLWidget->setRoiSelected(false);
    }
    else
    {
        currentROpenGLWidget->setCircleSelected(false);
        currentROpenGLWidget->update();
        currentSubWindow->update();
    }
}

void RMainWindow::on_actionMultiROI_toggled(bool arg1)
{
    if (arg1)
    {
        currentROpenGLWidget->setUseMultiROI(true);
    }
    else
    {
        currentROpenGLWidget->setUseMultiROI(false);
    }
}

void RMainWindow::on_actionSave_ROI_triggered()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    if (!currentROpenGLWidget->getFovRect().isValid() || currentROpenGLWidget->getFovRect().isEmpty())
    {
        tempMessageSignal(QString("Empty ROI. Need to select at least one ROI"), 10000);
        return;
    }

    processing->appendROIList(currentROpenGLWidget->getFovRect());
}

void RMainWindow::on_actionClear_ROIs_triggered()
{
    processing->clearROIs();
    currentROpenGLWidget->clearROIs();
    currentROpenGLWidget->update();
    currentSubWindow->update();
}

void RMainWindow::on_actionplot_metadata_triggered()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    displayMeanSeries();
}
