// LogUtils.h
#pragma once
#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
namespace logutils
{
std::string
createLogFileName(const std::string &outputFile,
				  const std::string &additionalName = "");
}
#endif // LOG_UTILS_H
