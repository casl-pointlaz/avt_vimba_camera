#include "avt_vimba_camera/jpegTurbo.h"

bool compressJpegTurbo(TurboJpegHandler* handler,
              const std::vector<unsigned char>& imageIn,
              sensor_msgs::Image& imageOut,
              int height,
              int width,
              const std::string& encoding,
              int quality) {
  if (imageIn.empty() || height <= 0 || width <= 0) {
    return false;
  }


  try {
    unsigned char* jpeg_buffer = nullptr;
    unsigned long jpeg_size = 0;

    TJPF pixel_format = (encoding == "rgb8") ? TJPF_RGB :
                        (encoding == "bgr8") ? TJPF_BGR : TJPF_UNKNOWN;

    if (pixel_format == TJPF_UNKNOWN) return false;

    int pitch = width * 3;
    int result = tjCompress2(
        handler->compressor,
        imageIn.data(),
        width,
        pitch,
        height,
        pixel_format,
        &jpeg_buffer,
        &jpeg_size,
        TJSAMP_420,
        quality,
        TJFLAG_FASTDCT
    );

    if (result != 0) {
      if (jpeg_buffer) tjFree(jpeg_buffer);
      return false;
    }

    imageOut.height = height;
    imageOut.width = width;
    imageOut.encoding = "jpegTurbo";
    imageOut.is_bigendian = false;
    imageOut.step = jpeg_size / height;
    imageOut.data.resize(jpeg_size);
    std::copy(jpeg_buffer, jpeg_buffer + jpeg_size, imageOut.data.begin());

    tjFree(jpeg_buffer);
    return true;

  } catch (const std::exception&) {
    return false;
  }
}

bool decompressJpegTurbo(TurboJpegHandler* handler,
                const sensor_msgs::Image& imageIn,
                std::vector<unsigned char>& imageOut,
                const std::string& desired_encoding) {
  if (imageIn.encoding != "jpegTurbo" || imageIn.data.empty()) {
    return false;
  }
  try {
    int width, height, subsamp, colorspace;

    int result = tjDecompressHeader3(
        handler->decompressor,
        imageIn.data.data(),
        imageIn.data.size(),
        &width,
        &height,
        &subsamp,
        &colorspace
    );

    if (result != 0) return false;

    TJPF pixel_format = (desired_encoding == "rgb8") ? TJPF_RGB :
                        (desired_encoding == "bgr8") ? TJPF_BGR : TJPF_UNKNOWN;

    if (pixel_format == TJPF_UNKNOWN) return false;

    imageOut.resize(width * height * 3);

    result = tjDecompress2(
        handler->decompressor,
        imageIn.data.data(),
        imageIn.data.size(),
        imageOut.data(),
        width,
        0,
        height,
        pixel_format,
        TJFLAG_FASTDCT
    );

    return (result == 0);

  } catch (const std::exception&) {
    return false;
  }
}