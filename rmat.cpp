#include "rmat.h"
#include <qdebug.h>
#include <iostream>

RMat::RMat()
{
    cv::Mat emptyMat;
    this->matImage = emptyMat;
    this->bayer = false;
    this->bscale = 1;
    this->bzero = 0;
    this->imageTitle = QString("");
    this->instrument = instruments::generic;
    this->flipUD = false;

}

RMat::RMat(cv::Mat mat)
{
    initialize();
    mat.copyTo(this->matImage);
    prepImages();
}

RMat::RMat(const RMat &rMat)
    : flipUD(rMat.flipUD), bayer(rMat.bayer), bscale(rMat.bscale), bzero(rMat.bzero), dataMin(rMat.dataMin), dataMax(rMat.dataMax), expTime(rMat.expTime), XPOSURE(rMat.XPOSURE), TEMP(rMat.TEMP),
       SOLAR_R(rMat.SOLAR_R), wbRed(rMat.wbRed), wbGreen(rMat.wbGreen), wbBlue(rMat.wbBlue), instrument(rMat.instrument), imageTitle(QString("")),
       item(NULL)
{
    rMat.matImage.copyTo(this->matImage);
    prepImages();
}

RMat::RMat(cv::Mat mat, RMat *rMat) : flipUD(rMat->flipUD), bayer(rMat->bayer), bscale(rMat->bscale), bzero(rMat->bzero), dataMin(rMat->dataMin), dataMax(rMat->dataMax),
    expTime(rMat->expTime), XPOSURE(rMat->XPOSURE), TEMP(rMat->TEMP), SOLAR_R(rMat->SOLAR_R), wbRed(rMat->wbRed), wbGreen(rMat->wbGreen), wbBlue(rMat->wbBlue),
    instrument(rMat->instrument), imageTitle(rMat->imageTitle),
    item(NULL)
{
    mat.copyTo(this->matImage);
    prepImages();
}

RMat::RMat(cv::Mat mat, bool bayer) : flipUD(false), bayer(bayer), bscale(1), bzero(0), dataMin(0), dataMax(0), expTime(0), XPOSURE(0), TEMP(-100),
    SOLAR_R(0), wbRed(1.0), wbGreen(1.0), wbBlue(1.0), instrument(instruments::generic), imageTitle(QString("")),
    item(NULL)
{
    mat.copyTo(this->matImage);
    prepImages();
}

RMat::RMat(cv::Mat mat, bool bayer, instruments instrument) : flipUD(false), bayer(bayer), bscale(1), bzero(0), dataMin(0), dataMax(0),
    expTime(0), XPOSURE(0), TEMP(-100), SOLAR_R(0), wbRed(1.0), wbGreen(1.0), wbBlue(1.0), instrument(instrument), imageTitle(QString("")),
    item(NULL)
{
    mat.copyTo(this->matImage);
    prepImages();
}

RMat::RMat(cv::Mat mat, bool bayer, instruments instrument, float XPOSURE, float TEMP) : flipUD(false), bayer(bayer), bscale(1), bzero(0), dataMin(0), dataMax(0), expTime(0),
    XPOSURE(XPOSURE), TEMP(TEMP), SOLAR_R(0), wbRed(1.0), wbGreen(1.0), wbBlue(1.0), instrument(instrument), imageTitle(QString("")),
    item(NULL)
{
    mat.copyTo(this->matImage);
    prepImages();
}



RMat::~RMat()
{
//    if (item != NULL)
//    {

//        qDebug("RMat:: deleting item from QTreeWidget");
//        //item->~QTreeWidgetItem();
    //    }
}

void RMat::initialize()
{
    bayer = false;
    bscale = 1;
    bzero = 0;
    dataMin = 0;
    dataMax = 0;
    expTime = 1;
    XPOSURE = 1;
    TEMP = -100;
    SOLAR_R = 0;
    wbRed = 1.0;
    wbGreen = 1.0;
    wbBlue = 1.0;

    item = NULL;
    flipUD = false;
    imageTitle = QString("");
    instrument = instruments::generic;
}

