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

#ifndef AVT_VIMBA_API_H
#define AVT_VIMBA_API_H

#include <VimbaCPP/Include/VimbaCPP.h>
#include "VimbaCPP/Include/VmbTransform.h"

#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/image_encodings.h>
#include <sensor_msgs/fill_image.h>
#include <std_msgs/UInt8.h>

#include <string>
#include <map>

#include <jetraw.h>
#include <cv_bridge/cv_bridge.h>
#include <thread>
#include <opencv2/opencv.hpp>
#include "BS_thread_pool.hpp"
#include <avt_vimba_camera/jpegTurbo.h>

using AVT::VmbAPI::CameraPtr;
using AVT::VmbAPI::FramePtr;
using AVT::VmbAPI::VimbaSystem;

namespace avt_vimba_camera
{
enum class CompressionType
{
  Jpeg,
  JpegTurbo,
  Jetraw,
  None
};

class AvtVimbaApi
{
public:
  AvtVimbaApi(
              const std::shared_ptr<BS::thread_pool<>> &threadPool = nullptr)
      : vs(VimbaSystem::GetInstance())
        ,threadPool_(threadPool)
  {
  }

    VimbaSystem& vs;      // Modified by pointlaz. vs is now public

    // Pixel Intensity Calculation
    int pixel_intensity_pixel_steps_;               // A pixel will be taken every 'pixel_intensity_pixel_steps' to do the pixel intensity calculation
    double pixel_intensity_saturated_threshold_;    // The ratio of saturated pixels for which the image is considered saturated
    int pixel_intensity_saturation_value_;          // The value for which a pixel is considered saturated
    bool pixel_intensity_count_saturated_pixels_in_mean_;
    bool pixel_intensity_use_moving_average_;
    int pixel_intensity_moving_average_k_;
    int cam_qty_;
    std::vector<std::vector<VmbUint8_t >> vector_of_values_per_cam_for_moving_average;
    bool pixel_intensity_echo_;
    std::vector<int> echoed_pixel_intensities_;
    bool pixel_intensity_ = false ;
    CompressionType compressionType_ = CompressionType::Jpeg;
    int qualityJPG_ = 90;

    //Debug image
    bool debugImage_ = false;
    bool compressInfo_ = false;

    //pool
    std::shared_ptr<BS::thread_pool<>> threadPool_;
    std::vector<std::unique_ptr<TurboJpegHandler>> jpegTurboHandlers_;

  void start()
  {
    VmbErrorType err = vs.Startup();
    if (VmbErrorSuccess == err)
    {
      ROS_INFO_STREAM("[Vimba System]: AVT Vimba System initialized successfully");
      listAvailableCameras();
    }
    else
    {
      ROS_ERROR_STREAM("[Vimba System]: Could not start Vimba system: " << errorCodeToMessage(err));
    }
  }

  void setJpegTurboHandlers()
  {
    size_t poolSize =  threadPool_->get_thread_count();
    if (poolSize < 1) poolSize = 1;
    jpegTurboHandlers_.reserve(poolSize);
    for (size_t i = 0; i < poolSize; ++i) {
      jpegTurboHandlers_.push_back(std::make_unique<TurboJpegHandler>());
    }
  }

  TurboJpegHandler* getJpegTurboHandler(size_t thread_index) {
    if (thread_index >= jpegTurboHandlers_.size()) {
      throw std::runtime_error("Thread index out of range");
    }
    return jpegTurboHandlers_[thread_index].get();
  }

  /** Translates Vimba error codes to readable error messages
   *
   * @param error Vimba error type
   * @return readable string error
   *
   **/
  std::string errorCodeToMessage(VmbErrorType error)
  {
    std::map<VmbErrorType, std::string> error_msg;
    error_msg[VmbErrorSuccess] = "Success.";
    error_msg[VmbErrorApiNotStarted] = "API not started.";
    error_msg[VmbErrorNotFound] = "Not found.";
    error_msg[VmbErrorBadHandle] = "Invalid handle ";
    error_msg[VmbErrorDeviceNotOpen] = "Device not open.";
    error_msg[VmbErrorInvalidAccess] = "Invalid access.";
    error_msg[VmbErrorBadParameter] = "Bad parameter.";
    error_msg[VmbErrorStructSize] = "Wrong DLL version.";
    error_msg[VmbErrorWrongType] = "Wrong type.";
    error_msg[VmbErrorInvalidValue] = "Invalid value.";
    error_msg[VmbErrorTimeout] = "Timeout.";
    error_msg[VmbErrorOther] = "TL error.";
    error_msg[VmbErrorInvalidCall] = "Invalid call.";
    error_msg[VmbErrorNoTL] = "TL not loaded.";
    error_msg[VmbErrorNotImplemented] = "Not implemented.";
    error_msg[VmbErrorNotSupported] = "Not supported.";
    error_msg[VmbErrorResources] = "Resource not available.";
    error_msg[VmbErrorInternalFault] = "Unexpected fault in VmbApi or driver.";
    error_msg[VmbErrorMoreData] = "More data returned than memory provided.";

    std::map<VmbErrorType, std::string>::const_iterator iter = error_msg.find(error);
    if (error_msg.end() != iter)
    {
      return iter->second;
    }
    return "Unsupported error code passed.";
  }

