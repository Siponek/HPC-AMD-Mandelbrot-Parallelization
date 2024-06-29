#include "LogUtils.h"

namespace logutils {

std::string createLogFileName(const std::string &outputFile, const std::string &additionalName) {
    const fs::path outputPath(outputFile);
    std::string filename = outputPath.stem().string();
    size_t underscorePos = filename.find('_');
    underscorePos = filename.find('_', underscorePos + 2);
    if (underscorePos != std::string::npos) {
        filename = filename.substr(0, underscorePos);
    }
    const fs::path logFilePath =
        outputPath.parent_path().parent_path() / "logs" /
        (filename + additionalName + ".log");
    fs::create_directories(logFilePath.parent_path());
    return logFilePath.string();
}
} // namespace logutils
