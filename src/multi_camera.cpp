/// Created by pointlaz

#include <avt_vimba_camera/multi_camera.h>
#include "BS_thread_pool.hpp"

#define DEBUG_PRINTS 1

namespace avt_vimba_camera
{
    MultiCamera::MultiCamera(ros::NodeHandle& nh, ros::NodeHandle& nhp)
            : nh_(nh), nhp_(nhp), it_(nhp)
    {


        // Set the params
        nhp_.param("camera_qty", camQty_, 1);
        std::shared_ptr<BS::thread_pool<>>  pool = std::make_shared<BS::thread_pool<>>(camQty_ * 2);
        api_ = std::make_shared<AvtVimbaApi>(pool);
        api_->start();

        std::string compressionType;
        nhp_.param<std::string>("compression_type", compressionType, "jpeg");
        if (compressionType == "jpeg")
        {
            compressionType_ = CompressionType::Jpeg;
        }
        if (compressionType == "jpegTurbo")
        {
          api_->setJpegTurboHandlers();
          compressionType_ = CompressionType::JpegTurbo;
        }
        else if (compressionType == "jetraw")
        {
          compressionType_ = CompressionType::Jetraw;
        }
        else if (compressionType == "none")
        {
          compressionType_ = CompressionType::None;
        }
        else
        {
            ROS_WARN_STREAM("compression_type = '" << compressionType << "' bad value. Must be 'jpeg', 'jpegTurbo', 'jetraw' or 'none'. Set to default value 'jpeg'");
        }

        std::string topicName = "image_raw_";
        nhp_.param("compress_jpeg_quality", qualityJPG_, 90);
        // Get Pixel Intensity params
        nhp_.param("calculate_pixel_intensity", calculate_pixel_intensity_,true);


        //Debug Image
        nhp_.param("debug_image", debugImage_,false);
        nhp_.param("compress_info", compressInfo_,false);

        guid_.resize(camQty_);
        pub_.resize(camQty_);
        camera_info_url_.resize(camQty_);
        frame_id_.resize(camQty_);
        cam_.resize(camQty_);
        name_.resize(camQty_);

        if(calculate_pixel_intensity_)
        {
            int pixel_intensity_pixel_steps, pixel_intensity_saturation_value, pixel_intensity_moving_average_k;
            double pixel_intensity_saturated_threshold;
            bool pixel_intensity_count_saturated_pixels_in_mean, pixel_intensity_echo, pixel_intensity_use_moving_average;
            std::vector<int> echoed_pixel_intensities;
            nhp_.param("pixel_intensity_pixel_steps", pixel_intensity_pixel_steps,10);
            nhp_.param("pixel_intensity_saturated_threshold", pixel_intensity_saturated_threshold,0.1);
            nhp_.param("pixel_intensity_saturation_value", pixel_intensity_saturation_value,255);
            nhp_.param("pixel_intensity_use_moving_average", pixel_intensity_use_moving_average,true);
            nhp_.param("pixel_intensity_moving_average_k", pixel_intensity_moving_average_k,5);
            nhp_.param("pixel_intensity_count_saturated_pixels_in_mean", pixel_intensity_count_saturated_pixels_in_mean, true);
            nhp_.param("pixel_intensity_echo", pixel_intensity_echo,false);
            nhp_.param<std::vector<int>>("echoed_pixel_intensities", echoed_pixel_intensities, {});
            api_->setPixelIntensityParameters(pixel_intensity_pixel_steps, pixel_intensity_saturated_threshold, pixel_intensity_saturation_value, pixel_intensity_count_saturated_pixels_in_mean, pixel_intensity_use_moving_average, pixel_intensity_moving_average_k, camQty_, pixel_intensity_echo, echoed_pixel_intensities);
            pixel_intensity_pub_.resize(camQty_);
        }

        if (compressionType_ == CompressionType::Jetraw)
        {
            auto res = dpcore_init();
            ROS_INFO_STREAM("JETRAW------------ INIT STATUS " << std::to_string(res));
            if (res <= 1)
            {
                ROS_INFO("JETRAW------------ACTIVATED");
            }
            else
            {
                ROS_WARN("JETRAW------------INIT FAILED FALL BACK TO JPG");
                compressionType_ = CompressionType::Jpeg;
            }
        }
        if (compressionType_ == CompressionType::Jpeg || compressionType_ == CompressionType::JpegTurbo) {
            api_->qualityJPG_ = qualityJPG_;
        }
        api_->compressionType_ = compressionType_;

        if (debugImage_)
        {
            api_->activateDebugImage();
            debugPub_.resize(camQty_);
        }

        if (compressInfo_)
        {
            api_->compressInfo_ = compressInfo_;
        }

        for (int i = 0; i < camQty_; i++)
        {
            pub_[i].reset(new image_transport::CameraPublisher);
            *pub_[i] = it_.advertiseCamera(topicName + std::to_string(i), 1);

            nhp_.param("guid_" + std::to_string(i), guid_[i], std::string(""));
            nhp_.param("camera_info_url_" + std::to_string(i), camera_info_url_[i], std::string(""));
            nhp_.param("frame_id_" + std::to_string(i), frame_id_[i], std::string(""));
            nhp_.param("name_" + std::to_string(i), name_[i], std::string(""));
            nhp_.param("print_all_features", print_all_features_, false);
            nhp_.param("use_measurement_time", use_measurement_time_, false);
            nhp_.param("ptp_offset", ptp_offset_, 0);

            ROS_INFO("-------------New Cam");
            std::shared_ptr<AvtVimbaCamera>
                cam = std::shared_ptr<AvtVimbaCamera>(new AvtVimbaCamera(frame_id_[i], i, api_, pub_[i]));
            if (calculate_pixel_intensity_)
            {
                ROS_INFO("-------------Color Intensity");
                pixel_intensity_pub_[i].reset(new ros::Publisher);
                *pixel_intensity_pub_[i] = nh_.advertise<std_msgs::UInt8>("/multi_camera/pixel_intensity_" + std::to_string(i), 1);
                cam->setPixelIntensityPublisher(pixel_intensity_pub_[i]);
            }
            if (debugImage_)
            {
                ROS_INFO("-------------Debug Image Publisher");
                debugPub_[i].reset(new image_transport::CameraPublisher);
                *debugPub_[i] = it_.advertiseCamera(topicName + std::to_string(i)+"_debug", 1);
                cam->setDebugPublisher(debugPub_[i]);
            }
            cam_[i] = cam;
        }

        ROS_INFO("-------------Reconfig");
        reconfigure_server_.setCallback(
            std::bind(&avt_vimba_camera::MultiCamera::configure,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2));
        reconfigure_server_.clearCallback();
    }