  std::string interfaceToString(VmbInterfaceType interfaceType)
  {
    switch (interfaceType)
    {
      case VmbInterfaceFirewire:
        return "FireWire";
        break;
      case VmbInterfaceEthernet:
        return "GigE";
        break;
      case VmbInterfaceUsb:
        return "USB";
        break;
      default:
        return "Unknown";
    }
  }

  std::string accessModeToString(VmbAccessModeType modeType)
  {
    if (modeType & VmbAccessModeFull)
      return "Read and write access";
    else if (modeType & VmbAccessModeRead)
      return "Only read access";
    else if (modeType & VmbAccessModeConfig)
      return "Device configuration access";
    else if (modeType & VmbAccessModeLite)
      return "Device read/write access without feature access (only addresses)";
    else if (modeType & VmbAccessModeNone)
      return "No access";
    else
      return "Undefined access";
  }

  bool frameToImage(const FramePtr vimba_frame_ptr, sensor_msgs::Image& image, sensor_msgs::Image& debugImage, std_msgs::UInt8 &pixel_intensity_msg, int camId, int poolIndex)
  {
      VmbPixelFormatType pixel_format;
      VmbUint32_t width, height, nSize;

      vimba_frame_ptr->GetWidth(width);
      vimba_frame_ptr->GetHeight(height);
      vimba_frame_ptr->GetPixelFormat(pixel_format);
      vimba_frame_ptr->GetImageSize(nSize);

      VmbUint32_t step = nSize/height;

      // NOTE: YUV and ARGB formats not supported
      std::string encoding;
      if (pixel_format==VmbPixelFormatMono8)
          encoding = sensor_msgs::image_encodings::MONO8;
      else if (pixel_format==VmbPixelFormatMono10)
          encoding = sensor_msgs::image_encodings::MONO16;
      else if (pixel_format==VmbPixelFormatMono12)
          encoding = sensor_msgs::image_encodings::MONO16;
      else if (pixel_format==VmbPixelFormatMono12Packed)
          encoding = sensor_msgs::image_encodings::MONO16;
      else if (pixel_format==VmbPixelFormatMono14)
          encoding = sensor_msgs::image_encodings::MONO16;
      else if (pixel_format==VmbPixelFormatMono16)
          encoding = sensor_msgs::image_encodings::MONO16;
      else if (pixel_format==VmbPixelFormatBayerGR8)
          encoding = sensor_msgs::image_encodings::BAYER_GRBG8;
      else if (pixel_format==VmbPixelFormatBayerRG8)
          encoding = sensor_msgs::image_encodings::BAYER_RGGB8;
      else if (pixel_format==VmbPixelFormatBayerGB8)
          encoding = sensor_msgs::image_encodings::BAYER_GBRG8;
      else if (pixel_format==VmbPixelFormatBayerBG8)
          encoding = sensor_msgs::image_encodings::BAYER_BGGR8;
      else if (pixel_format==VmbPixelFormatBayerGR10)
          encoding = sensor_msgs::image_encodings::TYPE_16SC1;
      else if (pixel_format==VmbPixelFormatBayerRG10)
          encoding = sensor_msgs::image_encodings::TYPE_16SC1;
      else if (pixel_format==VmbPixelFormatBayerGB10)
          encoding = sensor_msgs::image_encodings::TYPE_16SC1;
      else if (pixel_format==VmbPixelFormatBayerBG10)
          encoding = sensor_msgs::image_encodings::TYPE_16SC1;
      else if (pixel_format==VmbPixelFormatBayerGR12)
          encoding = sensor_msgs::image_encodings::TYPE_16SC1;
      else if (pixel_format==VmbPixelFormatBayerRG12)
          encoding = sensor_msgs::image_encodings::TYPE_16SC1;
      else if (pixel_format==VmbPixelFormatBayerGB12)
          encoding = sensor_msgs::image_encodings::TYPE_16SC1;
      else if (pixel_format==VmbPixelFormatBayerBG12)
          encoding = sensor_msgs::image_encodings::TYPE_16SC1;
      else if (pixel_format==VmbPixelFormatBayerGR12Packed)
          encoding = sensor_msgs::image_encodings::TYPE_32SC4;
      else if (pixel_format==VmbPixelFormatBayerRG12Packed)
          encoding = sensor_msgs::image_encodings::TYPE_32SC4;
      else if (pixel_format==VmbPixelFormatBayerGB12Packed)
          encoding = sensor_msgs::image_encodings::TYPE_32SC4;
      else if (pixel_format==VmbPixelFormatBayerBG12Packed)
          encoding = sensor_msgs::image_encodings::TYPE_32SC4;
      else if (pixel_format==VmbPixelFormatBayerGR16)
          encoding = sensor_msgs::image_encodings::TYPE_16SC1;
      else if (pixel_format==VmbPixelFormatBayerRG16)
          encoding = sensor_msgs::image_encodings::TYPE_16SC1;
      else if (pixel_format==VmbPixelFormatBayerGB16)
          encoding = sensor_msgs::image_encodings::TYPE_16SC1;
      else if (pixel_format==VmbPixelFormatBayerBG16)
          encoding = sensor_msgs::image_encodings::TYPE_16SC1;
      else if (pixel_format==VmbPixelFormatRgb8)
          encoding = sensor_msgs::image_encodings::RGB8;
      else if (pixel_format==VmbPixelFormatBgr8)
          encoding = sensor_msgs::image_encodings::BGR8;
      else if (pixel_format==VmbPixelFormatRgba8)
          encoding = sensor_msgs::image_encodings::RGBA8;
      else if (pixel_format==VmbPixelFormatBgra8)
          encoding = sensor_msgs::image_encodings::BGRA8;
      else if (pixel_format==VmbPixelFormatRgb12)
          encoding = sensor_msgs::image_encodings::TYPE_16UC3;
      else if (pixel_format==VmbPixelFormatRgb16)
          encoding = sensor_msgs::image_encodings::TYPE_16UC3;
      else
          ROS_WARN("Received frame with unsupported pixel format %d", pixel_format);
      if (encoding=="")
          return false;


      VmbErrorType err;
      bool res = false;

      ros::Time start_time = ros::Time::now();

      if (compressionType_ == CompressionType::Jetraw)
      {
          VmbUchar_t *buffer_ptr_in;
          err = vimba_frame_ptr->GetImage(buffer_ptr_in);
          if (VmbErrorSuccess != err)
          {
              ROS_ERROR_STREAM("[" << ros::this_node::getName() << "]: Could not GetImage. "
                                   << "\n Error: " << errorCodeToMessage(err));
          }
          if (pixel_intensity_)
          {
              try
              {
                  pixel_intensity_msg = calculatePixelIntensity(buffer_ptr_in, nSize, camId);
              }
              catch (std::exception &e)
              {
                  ROS_ERROR("Frame callback intensity error because %s", e.what());
              }
          }
          int32_t dstLen;
          res = jetrawCompress::encodeMsg(buffer_ptr_in,height, width, image, dstLen);
          if (!res)
          {
              ROS_ERROR("JETRAW-------------ENCODING FAILED");
          }

          if (debugImage_)
          {
            bool decodeDebug = jetrawCompress::decodeMsg(image,debugImage);
            if (!decodeDebug)
            {
                ROS_ERROR("JETRAW-------------DEBUG DECODING FAILED");
            }
          }
      }
      else if (compressionType_ == CompressionType::Jpeg)
      {

          if (pixel_intensity_)
          {
              VmbUchar_t *buffer_ptr_in;
              err = vimba_frame_ptr->GetImage(buffer_ptr_in);
              if (VmbErrorSuccess != err)
              {
                  ROS_ERROR_STREAM("[" << ros::this_node::getName() << "]: Could not GetImage. "
                                       << "\n Error: " << errorCodeToMessage(err));
              }
              try
              {
                  pixel_intensity_msg = calculatePixelIntensity(buffer_ptr_in, nSize, camId);
              }
              catch (std::exception &e)
              {
                  ROS_ERROR("Frame callback intensity error because %s", e.what());
              }

          }

          std::vector<VmbUchar_t> TransformedData;
          try
          {
              err = TransformImage(vimba_frame_ptr, TransformedData, "RGB24");
          }
          catch (std::exception &e)
          {
              ROS_ERROR("Frame callback transformingImage error because %s", e.what());
          }
          VmbUchar_t *buffer_ptr_out;
          buffer_ptr_out =reinterpret_cast<VmbUchar_t*>(TransformedData.data());
          encoding = sensor_msgs::image_encodings::RGB8;
          VmbUint32_t step = TransformedData.size() / height;
          res = sensor_msgs::fillImage(image, encoding, height, width, step, buffer_ptr_out);

          //Compressing to JPG
          if (res)
          {
              cv_bridge::CvImagePtr cv_ptr;
              cv_ptr = cv_bridge::toCvCopy(image, sensor_msgs::image_encodings::RGB8);
              std::vector<int> compression_params;
              compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);  // You can use other parameters like PNG compression
              compression_params.push_back(qualityJPG_);  // Set the desired image quality (0-100)
              res = cv::imencode(".jpg", cv_ptr->image, image.data, compression_params);
              image.encoding = "jpg";

              //Decompressing
              if (debugImage_)
              {
                  const cv::Mat_<uchar>
                      in(1, image.data.size(), const_cast<uchar *>(&image.data[0]));
                  const cv::Mat rgb_a = cv::imdecode(in, cv::IMREAD_UNCHANGED);
                  res = sensor_msgs::fillImage(debugImage,  sensor_msgs::image_encodings::RGB8, rgb_a.rows, rgb_a.cols, rgb_a.step, rgb_a.data);
              }
          }

      }
      else if (compressionType_ == CompressionType::JpegTurbo)
      {

        if (pixel_intensity_)
        {
          VmbUchar_t *buffer_ptr_in;
          err = vimba_frame_ptr->GetImage(buffer_ptr_in);
          if (VmbErrorSuccess != err)
          {
            ROS_ERROR_STREAM("[" << ros::this_node::getName() << "]: Could not GetImage. "
                                 << "\n Error: " << errorCodeToMessage(err));
          }
          try
          {
            pixel_intensity_msg = calculatePixelIntensity(buffer_ptr_in, nSize, camId);
          }
          catch (std::exception &e)
          {
            ROS_ERROR("Frame callback intensity error because %s", e.what());
          }

        }

        std::vector<VmbUchar_t> TransformedData;
        try
        {
          err = TransformImage(vimba_frame_ptr, TransformedData, "RGB24");
        }
        catch (std::exception &e)
        {
          ROS_ERROR("Frame callback transformingImage error because %s", e.what());
        }

        encoding = sensor_msgs::image_encodings::RGB8;
        res = compressJpegTurbo(getJpegTurboHandler(poolIndex),TransformedData,image,height,width,encoding,qualityJPG_);

        //Compressing to JPG
        if (debugImage_)
        {
          std::vector<VmbUchar_t> decompressedData;
          res = decompressJpegTurbo(getJpegTurboHandler(poolIndex),image,decompressedData,sensor_msgs::image_encodings::BGR8);
          VmbUint32_t step = decompressedData.size() / height;
          res = sensor_msgs::fillImage(debugImage,  sensor_msgs::image_encodings::BGR8, image.height, image.width, step, decompressedData.data());
        }
      }
      else
      {
          //None
          if (pixel_intensity_)
          {
              VmbUchar_t *buffer_ptr_in;
              err = vimba_frame_ptr->GetImage(buffer_ptr_in);
              if (VmbErrorSuccess != err)
              {
                  ROS_ERROR_STREAM("[" << ros::this_node::getName() << "]: Could not GetImage. "
                                       << "\n Error: " << errorCodeToMessage(err));
              }
              try
              {
                  pixel_intensity_msg = calculatePixelIntensity(buffer_ptr_in, nSize, camId);
              }
              catch (std::exception &e)
              {
                  ROS_ERROR("Frame callback intensity error because %s", e.what());
              }

          }
          VmbUchar_t *buffer_ptr;
          err = vimba_frame_ptr->GetImage(buffer_ptr);
          res = sensor_msgs::fillImage(image,encoding,height,width,step,buffer_ptr);
          if (debugImage_)
          {
              cv_bridge::CvImagePtr cv_ptr;
              cv_ptr = cv_bridge::toCvCopy(image, sensor_msgs::image_encodings::RGB8);
              res = sensor_msgs::fillImage(debugImage,  sensor_msgs::image_encodings::RGB8,cv_ptr->image.rows, cv_ptr->image.cols, cv_ptr->image.step, cv_ptr->image.data);
          }

      }

