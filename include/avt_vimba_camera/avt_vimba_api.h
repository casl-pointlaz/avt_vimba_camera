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

using AVT::VmbAPI::CameraPtr;
using AVT::VmbAPI::FramePtr;
using AVT::VmbAPI::VimbaSystem;

namespace avt_vimba_camera
{
class AvtVimbaApi
{
public:
  AvtVimbaApi() : vs(VimbaSystem::GetInstance())
  {
  }

    VimbaSystem& vs;      // Modified by pointlaz. vs is now public

    // Pixel Intensity Calculation
    int pixel_intensity_pixel_steps_;               // A pixel will be taken every 'pixel_intensity_pixel_steps' to do the pixel intensity calculation
    double pixel_intensity_saturated_threshold_;    // The ratio of saturated pixels for which the image is considered saturated
    int pixel_intensity_saturation_value_;          // The value for which a pixel is considered saturated
    bool pixel_intensity_echo_;
    bool pixel_intensity_ = false ;
    bool compressJetraw_ = false;
    bool compressJPG_ = false;
    int qualityJPG_ = 90;

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

  bool frameToImage(const FramePtr vimba_frame_ptr, sensor_msgs::Image& image, std_msgs::UInt8 &pixel_intensity_msg)
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
      if (compressJetraw_)
      {
          ros::Time start_time = ros::Time::now();
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
                  pixel_intensity_msg = calculatePixelIntensity(buffer_ptr_in, nSize);
              }
              catch (std::exception &e)
              {
                  ROS_ERROR("Frame callback intensity error because %s", e.what());
              }
          }
          VmbUchar_t *buffer_ptr_out;
          int32_t dstLen;
          ros::Time start_time_compress = ros::Time::now();
          if (!jetrawCompress::encode(buffer_ptr_in,height, width,buffer_ptr_out, dstLen))
          {
              ROS_ERROR("JETRAW-------------ENCODING FAILED");
          }
          ros::Duration elapsed_time = ros::Time::now() - start_time_compress;
          ROS_INFO_STREAM("Elapsed Time for compressing jetraw : " << std::to_string(elapsed_time.toSec()) << "s" << std::endl);


