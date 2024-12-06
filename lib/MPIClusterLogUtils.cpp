#pragma once
#include <MPIClusterLogUtils.hpp>

#include <errno.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
namespace MPI_logs
{

/**
 * @brief Extracts the parent directory from a given file path.
 *
 * @param path The full file path.
 * @return The parent directory path as a string. Returns "." if no
 * parent is found.
 */
std::string getParentPath(const std::string &path)
{
	size_t found = path.find_last_of("/\\");
	if (found != std::string::npos)
		return path.substr(0, found);
	else
		return "."; // Current directory
}
/**
 * @brief Recursively creates directories similar to the Unix 'mkdir
 * -p' command.
 *
 * @param path The directory path to create.
 * @return `true` if all directories were created successfully or
 * already exist; `false` otherwise.
 */
bool mkdir_p(const std::string &path)
{
	size_t pos = 0;
	bool success = true;
	do
	{
		pos = path.find_first_of("/\\", pos + 1);
		std::string subdir = path.substr(0, pos);
		if (subdir.empty())
			continue; // Skip empty
		if (mkdir(subdir.c_str(), 0755) != 0)
		{ // rwxr-xr-x permissions
			if (errno == EEXIST)
				continue; // Directory already exists
			else
			{
				perror(("mkdir " + subdir).c_str());
				success = false;
				break;
			}
		}
	} while (pos != std::string::npos);
	return success;
}
/**
 * @brief Extracts the base filename up to the second underscore.
 *
 * @param filename The full filename.
 * @return The base filename up to the second underscore, or the
 * original filename if fewer than two underscores are present.
 */
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
/**
 * @brief Generalized function to create a filename with a specific
 * folder and extension without using <filesystem>.
 *
 * @param output_file The original output file path.
 * @param additionalName The additional name to append to the base
 * filename.
 * @param folder The target folder to place the new file in.
 * @param extension The file extension for the new file.
 * @return The generated file path as a string.
 */
std::string createFilename(const std::string &output_file,
						   const std::string &additionalName,
						   const std::string &folder,
						   const std::string &extension)
{
	// Extract the filename from output_file
	size_t last_sep = output_file.find_last_of("/\\");
	std::string filename = (last_sep != std::string::npos)
							   ? output_file.substr(last_sep + 1)
							   : output_file;

	// Remove extension from filename
	size_t last_dot = filename.find_last_of('.');
	std::string stem = (last_dot != std::string::npos)
						   ? filename.substr(0, last_dot)
						   : filename;

	// Extract base filename up to second underscore
	std::string baseFilename = extractBaseFilename(stem);

	// Get parent path
	std::string parent_path = getParentPath(output_file);

	// Get grandparent path (parent of parent)
	std::string grandparent_path = getParentPath(parent_path);

	// Append the target folder
	std::string target_dir = grandparent_path + "/" + folder;

	// Create the target directory if it doesn't exist
	if (!mkdir_p(target_dir))
	{
		std::cerr << "Error: Could not create directories: "
				  << target_dir << std::endl;
		// Handle error as needed (e.g., throw exception, return
		// empty string, etc.)
	}

	// Create new filename by appending additionalName and extension
	std::string new_filename =
		baseFilename + additionalName + extension;

	// Construct the full target path
	std::string target_path = target_dir + "/" + new_filename;

	std::cout << "Creating file: " << target_path << std::endl;

	return target_path;
}
/**
 * @brief Creates a CSV filename based on the output file and an
 * additional name.
 *
 * @param output_file The original output file path.
 * @param additionalName An optional additional name to append to
 * the base filename.
 * @return The generated CSV file path as a string.
 */
std::string createCsvFilename(const std::string &output_file,
							  const std::string &additionalName)
{
	std::cout << "Creating CSV file name from output file: "
			  << output_file << std::endl;
	std::string csvPath =
		createFilename(output_file, additionalName, "data", ".csv");
	return csvPath;
}
/**
 * @brief Checks if a CSV file contains the specified header.
 *
 * @param filePath The path to the CSV file.
 * @param header The expected header string.
 * @return `true` if the header matches; `false` otherwise or if the
 * file cannot be read.
 */
bool csvFileHasHeader(const std::string &filePath,
					  const std::string &header)
{
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
	bool has_header = (firstLine == header);
	std::cout << "Header present: "
			  << (has_header ? "true" : "false") << std::endl;
	return has_header;
}
} // namespace MPI_logs
