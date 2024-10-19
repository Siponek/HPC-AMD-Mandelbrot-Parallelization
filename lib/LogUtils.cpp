#include "LogUtils.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
namespace logutils
{

// Helper function to extract the base filename up to the second
// underscore
std::string extractBaseFilename(const std::string &filename)
{
	size_t firstUnderscore = filename.find('_');
	if (firstUnderscore == std::string::npos)
		return filename;

	size_t secondUnderscore =
		filename.find('_', firstUnderscore + 1);
	if (secondUnderscore == std::string::npos)
		return filename;

	return filename.substr(0, secondUnderscore);
}

// Generalized function to create a filename with a specific folder
// and extension
std::string createFilename(const std::string &outputFile,
						   const std::string &additionalName,
						   const std::string &folder,
						   const std::string &extension)
{
	fs::path outputPath(outputFile);
	std::string baseFilename =
		extractBaseFilename(outputPath.stem().string());

	fs::path targetPath =
		outputPath.parent_path().parent_path() / folder /
		(baseFilename + additionalName + extension);

	fs::create_directories(targetPath.parent_path());

	return targetPath.string();
}

std::string createLogFileName(const std::string &outputFile,
							  const std::string &additionalName)
{
	return createFilename(outputFile, additionalName, "logs",
						  ".log");
}

std::string createCsvFilename(const std::string &outputFile,
							  const std::string &additionalName)
{
	std::cout << "Creating CSV file name from output file: "
			  << outputFile << std::endl;
	std::string csvPath =
		createFilename(outputFile, additionalName, "data", ".csv");
	std::cout << "CSV file path: " << csvPath << std::endl;
	return csvPath;
}

bool csvFileHasHeader(const std::string &filePath,
					  const std::string &header)
{
	std::cout << "Checking if CSV file has header: " << filePath
			  << std::endl;
	std::ifstream file(filePath);
	if (!file.good() || !file.is_open())
	{
		std::cerr << "Error: Cannot open file: " << filePath
				  << std::endl;
		return false;
	}

	std::string firstLine;
	if (!std::getline(file, firstLine))
	{
		std::cerr << "Error: Unable to read from file: " << filePath
				  << std::endl;
		return false;
	}
	std::cout << firstLine << std::endl;
	std::cout << header << std::endl;
	bool hasHeader = (firstLine == header);
	std::cout << "Header present: " << (hasHeader ? "Yes" : "No")
			  << std::endl;
	return hasHeader;
}

} // namespace logutils