void RMat::prepImages()
{
    std::cout << "RMat::PrepImages()" << std::endl;

    if (matImage.channels() == 1)
    {
        if (bayer)
        {
            std::cout << "RMat::PrepImages() Image is bayer. Converting..." << std::endl;

            matImage.convertTo(matImageRGB, CV_16U);
            cv::cvtColor(matImageRGB, matImageRGB, CV_BayerBG2RGB);
            cv::cvtColor(matImageRGB, matImageGray, CV_RGB2GRAY);
        }
        else
        {
            std::cout << "RMat::PrepImages() Image is not bayer. Gray image is assigned to the original array" << std::endl;

            matImageGray = matImage;
            cv::cvtColor(matImage, matImageRGB, CV_GRAY2RGB);
        }
    }
    else // Assumes 3 channels?
    {
        if (instrument != instruments::TIFF)
        {
            std::cout << "RMat::PrepImages() Image is not TIFF" << std::endl;

            cv::cvtColor(matImage, matImageGray, CV_RGB2GRAY);
            matImageRGB = matImage;
        }
        else
        {
            std::cout << "RMat::PrepImages() Image is TIFF" << std::endl;

            cv::cvtColor(matImage, matImageGray, CV_BGR2GRAY);
            cv::cvtColor(matImage, matImageRGB, CV_BGR2RGB);
        }

    }


    calcStats();
}

void RMat::computeHist(int nBins, float minRange, float maxRange)
{
    float range[2];
    range[0] = minRange;
    range[1] = maxRange;
    const float* histRange = { range };
    bool uniform = true;
    bool accumulate = false;

    /// Compute the histograms:

    if (minRange == maxRange)
    {
       matHist = cv::Mat::zeros(nBins, 1, CV_32F);
    }
    else
    {
        cv::calcHist( &matImageGray, 1, 0, cv::Mat(), matHist, 1, &nBins, &histRange, uniform, accumulate);   
    }

}

void RMat::calcStats()
{
    qDebug("RMat::calcStats()");

    // Calculate min and max from matImageGray
    calcMinMax();
    qDebug("RMat::calcStats():: [dataMin , dataMax] = [%f , %f]", (float) dataMin, (float) dataMax);


    /// Determine a data range for the sliders and histogram
    /// These params depend on the instrument's sensor (different bit depths).

    int matType = matImage.type();
    /// Histogram bin counts

    if (instrument == instruments::USET)
    {
        dataRange = 4096.0f;
        normalizeRange = 4096.0f;
        maxHistRange = 4095.0f;
    }
    else if (instrument == instruments::DSLR)
    {
        // DSLRs like Canon EOS are typically 14-bit sensors = 16384
        dataRange = 16384.0f;
        normalizeRange = 16384.0f;
        maxHistRange = 16383.0f;
    }
    else if (matType == CV_16U || matType == CV_16UC3)
    {
        dataRange = 65536.0f;
        normalizeRange = 65536.0f;
        maxHistRange = 65535.0f;
    }
    else if (matType == CV_32F || matType == CV_32FC3)
    {
        if (dataMin >= 0 && dataMax <= 1)
        {
            // Image is considered normalized. Restore positive integral values ranging over 16 bit
            matImage = matImage * 65535.0f;
            calcMinMax();
        }
        dataRange = (float) (dataMax - dataMin);
        /// For normalization / contrast stretching
        /// we assume that no CCD will go beyond 16 bits
        /// thus keeping a maximum 16-bit data range
        normalizeRange = 65536.0f;
        maxHistRange = 65535.0f;
    }
    else if (matImage.type() == CV_8U || matImage.type() == CV_8UC3)
    {
        dataRange = 256.0f;
        normalizeRange = 256.0f;
        maxHistRange = 255.0f;
    }
    else
    {
        maxHistRange = (float) dataMax;
    }

    cv::Scalar meanScalar;
    cv::Scalar stdDevScalar;
    cv::meanStdDev(matImageGray, meanScalar, stdDevScalar);
    mean = (float) meanScalar.val[0];
    stdDev = (float) stdDevScalar.val[0];
    nPixels = (uint) matImage.cols * matImage.rows;


    // The histogram may start from negative values, so the minimum may not always be 0;
    minHistRange = std::min(0.0f, (float) dataMin) ;
    // Get histogram
    int nBins = (int) dataRange;
    histWidth = (maxHistRange - minHistRange)/(double) (nBins);
    std::cout << "Calculating histogram..." << std::endl;
    std::cout << "minHistRange = " << minHistRange << std::endl;
    std::cout << "maxHistRange = " << maxHistRange << std::endl;
    computeHist(nBins, minHistRange, maxHistRange);

   // Calculate median
    median = calcMedian(histWidth, minHistRange);
    std::cout << "Median = " << median << std::endl;

    /// Define percentiles for the low and high thresholds
    /// i.e, pertcentage of pixels below the lowest intensity
    /// and above the highest intensity
    float cutOffLow;
    float cutOffHigh;


    if (instrument == instruments::USET)
    {
        cutOffLow = 0;
        cutOffHigh = 99.97f;
    }
    else
    {
        cutOffLow = 5.0f;
        cutOffHigh = 99.85f;
    }

    intensityLow = calcThreshold(cutOffLow, histWidth, minHistRange);
    intensityHigh = calcThreshold(cutOffHigh, histWidth, minHistRange);

    if (intensityLow == intensityHigh)
    {
        intensityHigh = intensityLow + 1;
    }
}

