#pragma once
#include <turbojpeg.h>
#include <sensor_msgs/Image.h>
#include <vector>

struct TurboJpegHandler {
  tjhandle compressor;
  tjhandle decompressor;

  TurboJpegHandler() : compressor(nullptr), decompressor(nullptr) {
    compressor = tjInitCompress();
    if (!compressor) {
      throw std::runtime_error("Failed to initialize TurboJPEG compressor");
    }

    decompressor = tjInitDecompress();
    if (!decompressor) {
      tjDestroy(compressor);
      throw std::runtime_error("Failed to initialize TurboJPEG decompressor");
    }
  }

  ~TurboJpegHandler() {
    if (compressor) tjDestroy(compressor);
    if (decompressor) tjDestroy(decompressor);
  }
};
bool compressJpegTurbo(TurboJpegHandler *handler,
                       const std::vector<unsigned char>& imageIn,
                       sensor_msgs::Image& imageOut,
                       int height,
                       int width,
                       const std::string& encoding,
                       int quality = 75);

bool decompressJpegTurbo(TurboJpegHandler *handler,
                         const sensor_msgs::Image& imageIn,
                         std::vector<unsigned char>& imageOut,
                         const std::string& desired_encoding = "rgb8");