    if(compressInfo_)
    {
        ros::Duration elapsed_time = ros::Time::now() - start_time;
        size_t compressSize = image.data.size();
        ROS_INFO_STREAM("Time to process image: " << std::to_string(elapsed_time.toSec()) << "s. Compressed from "
        << nSize << " to " << compressSize << " (" << static_cast<float>(nSize)/compressSize << "x times)"<<std::endl);
    }
    return res;
  }

  bool frameToImagePool(const FramePtr vimba_frame_ptr, sensor_msgs::Image& image, sensor_msgs::Image& debugImage, std_msgs::UInt8 &pixel_intensity_msg, int camId)
  {
    if (threadPool_)
    {
      std::future<bool>future = threadPool_->submit_task([this,&vimba_frame_ptr,&image,&debugImage,&pixel_intensity_msg, camId]
                                                          {
                                                            return this->frameToImage(vimba_frame_ptr,image,debugImage,pixel_intensity_msg,camId, *BS::this_thread::get_index());
                                                          }) ;
      return future.get();
    }
    else
    {
      return frameToImage(vimba_frame_ptr,image,debugImage,pixel_intensity_msg,camId,0);
    }
  }


    /**
   * @brief Set Pixel Intensity Calculation Parameters
   *
   * @param pixel_intensity_pixel_steps A pixel will be taken every 'pixel_intensity_pixel_steps' to do the pixel intensity calculation
   * @param pixel_intensity_saturated_threshold The ratio of saturated pixels for which the image is considered saturated
   * @param pixel_intensity_saturation_value The value for which a pixel is considered saturated
   * @param pixel_intensity_count_saturated_pixel_in_mean If true, the saturated pixels will be taken into account in the mean pixel intensity calculation
   * @param pixel_intensity_use_moving_average If true, a moving average filter will be used on the pixel intensities values
   * @param pixel_intensity_moving_average_k Number of values on which you want to do the moving average
   * @param cam_qty The number of cam on the Scanner
   * @param pixel_intensity_echo If true, results of the calculation and computation time will be echo in terminal
   * @param echoed_pixel_intensities List of cameras id for which we want to echo values
   */
    void setPixelIntensityParameters(int pixel_intensity_pixel_steps, double pixel_intensity_saturated_threshold, int pixel_intensity_saturation_value, bool pixel_intensity_count_saturated_pixels_in_mean, bool pixel_intensity_use_moving_average, int pixel_intensity_moving_average_k, int cam_qty, bool pixel_intensity_echo, std::vector<int> echoed_pixel_intensities)
    {
        pixel_intensity_pixel_steps_ = pixel_intensity_pixel_steps;
        pixel_intensity_saturated_threshold_ = pixel_intensity_saturated_threshold;
        pixel_intensity_saturation_value_ = pixel_intensity_saturation_value;
        pixel_intensity_count_saturated_pixels_in_mean_ = pixel_intensity_count_saturated_pixels_in_mean;
        pixel_intensity_use_moving_average_ = pixel_intensity_use_moving_average;
        pixel_intensity_moving_average_k_ = pixel_intensity_moving_average_k;
        cam_qty_ = cam_qty;
        vector_of_values_per_cam_for_moving_average.resize(cam_qty_);
        pixel_intensity_echo_ = pixel_intensity_echo;
        echoed_pixel_intensities_ = echoed_pixel_intensities;
        pixel_intensity_ = true ;
    }