void RMat::calcMinMax()
{
    if (matImageGray.empty())
    {
        matImageGray = matImage;
    }

    cv::minMaxLoc(matImageGray, &dataMin, &dataMax);
}

cv::Mat RMat::extractChannel(unsigned int channel)
{
    cv::Mat planesRGB[3];
    cv::split(matImageRGB, planesRGB);
    cv::Mat matPlane = planesRGB[channel];

    return matPlane;
}



float RMat::calcMedian(double histWidth, float minRange)
{
//    std::cout << "Calculating median..." << std::endl;
//    float cdf = 0;
//    int nBins = matHist.rows;
//    std::cout << "nBins = " << nBins << std::endl;

//    float medianLimit = std::round((float)nBins/2.0f);
//    std::cout << "medianLimit = " << medianLimit << std::endl;

//    float medianVal = -1;
//    for (int i = 1; i < nBins && medianVal < 0.0; i++)
//    {
//        cdf += matHist.at<float>(i);

//        QString qs = QString("cdf(i=%1) = %2").arg(i).arg(cdf);
//        std::cout << qs.toStdString() << std::endl;

//        if (cdf > medianLimit) { medianVal = i;}
//    }
    float median = calcThreshold(50.0f, histWidth, minRange);

    return median;
}

float RMat::calcThreshold(float cutOff, double histWidth, float minRange)
{
    float cdf = 0.0f;
    bool checkThreshold = false;
    int nBins = matHist.rows;

    int index = 0;
    int i = 0;
    while (i < nBins && checkThreshold == false){
        cdf += matHist.at<float>(i);
        checkThreshold = (100.0f * cdf/nPixels > cutOff);
        if (checkThreshold) { index = i;}
        i++;
    }


    /// Sometimes the percentile isn't reach (for example after normalizing)
    /// This makes sure that when the maximum percentile is not reached even
    /// when the last bin has been taken into the cdf, the index is set to the last bin.
    if (!checkThreshold)
    {
        index = nBins - 1;
    }

   float intensity = minRange + (float) histWidth * (float) index;

   return intensity;
}

// Getters
bool RMat::isBayer() const
{
    return bayer;
}

