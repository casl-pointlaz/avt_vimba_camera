#ifndef RAILCAM_TESTSUTILS_IMAGE_H
#define RAILCAM_TESTSUTILS_IMAGE_H
#include <Eigen/Dense>
#include <fstream>
#include <type_traits>

namespace railcam::testsutils {

    // Write grayscale PGM image
    template<typename Derived>
    requires (Derived::IsRowMajor == true) && (std::is_same_v<typename Derived::Scalar, uint8_t>)
    inline void writePGM(const std::string& filename, const Eigen::ArrayBase<Derived>& image) {
        std::ofstream file(filename, std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("Error: Could not open file " + filename);
        }

        int maxGrayValue = 255;

        // Write the PGM file header
        file << "P5\n" << image.cols() << " " << image.rows() << "\n" << maxGrayValue << "\n";

        // Write the pixel data
        file.write(reinterpret_cast<const char*>(image.eval().data()), image.size());

        if (!file) {
            throw std::runtime_error("Error: Failed to write pixel data");
        }

        file.close();
    }

    template<typename Derived>
    requires (Derived::IsRowMajor == false) && (std::is_same_v<typename Derived::Scalar, uint8_t>)
    inline void writePGM(const std::string& filename, const Eigen::ArrayBase<Derived>& colmajorimage) {
        Eigen::Array<typename Derived::Scalar, Derived::RowsAtCompileTime, Derived::ColsAtCompileTime, Eigen::RowMajor>
                romajorimage = colmajorimage;
        return writePGM(filename, romajorimage);
    }

    // Reads a valid integer from the file, ignoring comments and whitespace
    inline int readNextInt(std::ifstream& file) {
        std::string token;
        while (file >> token) {
            if (token[0] == '#') {
                file.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Skip the comment line
            } else {
                try {
                    return std::stoi(token);
                } catch (const std::exception&) {
                    throw std::runtime_error("Error: Invalid integer in PGM header.");
                }
            }
        }
        throw std::runtime_error("Error: Unexpected end of file while reading PGM header.");
    }

    // Reads the PGM header and extracts format, dimensions, and max gray value
    inline void readHeader(std::ifstream& file, std::string& magic, int& width, int& height, int& maxGrayValue) {
        file >> magic;
        if (magic != "P5" && magic != "P2") {
            throw std::runtime_error("Error: Unsupported PGM format. Expected P5 or P2.");
        }

        width = readNextInt(file);
        height = readNextInt(file);
        maxGrayValue = readNextInt(file);

        if (maxGrayValue <= 0 || maxGrayValue > std::numeric_limits<uint16_t>::max()) {
            throw std::runtime_error("Error: Invalid maxGrayValue in PGM, Got '" + std::to_string(maxGrayValue) + "'.");
        }

        file.ignore(1); // Consume the newline character after maxGrayValue
    }

    // Reads binary (P5) pixel data and scales it to uint8_t
    template<typename T>
    inline void readBinaryData(std::ifstream& file, Eigen::Array<uint8_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& image, int maxGrayValue) {
        Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> rawImage(image.rows(), image.cols());
        file.read(reinterpret_cast<char*>(rawImage.data()), rawImage.size() * sizeof(T));
        if (!file) {
            throw std::runtime_error("Error: Failed to read binary pixel data.");
        }

        // Normalize and convert to uint8_t
        image = (rawImage.template cast<float>() / maxGrayValue * 255.0f).round().template cast<uint8_t>();
    }

    // Reads ASCII (P2) pixel data and scales it to uint8_t
    inline void readAsciiData(std::ifstream& file, Eigen::Array<uint8_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& image, int maxGrayValue) {
        for (int i = 0; i < image.rows(); ++i) {
            for (int j = 0; j < image.cols(); ++j) {
                int pixelValue;
                file >> pixelValue;
                if (pixelValue < 0 || pixelValue > maxGrayValue) {
                    throw std::runtime_error("Error: Invalid pixel value in ASCII PGM.");
                }
                image(i, j) = static_cast<uint8_t>(std::round(pixelValue * 255.0f / maxGrayValue));
            }
        }
    }

    // Main function to read a PGM file and return uint8_t image
    inline Eigen::Array<uint8_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
    readPGM(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Error: Could not open file " + filename);
        }

        std::string magic;
        int width, height, maxGrayValue;
        readHeader(file, magic, width, height, maxGrayValue);

        // Allocate space for the uint8_t image
        Eigen::Array<uint8_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> image(height, width);

        if (magic == "P5") {
            // Determine appropriate type based on maxGrayValue
            if (maxGrayValue <= 255) {
                readBinaryData<uint8_t>(file, image, maxGrayValue);
            } else {
                readBinaryData<uint16_t>(file, image, maxGrayValue);
            }
        } else {
            readAsciiData(file, image, maxGrayValue);
        }

        return image;
    }

    template<typename Derived>
    requires (Derived::IsRowMajor == true) && (std::is_same_v<typename Derived::Scalar, uint8_t>)
    inline void writePPM(const std::string& filename, const Eigen::ArrayBase<Derived>& image) {
        if (image.cols() % 3 != 0) {
            throw std::invalid_argument("Image array should have a width that is a multiple of 3 (for RGB channels).");
        }

        int height = image.rows();
        int width = image.cols() / 3;  // Since each pixel has 3 channels (RGB)

        std::ofstream outFile(filename, std::ios::binary);
        if (!outFile) {
            throw std::runtime_error("Cannot open file for writing.");
        }

        // PPM Header
        outFile << "P6\n" << width << " " << height << "\n255\n";
        outFile.write(reinterpret_cast<const char*>(image.eval().data()), image.size());
        outFile.close();
    }

    template<typename Derived>
    requires (Derived::IsRowMajor == true) && (std::is_same_v<typename Derived::Scalar, uint8_t>)
    inline auto grayscaleToRGB(const Eigen::ArrayBase<Derived>& grayscale) {
        const auto height = grayscale.rows();
        const auto width = grayscale.cols();

        Eigen::Array<uint8_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> rgbImage(height, width * 3);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                uint8_t pixel = grayscale(y, x);
                rgbImage.block<1,3>(y, 3*x).setConstant(pixel);
            }
        }
        return rgbImage;
    }

    // Draw red pixels on an RGB image given an array of (x, y) coordinates
    template<typename Derived>
    requires (Derived::IsRowMajor == true) && (std::is_same_v<typename Derived::Scalar, uint8_t>)
    inline void drawRedPixels(Eigen::ArrayBase<Derived>& rgbImage,
                              const Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& lineScanData,
                              float alpha = 0.5f) {
        const auto width = rgbImage.cols() / 3;
        const Eigen::Array<float, 1, 3> red{255.0, 0, 0};

        for (int x = 0; x < width; ++x) {
            for (int i = 0; i < lineScanData.rows(); ++i) {
                int y = static_cast<int>(std::round(lineScanData(i, x))); // Round to nearest pixel
                if (y >= 0 && y < width) {
                    int baseIdx = 3 * x;
                    rgbImage.template block<1, 3>(y, baseIdx) = (
                            alpha * red +
                            (1 - alpha) * rgbImage.template block<1, 3>(y, baseIdx).template cast<float>()
                            ).template cast<uint8_t>();
                }
            }
        }
    }

} //namespace railcam::testsutils
#endif //RAILCAM_TESTSUTILS_IMAGE_H
