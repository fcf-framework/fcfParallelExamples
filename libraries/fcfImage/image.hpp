#ifndef ___FCF__IMAGE__IMAGE_HPP___
#define ___FCF__IMAGE__IMAGE_HPP___

#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <stdexcept>

#include "macro.hpp"

namespace fcf {
  namespace Image {

    void loadRGBFromBmpIFStream(std::istream& a_istream, std::vector<char>& a_buffer, size_t& a_dstWidth, size_t& a_dstHeight);

    #ifdef FCF_IMAGE_IMPLEMENTATION
      void loadRGBFromBmpIFStream(std::istream& a_istream, std::vector<char>& a_buffer, size_t& a_dstWidth, size_t& a_dstHeight) {
        a_istream.seekg(0, std::ios::end);
        size_t fileSize = a_istream.tellg();
        a_istream.seekg(0, std::ios::beg);
        std::vector<char> sourceData(fileSize);
        a_istream.read(&sourceData[0], fileSize);

        size_t bmpDataOffsetOffset    = 10;
        size_t bmpHeaderInfoOffset    = 14;
        size_t bmpHeaderInfoOffsetEnd = bmpHeaderInfoOffset + 40;
        size_t bmpWidthOffset = bmpHeaderInfoOffset + 4;
        size_t bmpHeightOffset = bmpHeaderInfoOffset + 8;
        size_t bmpBitCoutOffset = bmpHeaderInfoOffset + 14;
        size_t bmpCompressionOffset = bmpHeaderInfoOffset + 16;
        size_t bmpImageSizeOffset = bmpHeaderInfoOffset + 20;

        if (fileSize < bmpHeaderInfoOffsetEnd) {
          throw std::runtime_error("Invalid BMP format");
        }

        size_t dataOffset = *(unsigned int*)&sourceData[bmpDataOffsetOffset];
        if (dataOffset > fileSize) {
          throw std::runtime_error("Invalid BMP format");
        }
        size_t dataSize   = std::min((unsigned int)(fileSize - dataOffset), *(unsigned int*)&sourceData[bmpImageSizeOffset]);
        size_t imageWidth = *(unsigned int*)&sourceData[bmpWidthOffset];
        size_t imageHeight = *(unsigned int*)&sourceData[bmpHeightOffset];
        int    bitCount = *(unsigned short*)&sourceData[bmpBitCoutOffset];
        int    compression = *(unsigned int*)&sourceData[bmpCompressionOffset];

        if (compression != 0 && compression != 3){
          throw std::runtime_error("Application only accepts BMP RGB(24bit) or RGBA(32bit) formats");
        }
        if (bitCount != 32 && bitCount != 24){
          throw std::runtime_error("Application only accepts BMP RGB(24bit) or RGBA(32bit) formats");
        }
        if ((imageWidth * imageHeight * (bitCount / 8)) != dataSize) {
          throw std::runtime_error("Invalid BMP format");
        }
        int   step    = bitCount == 32 ? 4 : 3;
        char* data = &sourceData[dataOffset];
        a_buffer.resize(imageWidth * imageHeight * 3);
        size_t rgbIndex = 0;
        for(unsigned int index = 0; index < dataSize; index += step){
          a_buffer[rgbIndex]  = data[index];
          a_buffer[rgbIndex+1] = data[index+1];
          a_buffer[rgbIndex+2] = data[index+2];
          rgbIndex += 3;
        }
        a_dstWidth = imageWidth;
        a_dstHeight = imageHeight;
      }
    #endif

    template <typename TFilePath, typename TBuffer>
    void loadRGBFromBmpFile(TFilePath a_filePath, TBuffer& a_buffer, size_t& a_dstWidth, size_t& a_dstHeight){
      std::ifstream ifs;
      ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      ifs.open(a_filePath, std::ifstream::binary);
      ifs.exceptions(std::ifstream::badbit);
      loadRGBFromBmpIFStream(ifs, a_buffer, a_dstWidth, a_dstHeight);
    }


    void writeRGBToBmpOStream(std::ostream& a_ostream, const std::vector<char>& a_buffer, size_t a_width, size_t a_height);

    #ifdef FCF_IMAGE_IMPLEMENTATION
      void writeRGBToBmpOStream(std::ostream& a_ostream, const std::vector<char>& a_buffer, size_t a_width, size_t a_height){
        unsigned int dataOffset = 14 + 40;
        unsigned int imageSize = a_width * a_height * 3;
        unsigned int fileSize = dataOffset + imageSize;

        if (a_buffer.size() != imageSize){
          throw std::runtime_error("Buffer size does not mutch image size");
        }

        a_ostream.write("BM", 2);
        a_ostream.write((char*)&fileSize, 4);
        unsigned int reservedField = 0;
        a_ostream.write((char*)&reservedField, 4);
        a_ostream.write((char*)&dataOffset, 4);

        unsigned int headerInfoSize = 40;
        a_ostream.write((char*)&headerInfoSize, 4);
        unsigned int width = a_width;
        a_ostream.write((char*)&width, 4);
        unsigned int height = a_height;
        a_ostream.write((char*)&height, 4);
        unsigned short planes = 1;
        a_ostream.write((char*)&planes, sizeof(planes));
        unsigned short bitCount = 24;
        a_ostream.write((char*)&bitCount, sizeof(bitCount));
        unsigned int compression = 0;
        a_ostream.write((char*)&compression, sizeof(compression));
        a_ostream.write((char*)&imageSize, sizeof(imageSize));

        unsigned int xperm = 11811;
        a_ostream.write((char*)&xperm, sizeof(xperm));
        unsigned int yperm = 11811;
        a_ostream.write((char*)&yperm, sizeof(yperm));
        unsigned int colorsUsed = 0;
        a_ostream.write((char*)&colorsUsed, sizeof(colorsUsed));
        unsigned int colorsImportant = 0;
        a_ostream.write((char*)&colorsImportant, sizeof(colorsImportant));

        a_ostream.write(&a_buffer[0], a_buffer.size());
      }
    #endif

    template <typename TFilePath, typename TBuffer>
    void writeRGBToBmpFile(TFilePath a_filePath, TBuffer& a_buffer, size_t a_width, size_t a_height){
      std::ofstream ofs;
      ofs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      ofs.open(a_filePath, std::ifstream::binary);
      ofs.exceptions(std::ifstream::badbit);
      writeRGBToBmpOStream(ofs, a_buffer, a_width, a_height);
    }



  }
}

#endif // #ifndef ___FCF__IMAGE__IMAGE_HPP___