float RMat::getBscale() const
{
    return bscale;
}

int RMat::getBzero() const
{
    return bzero;
}

double RMat::getDataMin() const
{
    return dataMin;
}

double RMat::getDataMax() const
{
    return dataMax;
}

float RMat::getExpTime() const
{
    return expTime;
}

float RMat::getXPOSURE() const
{
    return XPOSURE;
}

float RMat::getTEMP() const
{
    return TEMP;
}

float RMat::getSOLAR_R() const
{
    return SOLAR_R;
}

float RMat::getWbRed() const
{
    return wbRed;
}

float RMat::getWbGreen() const
{
    return wbGreen;
}

float RMat::getWbBlue() const
{
    return wbBlue;
}

instruments RMat::getInstrument() const
{
    return instrument;
}

QString RMat::getImageTitle() const
{
    return imageTitle;
}

cv::Mat RMat::getMatHist() const
{
    return matHist;
}

float RMat::getMean() const
{
    return mean;
}

float RMat::getStdDev() const
{
    return stdDev;
}

float RMat::getMedian() const
{
    return median;
}

float RMat::getIntensityLow() const
{
    return intensityLow;
}

float RMat::getIntensityHigh() const
{
    return intensityHigh;
}

double RMat::getHistWidth() const
{
    return histWidth;
}

uint RMat::getNPixels() const
{
    return nPixels;
}

float RMat::getMinHistRange() const
{
    return minHistRange;
}

float RMat::getMaxHistRange() const
{
    return maxHistRange;
}

float RMat::getDataRange() const
{
    return dataRange;
}

float RMat::getNormalizeRange() const
{
    return normalizeRange;
}

QTreeWidgetItem* RMat::getItem() const
{
    return item;
}

QFileInfo RMat::getFileInfo() const
{

    return fileInfo;
}

QString RMat::getDate_obs() const
{
    return date_obs;
}

QString RMat::getTime_obs() const
{
    return time_obs;
}

QString RMat::getDate_time() const
{
    return date_time;
}

QUrl RMat::getUrl() const
{
    return url;
}


// Setters
void RMat::setBayer(bool bayer)
{
    this->bayer = bayer;
}

void RMat::setBscale(float bscale)
{
    this->bscale = bscale;
}

void RMat::setBzero(int bzero)
{
    this->bzero = bzero;
}

void RMat::setDataMin(float dataMin)
{
    this->dataMin = dataMin;
}

void RMat::setDataMax(float dataMax)
{
    this->dataMax = dataMax;
}

void RMat::setExpTime(float expTime)
{
    this->expTime = expTime;
}

void RMat::setSOLAR_R(float SOLAR_R)
{
    this->SOLAR_R = SOLAR_R;
}

void RMat::setXPOSURE(float XPOSURE)
{
    this->XPOSURE = XPOSURE;
}

void RMat::setTEMP(float temperature)
{
    this->TEMP = temperature;
}

void RMat::setWbRed(float wbRed)
{
    this->wbRed = wbRed;
}

void RMat::setWbGreen(float wbGreen)
{
    this->wbGreen = wbGreen;
}

void RMat::setWbBlue(float wbBlue)
{
    this->wbBlue = wbBlue;
}

void RMat::setImageTitle(QString title)
{
    this->imageTitle = title;
}

void RMat::setInstrument(instruments instrument)
{
    this->instrument = instrument;
}

void RMat::setItem(QTreeWidgetItem *item)
{
    this->item = item;
}

void RMat::setFileInfo(QFileInfo fileInfo)
{
    this->fileInfo = fileInfo;
}

void RMat::setDate_obs(QString date_obs)
{
    this->date_obs = date_obs;
}

void RMat::setTime_obs(QString time_obs)
{
    this->time_obs = time_obs;
}

void RMat::setDate_time(QString date_time)
{
    this->date_time = date_time;
}

void RMat::setUrl(QUrl url)
{
    this->url = url;
}