          encoding = "jetraw";
          VmbUint32_t step = dstLen;
          res = sensor_msgs::fillImage(image, encoding, height, width, dstLen, buffer_ptr_out);
          elapsed_time = ros::Time::now() - start_time;
          ROS_INFO_STREAM("Elapsed Time for jetraw msg process: " << std::to_string(elapsed_time.toSec()) << "s" << std::endl);

      }
      else if (compressJPG_)
      {
          ros::Time start_time = ros::Time::now();
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
                  pixel_intensity_msg = calculatePixelIntensity(buffer_ptr_in, nSize);
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
          ros::Time start_time_compress = ros::Time::now();
          if (res)
          {
              cv_bridge::CvImagePtr cv_ptr;
              cv_ptr = cv_bridge::toCvCopy(image, sensor_msgs::image_encodings::RGB8);
              std::vector<int> compression_params;
              compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);  // You can use other parameters like PNG compression
              compression_params.push_back(qualityJPG_);  // Set the desired image quality (0-100)
              res = cv::imencode(".jpg", cv_ptr->image, image.data, compression_params);
              image.encoding = "jpg";
          }
          ros::Duration elapsed_time = ros::Time::now() - start_time_compress;
          ROS_INFO_STREAM("Elapsed Time for compressing jetraw : " << std::to_string(elapsed_time.toSec()) << "s" << std::endl);

          elapsed_time = ros::Time::now() - start_time;
          ROS_INFO_STREAM("Elapsed Time for jpg msg process: " << std::to_string(elapsed_time.toSec()) << "s" << std::endl);

      }
      else
      {
          //Other Transform
          ros::Time start_time = ros::Time::now();
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
                  pixel_intensity_msg = calculatePixelIntensity(buffer_ptr_in, nSize);
              }
              catch (std::exception &e)
              {
                  ROS_ERROR("Frame callback intensity error because %s", e.what());
              }

          }

          VmbUchar_t *buffer_ptr;
          VmbErrorType err = vimba_frame_ptr->GetImage(buffer_ptr);
          res = sensor_msgs::fillImage(image,encoding,height,width,step,buffer_ptr);
          ros::Duration elapsed_time = ros::Time::now() - start_time;
          ROS_INFO_STREAM("Elapsed Time for bayer msg process: " << std::to_string(elapsed_time.toSec()) << "s" << std::endl);
      }
    return res;
  }

    /**
   * @brief Set Pixel Intensity Calculation Parameters
   *
   * @param pixel_intensity_pixel_steps A pixel will be taken every 'pixel_intensity_pixel_steps' to do the pixel intensity calculation
   * @param pixel_intensity_saturated_threshold The ratio of saturated pixels for which the image is considered saturated
   * @param pixel_intensity_saturation_value The value for which a pixel is considered saturated
   * @param pixel_intensity_echo If true, results of the calculation and computation time will be echo in terminal
   */
    void setPixelIntensityParameters(int pixel_intensity_pixel_steps, double pixel_intensity_saturated_threshold, int pixel_intensity_saturation_value, bool pixel_intensity_echo)
    {
        pixel_intensity_pixel_steps_ = pixel_intensity_pixel_steps;
        pixel_intensity_saturated_threshold_ = pixel_intensity_saturated_threshold;
        pixel_intensity_saturation_value_ = pixel_intensity_saturation_value;
        pixel_intensity_echo_ = pixel_intensity_echo;
        pixel_intensity_ = true ;
    }

    void activateJetraw(){
        compressJetraw_ = true;
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

    std_msgs::UInt8 calculatePixelIntensity(VmbUchar_t* data, VmbUint32_t size)
    {

        ros::Time start_time = ros::Time::now();
        int nb_pixels = 0;
        int nb_saturated_pixels = 0;
        int nb_non_saturated_pixels = 0;
        int sum_values_non_saturated_pixels = 0;
        VmbUchar_t pixelSaturation = (VmbUchar_t)pixel_intensity_saturation_value_;

        // Parse Image Buffer
        for(int i = 0 ; i < size ; i += pixel_intensity_pixel_steps_)
        {
            // Saturated Pixel
            if(data[i] >= pixelSaturation)
            {
                nb_saturated_pixels++;
            }
                // Non Saturated Pixel
            else
            {
                sum_values_non_saturated_pixels += data[i];
                nb_non_saturated_pixels++;
            }
            nb_pixels++;
        }

        // Message definition
        std_msgs::UInt8 pixel_intensity_msg;

        // If too many pixels are saturated, the value '255' is published so the lights_intensities node know it has to quickly reduce the light intensity
        float ratio_saturated_pixels = (float)nb_saturated_pixels / (float)nb_pixels;
        if(ratio_saturated_pixels > pixel_intensity_saturated_threshold_)
        {
            pixel_intensity_msg.data = 255;
        }
            // Else, we calculate the mean intensity value from the non-saturated pixels
        else
        {
            pixel_intensity_msg.data = sum_values_non_saturated_pixels / nb_non_saturated_pixels;
        }

        // Echos
        if(pixel_intensity_echo_)
        {
            ROS_INFO_STREAM("--------------------------------------------------");
            ROS_INFO_STREAM("nb_pixels: " << nb_pixels << " ; ratio_saturated_pixels: " << ratio_saturated_pixels << " ; pixel_intensity_msg.data: " << std::to_string(pixel_intensity_msg.data));
            ros::Duration elapsed_time = ros::Time::now() - start_time;
            ROS_INFO_STREAM("Elapsed Time: " << std::to_string(elapsed_time.toSec()) << "s" << std::endl);
        }

        return pixel_intensity_msg;
    }

};
}  // namespace avt_vimba_camera
#endif
