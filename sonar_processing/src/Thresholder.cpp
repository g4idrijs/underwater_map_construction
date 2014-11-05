#include "Thresholder.h"
#include <pluginlib/class_list_macros.h>

namespace sonar_processing
{
Thresholder::Thresholder(){}

Thresholder::~Thresholder(){}

void Thresholder::onInit()
{
    NODELET_INFO("Initializing thresholder nodelet...");
    nh_ = getPrivateNodeHandle();

    // Get all the parameters
    nh_.param("mode", mode_, string("OTSU")); // {OTSU | FIXED}[PROPORTIONAL]
    nh_.param("maxBinValue", maxBinValue_, int(128));
    nh_.param("OTSUMultiplier", OTSUMultiplier_, double(1.0));
    nh_.param("maxThresholdProportion", maxThresholdProportion_, double(0.5));
    nh_.param("minThresholdProportion", minThresholdProportion_, double(0.5));

    nh_.param("cloudsubscribeTopic", cloudSubscribeTopic_, string("/sonar/scan/sonarCloud"));
    nh_.param("thresholdedCloudPublishTopic", thresholdedCloudPublishTopic_, string("/sonar/scan/thresholdedCloud"));


    // Subscribe to incoming sonar data from driver
    cloudSubscriber_ = nh_.subscribe(cloudSubscribeTopic_.c_str(), 0, &Thresholder::cloudCallback, this);
}

void Thresholder::CloudCallback(sensor_msgs::PointCloud2ConstPtr cloudMessagePtr){
    //Cloud for thresholding
    intensityCloud::Ptr cloudPtr = boost::make_shared<intensityCloud>();
    //Conver message to intensityCloud
    pcl::fromROSMsg(cloudMessagePtr, cloudPtr);
    //Threshold cloud
    ThresholdCloud(cloudPtr);
    //Publish cloud
    publishCloud(cloud);
}

void Thresholder::ThresholdCloud(intensityCloud::Ptr cloudPtr)
{
    double threshold = 0;
    if (mode_.find("OTSU") != -1){
        threshold = getOTSUThreshold(cloudPtr);
        if (mode_.find("PROPOTIONAL") != -1) threshold = threshold * thresholdProportion_;
    } else if (mode_.find("FIXED") != -1){
        threshold = fixedThresholdValue_;
    } if (mode_.find("PROPORTIONAL") != -1){
        int maxIntensity = 0, minIntensity = maxBinValue_;
        intensityCloud::iterator cloudIterator;
        for (cloudIterator = cloudPtr->begin();cloudIterator != cloudPtr->end();cloudIterator++){
            if (cloudIterator->intensity > maxFixed) maxFixed = cloudIterator->intensity;
            if (cloudIterator->intensity < minFixed) minFixed = cloudIterator->intensity;
        }
        threshold = maxThresholdProportion_ * maxIntensity + minThresholdProportion_ * minIntensity;
    }

    // Set input cloud
    passthroughFilter.setInputCloud(cloudPtr);
    // Field we want to filter (intensity)
    passthroughFilter.setFilterFieldName("intensity");
    // Set range of intensity values accepted
    passthroughFilter.setFilterLimits(threshold, maxBinValue_+1);
    // Keep organised setting removed points to NaN
    passthroughFilter.setKeepOrganized(false);
    // Apply filter
    passthroughFilter.filter(*cloudPtr);
}

void Thresholder::getOTSUThreshold(intensityCloud::Ptr cloudPtr){

}

void Thresholder::publishCloud(pcl::PointCloud::Ptr cloudPtr){
    sensor_msgs::PointCloud2 cloudMessage;
    pcl::toROSMsg(*cloudPtr,cloudMessage);
    cloudMessage.header.frame_id = "/map";
    sonarCloudPublisher_.publish(cloudMessage);
}

PLUGINLIB_DECLARE_CLASS(sonar_processing, Thresholder, sonar_processing::Thresholder, nodelet::Nodelet)
}