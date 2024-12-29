#include "LogUtils.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
namespace logutils
{
std::string getCurrentTimestamp()
{
	auto now = std::chrono::system_clock::now();
	std::time_t now_time_t =
		std::chrono::system_clock::to_time_t(now);
	std::tm now_tm;
#ifdef _WIN32
	localtime_s(&now_tm, &now_time_t);
#else
	localtime_r(&now_time_t, &now_tm);
#endif

	std::ostringstream timestamp_stream;
	timestamp_stream << std::put_time(
		&now_tm,
		"%b %d %Y %H:%M:%S"); // e.g., "Oct 19 2024 21:35:37"
	return timestamp_stream.str();
}

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
std::string createFilename(const std::string &output_file,
						   const std::string &additionalName,
						   const std::string &folder,
						   const std::string &extension)
{
	fs::path outputPath(output_file);
	std::string baseFilename =
		extractBaseFilename(outputPath.stem().string());

	fs::path targetPath =
		outputPath.parent_path().parent_path() / folder /
		(baseFilename + additionalName + extension);

	fs::create_directories(targetPath.parent_path());
	std::cout << "Creating file: " << targetPath.string()
			  << std::endl;
	return targetPath.string();
}

std::string create_log_file_name(const std::string &output_file,
								 const std::string &additionalName)
{
	return createFilename(output_file, additionalName, "logs",
						  ".log");
}

std::string createCsvFilename(const std::string &output_file,
							  const std::string &additionalName)
{
	std::cout << "Creating CSV file name from output file: "
			  << output_file << std::endl;
	std::string csvPath =
		createFilename(output_file, additionalName, "data", ".csv");
	return csvPath;
}

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

} // namespace logutils

namespace cmdParse
{
cmdParse::Command get_command(const std::string &arg)
{
	if (arg == "--help")
		return Command::HELP;
	if (arg == "--version")
		return Command::VERSION;
	if (arg == "--iterations")
		return Command::ITERATIONS;
	if (arg == "--resolution")
		return Command::RESOLUTION;
	if (arg == "--threads")
		return Command::THREADS_NUMBER;
	// Assuming the first non-flag argument is the output file
	return Command::INVALID;
}

cmdParse::ParsedArgs parse_cmd_arguments(int argc, char *argv[])
{
	ParsedArgs args;
	try
	{
		std::filesystem::path filePath = argv[0];
		std::string fileName = filePath.filename().string();

		if (argc < 2)
		{
			std::cerr << "Usage: " << fileName
					  << " <output_file> [options]" << std::endl;
			exit(EXIT_FAILURE);
		}

		for (int i = 1; i < argc; ++i)
		{
			std::string arg = argv[i];
			Command cmd = get_command(arg);

			switch (cmd)
			{
			case Command::HELP:
				std::cout
					<< "Usage: " << fileName
					<< " <output_file> [--iterations <iterations>] "
					   "[--resolution <resolution>] [--version]"
					<< std::endl;
				exit(EXIT_SUCCESS);

			case Command::VERSION:
				std::cout << "Version: 1.0.0" << std::endl;
				exit(EXIT_SUCCESS);
			case Command::OUTPUT_FILE:
				if (args.output_file.empty())
				{
					args.output_file = arg;
				}
				else
				{
					std::cerr
						<< "Invalid argument or multiple output "
						   "files specified: "
						<< arg << std::endl;
					exit(EXIT_FAILURE);
				}
				break;
			case Command::ITERATIONS:
				if (i + 1 < argc)
				{
					args.iterations = std::stoi(argv[++i]);
					if (args.iterations <= 0)
					{
						std::cerr
							<< "--iterations must be a positive "
							   "integer."
							<< std::endl;
						exit(EXIT_FAILURE);
					}
				}
				else
				{
					std::cerr << "--iterations requires a value."
							  << std::endl;
					exit(EXIT_FAILURE);
				}
				break;

			case Command::RESOLUTION:
				if (i + 1 < argc)
				{
					args.resolution = std::stoi(argv[++i]);
					if (args.resolution <= 0)
					{
						std::cerr
							<< "--resolution must be a positive "
							   "integer."
							<< std::endl;
						exit(EXIT_FAILURE);
					}
				}
				else
				{
					std::cerr << "--resolution requires a value."
							  << std::endl;
					exit(EXIT_FAILURE);
				}
				break;
			case Command::THREADS_NUMBER:
				if (i + 1 < argc)
				{
					args.threads_num = std::stoi(argv[++i]);
					if (args.threads_num <= 0)
					{
						std::cerr << "--threads must be a positive "
									 "integer."
								  << std::endl;
						exit(EXIT_FAILURE);
					}
				}
				else
				{
					std::cerr << "--threads requires a value."
							  << std::endl;
					exit(EXIT_FAILURE);
				}
				break;
			case Command::INVALID:
				if (args.output_file.empty())
				{
					args.output_file = arg;
				}
				else
				{
					std::cerr
						<< "Invalid argument or multiple output "
						   "files specified: "
						<< arg << std::endl;
					exit(EXIT_FAILURE);
				}
				break;
			}
		}

		if (args.output_file.empty())
		{
			std::cerr
				<< "Please specify the output file as a parameter."
				<< std::endl;
			exit(EXIT_FAILURE);
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error parsing arguments: [";
		for (int i = 0; i < argc; ++i)
		{
			std::cerr << argv[i];
			if (i < argc - 1)
				std::cerr << ", ";
		}
		std::cerr << "] " << e.what() << std::endl;
		exit(EXIT_FAILURE);
	}
	return args;
}
} // namespace cmdParse
