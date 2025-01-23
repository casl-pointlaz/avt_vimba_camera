/// Copyright (c) 2014,
/// Systems, Robotics and Vision Group
/// University of the Balearic Islands
/// All rights reserved.
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions are met:
///     * Redistributions of source code must retain the above copyright
///       notice, this list of conditions and the following disclaimer.
///     * Redistributions in binary form must reproduce the above copyright
///       notice, this list of conditions and the following disclaimer in the
///       documentation and/or other materials provided with the distribution.
///     * All advertising materials mentioning features or use of this software
///       must display the following acknowledgement:
///       This product includes software developed by
///       Systems, Robotics and Vision Group, Univ. of the Balearic Islands
///     * Neither the name of Systems, Robotics and Vision Group, University of
///       the Balearic Islands nor the names of its contributors may be used
///       to endorse or promote products derived from this software without
///       specific prior written permission.
///
/// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
/// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
/// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
/// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
/// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
/// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
/// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef AVT_VIMBA_CAMERA_H
#define AVT_VIMBA_CAMERA_H

#include <VimbaCPP/Include/VimbaCPP.h>

#include <avt_vimba_camera/AvtVimbaCameraConfig.h>
#include <avt_vimba_camera/frame_observer.h>
#include <avt_vimba_camera/avt_vimba_api.h>
#include <camera_info_manager/camera_info_manager.h>
#include <image_transport/image_transport.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/CompressedImage.h>
#include <sensor_msgs/CameraInfo.h>
#include <std_msgs/UInt8.h>
#include <cv_bridge/cv_bridge.h>
#include <thread>
#include <opencv2/opencv.hpp>

#include <string>
#include <mutex>

using AVT::VmbAPI::CameraPtr;
using AVT::VmbAPI::FramePtr;
using AVT::VmbAPI::IFrameObserverPtr;
using AVT::VmbAPI::VimbaSystem;

namespace avt_vimba_camera
{
enum CameraState
{
  OPENING,
  IDLE,
  CAMERA_NOT_FOUND,
  FORMAT_ERROR,
  ERROR,
  OK
};


class AvtVimbaCamera
{
public:
  typedef avt_vimba_camera::AvtVimbaCameraConfig Config;
  typedef std::function<void(const FramePtr,const int i)> compressCallbackFunc;         // Modified by pointlaz. camId as parameter

  AvtVimbaCamera();
  AvtVimbaCamera(const std::string& name, const int camId = 0, std::shared_ptr<AvtVimbaApi> api = nullptr,
                 std::shared_ptr<image_transport::CameraPublisher> pub = nullptr
                 );         // Modified by pointlaz. camId as parameter

 //Publishers
  std::shared_ptr<image_transport::CameraPublisher> pub_;
  std::shared_ptr<image_transport::CameraPublisher> debugPub_;
  std::shared_ptr<ros::Publisher> pixel_intensity_pub_;


  //Compressing
  CompressionType compressionType_ = CompressionType::Jpeg;
  int qualityJPG_ = 90;
  void setPixelIntensityPublisher(std::shared_ptr<ros::Publisher> pub) {if(pub) pixel_intensity_pub_ = pub;}

  //Debug Pupub_blisher
  void setDebugPublisher(std::shared_ptr<image_transport::CameraPublisher> pub)
  {
      ROS_WARN("DEBUG PUB ACTIVATED WILL SLOW DOWN PROCESSING BUT PUBLISH RGB IMAGES");
      if(pub) debugPub_ = pub;
  }


  void start(const std::string& ip_str, const std::string& guid_str, const std::string& frame_id,
             bool print_all_features = false);
  void stop();

  void updateConfig(Config& config);
  void startImaging();
  void stopImaging();

  // Utility functions
  bool isOpened()
  {
    return opened_;
  }

  // Getters
  int getSensorWidth();
  int getSensorHeight();

  // Setters
  void setCallback(compressCallbackFunc callback)
  {
    userFrameCallback = callback;
  }
   // Created by pointlaz
   int camId_;
  bool opened_ = false;
  bool streaming_ = false;
  bool on_init_ = false ;
  bool connected_ = false;

  ~AvtVimbaCamera()
  {
      stop();
      if(frame_obs_ptr_) frame_obs_ptr_.reset();
      if (vimba_frame_ptr_) vimba_frame_ptr_.reset();
      if (vimba_camera_ptr_) vimba_camera_ptr_.reset();
      std::cout<< "cam clean finish" << std::endl;
  }

    void compress(const FramePtr& vimba_frame_ptr);

    Config config_;
private:
  std::shared_ptr<AvtVimbaApi> api_;
  // IFrame Observer
  SP_DECL(FrameObserver) frame_obs_ptr_;
  // The currently streaming camera
  CameraPtr vimba_camera_ptr_;
  // Current frame
  FramePtr vimba_frame_ptr_;
  // Mutex
  std::mutex config_mutex_;

  CameraState camera_state_;

  std::string name_;
  std::string guid_;
  std::string frame_id_;


  // Created by pointlaz to store camId
  CameraPtr openCamera(const std::string& id_str, bool print_all_features);

  compressCallbackFunc userFrameCallback;
  void frameCallback(const FramePtr vimba_frame_ptr);       // Modified by pointlaz. camId as parameter

  template <typename T>
  VmbErrorType setFeatureValue(const std::string& feature_str, const T& val);
  template <typename T>
  bool getFeatureValue(const std::string& feature_str, T& val);
  bool getFeatureValue(const std::string& feature_str, std::string& val);
  template <typename Vimba_Type, typename Std_Type>
  void configureFeature(const std::string& feature_str, const Vimba_Type& val_in, Std_Type& val_out);
  void configureFeature(const std::string& feature_str, const std::string& val_in, std::string& val_out);
  bool runCommand(const std::string& command_str);
  void printAllCameraFeatures(const CameraPtr& camera);

  void updateAcquisitionConfig(Config& config);
  void updateExposureConfig(Config& config);
  void updateGammaConfig(Config& config);
  void updateDspsubregionConfig(Config& config);
  void updateGainConfig(Config& config);
  void updateWhiteBalanceConfig(Config& config);
  void updateImageModeConfig(Config& config);
  void updateROIConfig(Config& config);
  void updateBandwidthConfig(Config& config);
  void updatePixelFormatConfig(Config& config);
  void updatePtpModeConfig(Config& config);
  void updateGPIOConfig(Config& config);
  void updateUSBGPIOConfig(Config& config);
  void updateIrisConfig(Config& config);

};
}  // namespace avt_vimba_camera
#endif