    void activateDebugImage()
    {
        debugImage_ = true;
    }


private:

  void listAvailableCameras()
  {
    ROS_INFO("Searching for cameras ...");
    CameraPtrVector cameras;
    if (VmbErrorSuccess == vs.Startup())
    {
      if (VmbErrorSuccess == vs.GetCameras(cameras))
      {
        for (const auto& camera : cameras)
        {
          std::string strID;
          std::string strName;
          std::string strModelname;
          std::string strSerialNumber;
          std::string strInterfaceID;
          VmbInterfaceType interfaceType;
          VmbAccessModeType accessType;

          VmbErrorType err = camera->GetID(strID);
          if (VmbErrorSuccess != err)
          {
            ROS_ERROR_STREAM("[Could not get camera ID. Error code: " << err << "]");
          }

          err = camera->GetName(strName);
          if (VmbErrorSuccess != err)
          {
            ROS_ERROR_STREAM("[Could not get camera name. Error code: " << err << "]");
          }

          err = camera->GetModel(strModelname);
          if (VmbErrorSuccess != err)
          {
            ROS_ERROR_STREAM("[Could not get camera mode name. Error code: " << err << "]");
          }

          err = camera->GetSerialNumber(strSerialNumber);
          if (VmbErrorSuccess != err)
          {
            ROS_ERROR_STREAM("[Could not get camera serial number. Error code: " << err << "]");
          }

          err = camera->GetInterfaceID(strInterfaceID);
          if (VmbErrorSuccess != err)
          {
            ROS_ERROR_STREAM("[Could not get interface ID. Error code: " << err << "]");
          }

          err = camera->GetInterfaceType(interfaceType);
          if (VmbErrorSuccess != err)
          {
            ROS_ERROR_STREAM("[Could not get interface type. Error code: " << err << "]");
          }

          err = camera->GetPermittedAccess(accessType);
          if (VmbErrorSuccess != err)
          {
            ROS_ERROR_STREAM("[Could not get access type. Error code: " << err << "]");
          }

          ROS_INFO_STREAM("Found camera named " << strName << ":");
          ROS_INFO_STREAM(" - Model Name     : " << strModelname);
          ROS_INFO_STREAM(" - Camera ID      : " << strID);
          ROS_INFO_STREAM(" - Serial Number  : " << strSerialNumber);
          ROS_INFO_STREAM(" - Interface ID   : " << strInterfaceID);
          ROS_INFO_STREAM(" - Interface type : " << interfaceToString(interfaceType));
          ROS_INFO_STREAM(" - Access type    : " << accessModeToString(accessType));
        }
      }
      else
      {
        ROS_WARN("Could not get cameras from Vimba System");
      }
    }
    else
    {
      ROS_WARN("Could not start Vimba System");
    }
  }



