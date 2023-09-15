/// Created by pointlaz

#ifndef MULTI_CAMERA_H
#define MULTI_CAMERA_H

#include <avt_vimba_camera/avt_vimba_camera.h>
#include <avt_vimba_camera/AvtVimbaCameraConfig.h>
#include <avt_vimba_camera/avt_vimba_api.h>

#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/CompressedImage.h>
#include <sensor_msgs/CameraInfo.h>
#include <std_msgs/UInt8.h>
#include <camera_info_manager/camera_info_manager.h>
#include <image_transport/image_transport.h>
#include <dynamic_reconfigure/server.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

#include "dpcore/dpcore.h"
#include "jetraw/jetraw.h"
#include "jetraw_tiff/jetraw_tiff.h"

#include <string>

namespace avt_vimba_camera
{
    class MultiCamera
    {
    public:
        MultiCamera(ros::NodeHandle& nh, ros::NodeHandle& nhp);
        ~MultiCamera(void);

    private:
        AvtVimbaApi api_;
        std::vector<std::shared_ptr<AvtVimbaCamera>> cam_;

        ros::NodeHandle nh_;
        ros::NodeHandle nhp_;

        std::string ip_;
        std::vector<std::string> guid_;
        std::vector<std::string> name_;
        std::vector<std::string> camera_info_url_;
        std::vector<std::string> frame_id_;
        bool print_all_features_;
        bool use_measurement_time_;
        int32_t ptp_offset_;
        int camQty_;
        std::string settings_path_;
        int action_device_key_;
        int action_group_key_;
        int action_group_mask_;

        //Compressing
        std::string compressionMode_;
        int qualityJPG_;
        std::string jetraw_calibration_path_;

        //Calculating color intensity
        bool calculateColorIntensity_;
        std::string colorIntensityRGB_;
        int colorIntensityPxSteps_;
        uint8_t calculateColorIntensity(cv::Mat &img);


        image_transport::ImageTransport it_;
        std::vector<image_transport::CameraPublisher> pub_;
        std::vector<ros::Publisher> colorPub_;

        std::vector<std::shared_ptr<camera_info_manager::CameraInfoManager>> info_man_;

        // Dynamic reconfigure
        typedef avt_vimba_camera::AvtVimbaCameraConfig Config;
        typedef dynamic_reconfigure::Server<Config> ReconfigureServer;
        ReconfigureServer reconfigure_server_{nhp_};

        // Camera configuration
        Config camera_config_;

        void frameCallback(const FramePtr& ,const int camId=0);
        void configure(Config& newconfig, uint32_t level);
        void updateCameraInfo(const Config& config, const int camId=0);

    };
}  // namespace avt_vimba_camera
#endif