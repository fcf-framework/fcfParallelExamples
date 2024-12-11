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
      int offset = (int)(a_task->lowIndex * 3);
      int y = (int)(a_task->lowIndex / a_width);
      int x = (int)(a_task->lowIndex % a_width);
      int begby = max(y - (int)a_blur, 0);
      int endby = min(y + (int)a_blur + 1, a_height);
      int begbx = max(x - (int)a_blur, 0);
      int endbx = min(x + (int)a_blur + 1, a_width);
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
      /*
      std::cout << a_task->offset << " ["
                << (unsigned int)(unsigned char)a_result[offset] << ":"
                << (unsigned int)(unsigned char)a_result[offset + 1] << ":"
                << (unsigned int)(unsigned char)a_result[offset + 2] << ":"
                << "]" << std::endl;
                */
    }
)



bool enableEngine(int a_devIndex, fcf::Parallel::Executor& a_executor, fcf::Union& a_state) {
  a_state.ref<fcf::UnionMap>();
  a_state["devices"].ref<fcf::UnionVector>();
  bool result = false;
  int curDeviceIndex = 0;
  size_t engineCount = a_executor.getEngineCount();
  for(size_t engineIndex = 0; engineIndex < engineCount; ++engineIndex) {
    if (a_devIndex >= 0) {
      a_executor.getEngine(engineIndex).property("enable", false);
    }
    if (a_executor.getEngine(engineIndex).property("devices").is(fcf::UT_VECTOR)) {
      for(fcf::Union& propDev : a_executor.getEngine(engineIndex).property("devices")) {
        if (a_devIndex >= 0) {
          propDev["enable"] = false;
        }
        if (a_devIndex < 0 &&
            (bool)a_executor.getEngine(engineIndex).property("enable") &&
            (bool)propDev["enable"]) {
          fcf::Union udev(fcf::UT_MAP);
          udev["engineIndex"] = engineIndex;
          udev["deviceName"] = (std::string)a_executor.getEngine(engineIndex).property("name") + " (" + (std::string)propDev["name"] + ")";
          a_state["devices"].insert(udev);
          result = true;
        } else if (curDeviceIndex == a_devIndex) {
          fcf::Union udev(fcf::UT_MAP);
          udev["engineIndex"] = engineIndex;
          udev["deviceName"] = (std::string)a_executor.getEngine(engineIndex).property("name") + " (" + (std::string)propDev["name"] + ")";
          a_state["devices"].insert(udev);
          a_executor.getEngine(engineIndex).property("enable", true);
          propDev["enable"]      = true;
          result = true;
        }
        ++curDeviceIndex;
      }
    } else {
      if (a_devIndex < 0 && (bool)a_executor.getEngine(engineIndex).property("enable")) {
        fcf::Union udev(fcf::UT_MAP);
        udev["engineIndex"] = engineIndex;
        udev["deviceName"] = (std::string)a_executor.getEngine(engineIndex).property("name");
        a_state["devices"].insert(udev);
        result = true;
      } if (curDeviceIndex == a_devIndex) {
        fcf::Union udev(fcf::UT_MAP);
        udev["engineIndex"] = engineIndex;
        udev["deviceName"] = (std::string)a_executor.getEngine(engineIndex).property("name");
        a_state["devices"].insert(udev);
        a_executor.getEngine(engineIndex).property("enable", true);
        result = true;
      }
      ++curDeviceIndex;
    }
  }
  return result;
}

void executeEngine(int, fcf::Parallel::Executor& a_executor, const std::string& a_sourceFilePath, const std::string& a_outputFilePath, fcf::Union& a_state) {
  std::vector<char> sourceRGB;
  size_t            sourceRGBWidth;
  size_t            sourceRGBHeight;
  fcf::Image::loadRGBFromBmpFile(a_sourceFilePath, sourceRGB, sourceRGBWidth, sourceRGBHeight);

  std::vector<char> outputRGB(sourceRGB.size(), (char)(unsigned char)0xff);

  a_executor.initialize();

  fcf::Parallel::Stat stat;

  fcf::Parallel::Call call;
  call.name = "blur";
  call.size = sourceRGBWidth * sourceRGBHeight;
  call.split = false;
  call.packageSize = call.size;
  call.stat = &stat;

  a_executor(call,
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

  fcf::Image::writeRGBToBmpFile(a_outputFilePath, outputRGB, sourceRGBWidth, sourceRGBHeight);

  a_state["duration"] = (double)stat.packageDuration / 1000 / 1000 / 1000;
}

void printStartInfo(int, fcf::Parallel::Executor&, const std::string&, const std::string& a_output, fcf::Union& a_state) {
  std::string devicesStr;
  for(fcf::Union& udev : a_state["devices"]){
    if (!devicesStr.empty()){
      devicesStr += "; ";
    }
    devicesStr += (std::string)udev["engineIndex"] + ":" + (std::string)udev["deviceName"];
  }
  std::cout << "------------"  << std::endl;
  std::cout << "Output file: " << a_output << std::endl;
  std::cout << "Devices:     "     << devicesStr << std::endl;
}

void printResultInfo(int, fcf::Parallel::Executor&, const std::string&, const std::string&, fcf::Union& a_state) {
  std::cout << "Duration:    " << a_state["duration"] << std::endl;
  std::cout << std::endl;
}

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

  size_t      outputSuffixPos = outputFilePath.rfind(".");
  std::string outputSuffix    = std::string::npos != outputSuffixPos ? outputFilePath.substr(outputSuffixPos)     : ".bmp";
  std::string outputPrefix    = std::string::npos != outputSuffixPos ? outputFilePath.substr(0, outputSuffixPos)  : "";

  int engineIndex = -1;
  while(true){
    fcf::Union state;
    fcf::Parallel::Executor executor;
    if (!enableEngine(engineIndex, executor, state)) {
      break;
    }
    executor.getEngine("cpu").property("threads", 3);
    std::string currentOutputFilePath = outputPrefix +
                                        ".device_" +
                                        (engineIndex == -1 ? std::string("default") : std::to_string(engineIndex)) +
                                        outputSuffix;
    printStartInfo(engineIndex, executor, sourceFilePath, currentOutputFilePath, state);
    executeEngine(engineIndex, executor, sourceFilePath, currentOutputFilePath, state);
    printResultInfo(engineIndex, executor, sourceFilePath, currentOutputFilePath, state);
    ++engineIndex;
  }
  return 0;
}
