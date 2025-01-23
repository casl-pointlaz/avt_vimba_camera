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

#include <dpcore/dpcore.h>
#include <jetraw.h>

#include <string>

namespace avt_vimba_camera
{

    class MultiCamera
    {
    public:
        MultiCamera(ros::NodeHandle& nh, ros::NodeHandle& nhp);
        ~MultiCamera(void);

    private:
        std::shared_ptr<AvtVimbaApi> api_;
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
        CompressionType compressionType_;
        int qualityJPG_;
        bool allReady_ = false;
        bool allConfigure_ = false;

        // Calculating pixel intensity
        bool calculate_pixel_intensity_;

        // Debug Image
        bool debugImage_;
        bool compressInfo_;

        image_transport::ImageTransport it_;
        std::vector<std::shared_ptr<image_transport::CameraPublisher>> pub_;
        std::vector<std::shared_ptr<image_transport::CameraPublisher>> debugPub_;
        std::vector<std::shared_ptr<ros::Publisher>> pixel_intensity_pub_;


        // Dynamic reconfigure
        typedef avt_vimba_camera::AvtVimbaCameraConfig Config;
        typedef dynamic_reconfigure::Server<Config> ReconfigureServer;
        ReconfigureServer reconfigure_server_{nhp_};

        // Camera configuration
        Config camera_config_;

        void compressCallback(const FramePtr& ,const int camId=0);
        void configure(Config& newconfig, uint32_t level);

    };
}  // namespace avt_vimba_camera
#endif