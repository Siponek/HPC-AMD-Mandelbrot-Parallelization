// LogUtils.h
#pragma once
#include <string>

namespace logutils
{
/**
 * @brief Creates a log file name based on the output file and an
 * additional name.
 *
 * This function extracts the base filename up to the second
 * underscore, appends the additional name, and sets the file
 * extension to ".log". It also ensures that the necessary
 * directories exist.
 *
 * @param outputFile The original output file path.
 * @param additionalName An optional additional name to append to
 * the base filename.
 * @return The generated log file path as a string.
 */
std::string
createLogFileName(const std::string &outputFile,
				  const std::string &additionalName = "");

/**
 * @brief Creates a CSV file name based on the output file and an
 * additional name.
 *
 * This function extracts the base filename up to the second
 * underscore, appends the additional name, and sets the file
 * extension to ".csv". It also ensures that the necessary
 * directories exist.
 *
 * @param outputFile The original output file path.
 * @param additionalName An optional additional name to append to
 * the base filename.
 * @return The generated CSV file path as a string.
 */
std::string
createCsvFilename(const std::string &outputFile,
				  const std::string &additionalName = "");

/**
 * @brief Checks if a CSV file contains the specified header.
 *
 * This function opens the CSV file, reads the first line, and
 * compares it to the provided header string.
 *
 * @param filePath The path to the CSV file.
 * @param header The expected header string.
 * @return `true` if the header matches; `false` otherwise or if the
 * file cannot be read.
 */
bool csvFileHasHeader(const std::string &filePath,
					  const std::string &header);
} // namespace logutils

namespace cmdParse
{
enum class Command
{
	HELP,
	VERSION,
	ITERATIONS,
	RESOLUTION,
	OUTPUT_FILE,
	INVALID
};

struct ParsedArgs
{
	std::string outputFile;
	int iterations = 0;
	int resolution = 0;
};

cmdParse::Command getCommand(const std::string &arg);

cmdParse::ParsedArgs parse_cmd_arguments(int argc, char *argv[]);
} // namespace cmdParse