    VmbErrorType TransformImage( const FramePtr & SourceFrame, std::vector<VmbUchar_t> & DestinationData, const std::string &DestinationFormat )
    {
        if( SP_ISNULL( SourceFrame) )
        {
            return VmbErrorBadParameter;
        }
        VmbErrorType        Result;
        VmbPixelFormatType  InputFormat;
        VmbUint32_t         InputWidth,InputHeight;
        Result = SP_ACCESS( SourceFrame )->GetPixelFormat( InputFormat ) ;
        if( VmbErrorSuccess != Result )
        {
            return Result;
        }
        Result = SP_ACCESS( SourceFrame )->GetWidth( InputWidth );
        if( VmbErrorSuccess != Result )
        {
            return Result;
        }
        Result = SP_ACCESS( SourceFrame )->GetHeight( InputHeight );
        if( VmbErrorSuccess != Result )
        {
            return Result;
        }
        // Prepare source image
        VmbImage SourceImage;
        SourceImage.Size = sizeof( SourceImage );
        Result = static_cast<VmbErrorType>( VmbSetImageInfoFromPixelFormat( InputFormat, InputWidth, InputHeight, &SourceImage ));
        if( VmbErrorSuccess != Result )
        {
            return Result;
        }
        VmbUchar_t *DataBegin = NULL;
        Result = SP_ACCESS( SourceFrame )->GetBuffer( DataBegin );
        if( VmbErrorSuccess != Result )
        {
            return Result;
        }
        SourceImage.Data = DataBegin;
        // Prepare destination image
        VmbImage DestinationImage;
        DestinationImage.Size = sizeof( DestinationImage );
        Result = static_cast<VmbErrorType>( VmbSetImageInfoFromString( DestinationFormat.c_str(), static_cast<VmbUint32_t>(DestinationFormat.size()), InputWidth, InputHeight, &DestinationImage) );
        if ( VmbErrorSuccess != Result )
        {
            return Result;
        }
        const size_t ByteCount = ( DestinationImage.ImageInfo.PixelInfo.BitsPerPixel * InputWidth* InputHeight ) / 8 ;
        DestinationData.resize( ByteCount );
        DestinationImage.Data = &*DestinationData.begin();
        // Transform data
        Result = static_cast<VmbErrorType>( VmbImageTransform( &SourceImage, &DestinationImage, NULL , 0 ));
        return Result;
    }

