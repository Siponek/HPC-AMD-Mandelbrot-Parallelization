#pragma once

#include <string>

namespace MPI_logs
{

/**
 * @brief Extracts the base filename up to the second underscore.
 *
 * This function parses the provided filename and returns the
 * substring from the beginning up to the second underscore
 * character (`_`). If the filename contains fewer than two
 * underscores, the original filename is returned.
 *
 * @param filename The full filename as a string.
 * @return The base filename up to the second underscore, or the
 * original filename if fewer underscores are present.
 */
std::string extractBaseFilename(const std::string &filename);

/**
 * @brief Generalized function to create a filename with a specific
 * folder and extension.
 *
 * This function constructs a new filename by appending an
 * additional name to the base filename extracted up to the second
 * underscore. It then combines this new filename with the specified
 * folder and extension. The function also ensures that the target
 * directory exists, creating it if necessary.
 *
 * **Note:** This implementation does not use the `<filesystem>`
 * library and relies on standard string manipulation and POSIX
 * system calls for directory management.
 *
 * @param output_file The original output file path as a string.
 * @param additionalName The additional name to append to the base
 * filename.
 * @param folder The target folder where the new file will be
 * placed.
 * @param extension The file extension for the new file (e.g.,
 * ".csv").
 * @return The generated file path as a string.
 */
std::string createFilename(const std::string &output_file,
						   const std::string &additionalName,
						   const std::string &folder,
						   const std::string &extension);

/**
 * @brief Creates a CSV filename based on the output file and an
 * additional name.
 *
 * This function leverages `createFilename` to generate a
 * CSV-specific filename by appending the provided additional name
 * and setting the appropriate folder and extension for CSV files.
 *
 * @param output_file The original output file path as a string.
 * @param additionalName An optional additional name to append to
 * the base filename.
 * @return The generated CSV file path as a string.
 */
std::string createCsvFilename(const std::string &output_file,
							  const std::string &additionalName);

/**
 * @brief Checks if a CSV file contains the specified header.
 *
 * This function opens the CSV file located at `filePath`, reads the
 * first line, and compares it to the provided `header` string. It
 * returns `true` if the header matches; otherwise, it returns
 * `false`. Additionally, it outputs diagnostic messages to indicate
 * the status of the operation.
 *
 * @param filePath The path to the CSV file as a string.
 * @param header The expected header line as a string.
 * @return `true` if the header matches and the file was read
 * successfully; `false` otherwise.
 */
bool csvFileHasHeader(const std::string &filePath,
					  const std::string &header);

} // namespace MPI_logs