    MultiCamera::~MultiCamera()
    {
        cam_.clear();
        if(api_)api_.reset();
        pub_.clear();
        pixel_intensity_pub_.clear();
        reconfigure_server_.clearCallback();
        std::cout<< "multi clean finish" << std::endl;
    }



/** Dynamic reconfigure callback
 *
 *  Called immediately when callback first defined. Called again
 *  when dynamic reconfigure starts or changes a parameter value.
 *
 *  @param newconfig new Config values
 *  @param level bit-wise OR of reconfiguration levels for all
 *               changed parameters (0xffffffff on initial call)
 **/
    void MultiCamera::configure(Config& newconfig, uint32_t level)
    {
        ROS_WARN_STREAM("-------------Configure");
        for (int i = 0 ; i < cam_.size();i++)
        {
            ROS_INFO("-------------------------------CAMERA %d", i);
            // The camera already stops & starts acquisition
            // so there's no problem on changing any feature.
            if (cam_[i]->isOpened())
            {
                ROS_WARN_STREAM("-------------STOP IMAGING CAM " << i);
                cam_[i]->stopImaging();
            }
            if (!cam_[i]->isOpened())
            {
                ROS_WARN_STREAM("-------------START CAM " << i);
                cam_[i]->start(ip_, guid_[i], frame_id_[i], print_all_features_);
            }

            if (cam_[i]->isOpened())
            {
                ROS_WARN_STREAM("-------------UPDATE CONFIG CAM " << i);
                cam_[i]->updateConfig(newconfig);
            }
            if (cam_[i]->connected_)
            {
                ROS_WARN_STREAM("-------------START IMAGING CAM " << i);
                cam_[i]->startImaging();
            }

        }
    }



}  // namespace avt_vimba_camera