    std_msgs::UInt8 calculatePixelIntensity(VmbUchar_t* data, VmbUint32_t size, int camId)
    {
        ros::Time start_time = ros::Time::now();
        int nb_pixels = 0;
        int nb_saturated_pixels = 0;
        int nb_non_saturated_pixels = 0;
        int sum_values_to_calculate_mean = 0;
        VmbUchar_t pixelSaturation = (VmbUchar_t)pixel_intensity_saturation_value_;

        // Parse Image Buffer
        for(int i = 0 ; i < size ; i += pixel_intensity_pixel_steps_)
        {
            // Saturated Pixel
            if(data[i] >= pixelSaturation)
            {
                if(pixel_intensity_count_saturated_pixels_in_mean_)
                {
                    sum_values_to_calculate_mean += data[i];
                }
                nb_saturated_pixels++;
            }
            // Non Saturated Pixel
            else
            {
                sum_values_to_calculate_mean += data[i];
                nb_non_saturated_pixels++;
            }
            nb_pixels++;
        }

        // If too many pixels are saturated, the value '255' is published so the lights_intensities node know it has to quickly reduce the light intensity
        // If pixel_intensity_saturated_threshold_ < 0.0, it means we don't use this threshold
        VmbUint8_t pixel_intensity_measured;
        float ratio_saturated_pixels = (float)nb_saturated_pixels / (float)nb_pixels;
        if(pixel_intensity_saturated_threshold_ > 0.0 && ratio_saturated_pixels > pixel_intensity_saturated_threshold_)
        {
            pixel_intensity_measured = 255;
        }
        // Else, we calculate the mean intensity value
        else
        {
            if(pixel_intensity_count_saturated_pixels_in_mean_)
            {
                pixel_intensity_measured = sum_values_to_calculate_mean / nb_pixels;
            }
            else
            {
                pixel_intensity_measured = sum_values_to_calculate_mean / nb_non_saturated_pixels;
            }
        }

        // Message definition
        std_msgs::UInt8 pixel_intensity_msg;

        // Moving Average
        if(pixel_intensity_use_moving_average_)
        {
            // Manage vector of intensities
            if(vector_of_values_per_cam_for_moving_average[camId].size() < pixel_intensity_moving_average_k_)
            {
                vector_of_values_per_cam_for_moving_average[camId].push_back(pixel_intensity_measured);
            }
            else
            {
                vector_of_values_per_cam_for_moving_average[camId].erase(vector_of_values_per_cam_for_moving_average[camId].begin());
                vector_of_values_per_cam_for_moving_average[camId].push_back(pixel_intensity_measured);
            }

            // Mean
            int sum_pixel_intensities = 0;
            for(int i = 0 ; i < vector_of_values_per_cam_for_moving_average[camId].size() ; i++)
            {
                sum_pixel_intensities += vector_of_values_per_cam_for_moving_average[camId][i];
            }
            pixel_intensity_msg.data = (VmbUint8_t)(sum_pixel_intensities / vector_of_values_per_cam_for_moving_average[camId].size());
        }
        else
        {
            pixel_intensity_msg.data = pixel_intensity_measured;
        }

        // Echos
        if(pixel_intensity_echo_ && std::count(echoed_pixel_intensities_.begin(), echoed_pixel_intensities_.end(), camId))
        {
            ros::Duration elapsed_time = ros::Time::now() - start_time;
            ROS_INFO_STREAM("camId: " << std::to_string(camId) << std::endl <<
                                          " - Elapsed Time: " << std::to_string(elapsed_time.toSec()) << "s" << std::endl <<
                                          " - nb_measured_pixels: " << std::to_string(nb_pixels) << std::endl <<
                                          " - ratio_saturated_pixels: " << std::to_string(ratio_saturated_pixels) << std::endl <<
                                          " - pixel_intensity_measured: " << std::to_string(pixel_intensity_measured) << std::endl <<
                                          " - pixel_intensity_msg.data: " << std::to_string(pixel_intensity_msg.data) << std::endl <<
                                          "--------------------------------------");
        }

        return pixel_intensity_msg;
    }

};
}  // namespace avt_vimba_camera
#endif
