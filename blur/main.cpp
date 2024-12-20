#include <fcfImage/image.hpp>
#include <fcfParallel/parallel.hpp>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <vector>
#include <stdexcept>
void printHelp(){
  std::cout << "An example application illustrating the use of fcfParallel" << std::endl;
  std::cout << "  Application launch format: parallel-example-001-blur SOURCE_BMP_FILE OUTPUT_BMP_FILE" << std::endl;
  std::cout << "  Options:" << std::endl;
  std::cout << "    SOURCE_BMP_FILE - Source BMP file." << std::endl;
  std::cout << "    OUTPUT_BMP_FILE - Resulting BMP file with the blur effect applied." << std::endl;
}

FCF_PARALLEL_UNIT(
    blur,
    void FCF_PARALLEL_MAIN(const FCFParallelTask* a_task,
                           int a_blur,
                           int a_width,
                           int a_height,
                           FCF_PARALLEL_GLOBAL const char* a_source,
                           FCF_PARALLEL_GLOBAL char* a_result) {
      int offset = a_task->lowIndex * 3;
      int y = a_task->lowIndex / a_width;
      int x = a_task->lowIndex % a_width;
      int begby = max(y - a_blur, 0);
      int endby = min(y + a_blur + 1, a_height);
      int begbx = max(x - a_blur, 0);
      int endbx = min(x + a_blur + 1, a_width);
      int c     = (endby - begby) * (endbx - begbx);

      for(int channel = 0; channel < 3; ++channel) {
        int value = 0;
        for(int by = begby; by < endby; ++by) {
          for(int bx = begbx; bx < endbx; ++bx) {
            int bRawIndex = (by * a_width + bx) * 3;
            value += (int)(unsigned char)a_source[bRawIndex + channel];
          }
        }
        a_result[offset + channel] = (char)(value / c);
      }
    }
)

int main(int a_argc, char* a_argv[]){
  std::string sourceFilePath;
  std::string outputFilePath;
  for(int i = 1; i < a_argc; ++i) {
    if (!std::strcmp(a_argv[i], "-h") || !std::strcmp(a_argv[i], "--help")) {
      printHelp();
      return 0;
    } else if (sourceFilePath.empty()) {
      sourceFilePath = a_argv[i];
    } else if (outputFilePath.empty()) {
      outputFilePath = a_argv[i];
    }

  }
  if (sourceFilePath.empty() || outputFilePath.empty()) {
    std::cout << "Incorrent command line parameters. Use --help option for got help." << std::endl;
    return 1;
  }

  std::vector<char> sourceRGB;
  size_t            sourceRGBWidth;
  size_t            sourceRGBHeight;
  try {
    fcf::Image::loadRGBFromBmpFile(sourceFilePath, sourceRGB, sourceRGBWidth, sourceRGBHeight);
  } catch(std::exception& e){
    std::cerr << "Invalid load BMP file: " << e.what() << std::endl;
    return 1;
  }

  std::vector<char> outputRGB(sourceRGB.size());

  fcf::Union state;

  try {
    fcf::Parallel::Executor executor;
    executor.initialize();

    fcf::Parallel::Call call;
    call.name = "blur";
    call.size = sourceRGBWidth * sourceRGBHeight;
    call.stat = &state;

    executor(call,
             (unsigned int)5,
             (unsigned int)sourceRGBWidth,
             (unsigned int)sourceRGBHeight,
             fcf::Parallel::refArg(sourceRGB),
             fcf::Parallel::refArg(outputRGB,
                                   fcf::Parallel::ArgSplit(fcf::Parallel::PS_FULL),
                                   fcf::Parallel::ArgUpload(true),
                                   fcf::Parallel::ArgSplitSize(3)
                                  )
             );
  } catch(std::exception& e) {
    std::cerr << "Error in performing parallel calculations: " << e.what() << std::endl;
    return 1;
  }

  try {
    fcf::Image::writeRGBToBmpFile(outputFilePath, outputRGB, sourceRGBWidth, sourceRGBHeight);
  } catch(std::exception& e){
    std::cerr << "Invalid write BMP file: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "Time spent on implementation: " << ((double)state["packageDuration"]/(1000*1000*1000)) << " sec" << std::endl;
  std::cout << "Actions performed on the following devices: " << std::endl;
  for(fcf::Union& dev : state["devices"]) {
    std::cout << "    Engine: "<< dev["engine"] << "; Device: " << dev["device"] << std::endl;
  }

  return 0;
}
