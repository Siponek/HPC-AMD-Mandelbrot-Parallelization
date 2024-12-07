// C++
#include <algorithm>
#include <cctype>
#include <chrono>
#include <complex>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mpi.h>
#include <omp.h>
#include <string>
#include <sys/stat.h>

class NoNumber : public std::exception
{
	std::string message;

  public:
	NoNumber(const std::string &input)
		: message("NoNumber exception: " + input)
	{
	}
	virtual const char *what() const noexcept override
	{
		return message.c_str();
	}
};

class Overflow : public std::exception
{
	std::string message;

  public:
	Overflow(const std::string &input)
		: message("Overflow exception: " + input)
	{
	}
	virtual const char *what() const noexcept override
	{
		return message.c_str();
	}
};

namespace MandelbrotSet
{
// Ranges of the set
constexpr float MIN_X = -2.0;
constexpr float MAX_X = 1.0;
constexpr float MIN_Y = -1.0;
constexpr float MAX_Y = 1.0;
constexpr float RATIO_X = (MAX_X - MIN_X);
constexpr float RATIO_Y = (MAX_Y - MIN_Y);
} // namespace MandelbrotSet

using namespace std;
using namespace MandelbrotSet;

std::string getCurrentTimestamp()
{
	// Get the current time as a time_point
	auto now = std::chrono::system_clock::now();

	// Convert time_point to time_t, which represents the calendar
	// time
	std::time_t now_time_t =
		std::chrono::system_clock::to_time_t(now);

	// Convert time_t to tm structure for local time
	std::tm now_tm;
#ifdef _WIN32
	localtime_s(&now_tm, &now_time_t); // Thread-safe on Windows
#else
	localtime_r(&now_time_t,
				&now_tm); // Thread-safe on POSIX systems
#endif

	// Buffer to hold the formatted date and time
	char buffer[100];

	// Format the date and time into the buffer
	// Example format: "Oct 19 2024 21:35:37"
	if (std::strftime(buffer, sizeof(buffer), "%b %d %Y %H:%M:%S",
					  &now_tm))
	{
		return std::string(buffer);
	}
	else
	{
		// Handle formatting error
		return "Unknown Time";
	}
}
// Function to extract file name from a given path
string getFileName(const string &path)
{
	size_t pos = path.find_last_of("/\\");
	if (pos != string::npos)
	{
		return path.substr(pos + 1);
	}
	else
	{
		return path;
	}
}
// Function to extract the base filename (without extension) and
// limit to first two underscores
string extractBaseFileName(const string &filename)
{
	// Remove extension
	size_t lastDot = filename.find_last_of('.');
	string base = (lastDot != string::npos)
					  ? filename.substr(0, lastDot)
					  : filename;

	// Limit to first two underscores
	size_t firstUnderscore = base.find('_');
	if (firstUnderscore == string::npos)
		return base; // No underscore found

	size_t secondUnderscore = base.find('_', firstUnderscore + 1);
	if (secondUnderscore != string::npos)
		return base.substr(0, secondUnderscore);
	else
		return base; // Only one underscore found
}
string create_csv_file_name(const std::string &output_file,
							const std::string &additionalName = "")
{
	// Extract filename from path
	std::string filename = getFileName(output_file);
	// Extract base filename
	filename = extractBaseFileName(filename);

	// Define log directory
	std::string logDir = "data";

	// Create 'logs' directory if it doesn't exist
	struct stat info;
	if (stat(logDir.c_str(), &info) != 0)
	{
		// Directory does not exist, attempt to create it
		if (mkdir(logDir.c_str(), 0755) != 0)
		{
			std::cerr << "Error: Unable to create csv directory '"
					  << logDir << "'." << endl;
			// You can choose to exit or handle the error as needed
		}
	}
	else if (!(info.st_mode & S_IFDIR))
	{
		std::cerr << "Error: '" << logDir
				  << "' exists but is not a directory." << endl;
		// Handle the error as needed
	}

	// Construct log file path
	std::string logFilePath =
		logDir + "/" + filename + additionalName + ".csv";
	return logFilePath;
}
string create_log_file_name(const std::string &output_file,
							const std::string &additionalName = "")
{
	// Extract filename from path
	std::string filename = getFileName(output_file);
	// Extract base filename
	filename = extractBaseFileName(filename);

	// Define log directory
	std::string logDir = "data";

	// Create 'logs' directory if it doesn't exist
	struct stat info;
	if (stat(logDir.c_str(), &info) != 0)
	{
		// Directory does not exist, create it
		if (mkdir(logDir.c_str(), 0755) != 0)
		{
			std::cerr << "Error: Unable to create csv directory '"
					  << logDir << "'." << endl;
		}
	}
	else if (!(info.st_mode & S_IFDIR))
	{
		std::cerr << "Error: '" << logDir
				  << "' exists but is not a directory." << endl;
	}

	// Construct log file path
	std::string logFilePath =
		logDir + "/" + filename + additionalName + ".csv";
	return logFilePath;
}
string create_log_file_name(const std::string &output_file,
							const std::string &additionalName = "")
{
	// Extract filename from path
	std::string filename = getFileName(output_file);
	// Extract base filename
	filename = extractBaseFileName(filename);

	// Define log directory
	std::string logDir = "logs";

	// Create 'logs' directory if it doesn't exist
	struct stat info;
	if (stat(logDir.c_str(), &info) != 0)
	{
		// Directory does not exist, create it
		if (mkdir(logDir.c_str(), 0755) != 0)
		{
			std::cerr << "Error: Unable to create log directory '"
					  << logDir << "'." << endl;
		}
	}
	else if (!(info.st_mode & S_IFDIR))
	{
		std::cerr << "Error: '" << logDir
				  << "' exists but is not a directory." << endl;
	}

	// Construct log file path
	std::string logFilePath =
		logDir + "/" + filename + additionalName + ".log";
	return logFilePath;
}

void checkMPIError(int errCode, const string &msg)
{
	if (errCode != MPI_SUCCESS)
	{
		cerr << "MPI Error: " << msg << " Error Code: " << errCode
			 << endl;
		MPI_Abort(MPI_COMM_WORLD, errCode);
	}
}

// Rewritten isValidOutputPath without using filesystem
bool isValidOutputPath(const string &output_file)
{
	// Extract parent directory from output_file
	size_t pos = output_file.find_last_of("/\\");
	string parentPath;
	if (pos != string::npos)
	{
		parentPath = output_file.substr(0, pos);
	}
	else
	{
		parentPath =
			"."; // Current directory if no parent path is found
	}

	// Check if parent directory exists
	struct stat sb;
	if (!parentPath.empty())
	{
		if (stat(parentPath.c_str(), &sb) != 0)
		{
			cerr << "Error: The directory " << parentPath
				 << " does not exist." << endl;
			return false;
		}

		// Check if it is indeed a directory
		if (!S_ISDIR(sb.st_mode))
		{
			cerr << "Error: The path " << parentPath
				 << " is not a directory." << endl;
			return false;
		}
	}

	// Attempt to open the file to check write permissions
	std::ofstream testFile(output_file.c_str());
	if (!testFile.is_open())
	{
		cerr << "Error: Cannot write to the file " << output_file
			 << endl;
		return false;
	}
	testFile.close();
	return true;
}

int castInput(std::string &input)
{
	int result{};
	// Check if there is any alphabetic character in the input
	// string
	if (std::any_of(input.begin(), input.end(), ::isalpha))
	{
		// If so, throw NoNumber exception
		throw NoNumber(input);
	}
	// Check if the string has more than 10 characters to prevent
	// integer overflow
	if (input.length() > 10)
	{
		// If so, throw Overflow exception
		throw Overflow(input);
	}
	// Convert string to integer
	result = std::stoi(input);
	return result;
}
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
	cout << "Creating directory for" << target_dir << endl;
	// Create the target directory if it doesn't exist
	if (!mkdir_p(target_dir))
	{
		std::cerr << "Error: Could not create directories: "
				  << target_dir << std::endl;
	}

	// Create new filename by appending additionalName and extension
	std::string new_filename =
		baseFilename + additionalName + extension;

	// Construct the full target path
	std::string target_path = target_dir + "/" + new_filename;

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
	std::cout << "Creating CSV file for logs: "
			  << output_file + " " + additionalName << std::endl;
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
		exit(EXIT_FAILURE);
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
int main(int argc, char **argv)
{
	// cout.sync_with_stdio(false);
	string programPath = argv[0];
	string fileName =
		getFileName(programPath); // Extracted filename

	int err, nproc, myid;
	err = MPI_Init(&argc, &argv);
	checkMPIError(err, "MPI_Init failed.");
	err = MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	checkMPIError(err, "MPI_Comm_size failed.");
	err = MPI_Comm_rank(MPI_COMM_WORLD, &myid);
	checkMPIError(err, "MPI_Comm_rank failed.");
	if (argc < 4)
	{
		if (myid == 0)
		{
			cerr << "Incorrect number of argumennts!";
			cerr << "Usage: " << fileName
				 << " <output_file> <iterations> <resolution_value>"
				 << endl;
		}
		MPI_Finalize();
		return -1;
	}

	int iterations = 0;
	int resolution_value = 0;
	string argToString = argv[2];

	try
	{
		// Convert iterations input to integer
		argToString = argv[2];
		iterations = castInput(argToString);
		if (iterations <= 0)
		{
			cerr
				<< "Please specify a positive number of iterations."
				<< endl;
			MPI_Finalize();
			return -2;
		}
	}
	// Catch custom exceptions
	catch (const NoNumber &e)
	{
		cerr << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
	catch (const Overflow &e)
	{
		cerr << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
	// Catch standard exceptions from stoi
	catch (const std::invalid_argument &e)
	{
		cerr << "Invalid argument: " << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
	catch (const std::out_of_range &e)
	{
		cerr << "Out of range error: " << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}

	// Parse and validate resolution_value input
	try
	{
		argToString = argv[3];
		resolution_value = castInput(argToString);
		if (resolution_value <= 0)
		{
			cerr << "Please specify a positive resolution_value."
				 << endl;
			MPI_Finalize();
			return -3;
		}
	}
	catch (const NoNumber &e)
	{
		cerr << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
	catch (const Overflow &e)
	{
		cerr << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
	catch (const std::invalid_argument &e)
	{
		cerr << "Invalid argument: " << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
	catch (const std::out_of_range &e)
	{
		cerr << "Out of range error: " << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}

	// Check if the output file path is valid on the root process
	if (myid == 0 && !isValidOutputPath(argv[1]))
	{
		MPI_Finalize();
		return -4;
	}

	// Root process outputs number of nodes and resolution_value
	if (myid == 0)
	{
		cout << "Number of nodes: " << nproc << endl;
		cout << "Resolution: " << resolution_value << endl;
	}

	const int HEIGHT = resolution_value * RATIO_Y;
	const int WIDTH = resolution_value * RATIO_X;
	const float STEP = RATIO_X / WIDTH;
	const int ITERATIONS = iterations;

	const int total_pixels = HEIGHT * WIDTH;
	const int pixels_per_process = total_pixels / nproc;
	const int start_index = myid * pixels_per_process;
	const int end_index = (myid + 1) * pixels_per_process;

	int *image = nullptr;
	int *sub_image = new int[pixels_per_process]();

	if (myid == 0)
	{
		image = new int[total_pixels];
	}
	// Setting max threads per node
	int threads_used = omp_get_max_threads();
	omp_set_num_threads(threads_used);

	auto start_time = chrono::steady_clock::now();

#pragma omp parallel for schedule(dynamic) default(none)           \
	firstprivate(sub_image, ITERATIONS) shared(WIDTH, STEP)
	for (int pos = start_index; pos < end_index; pos++)
	{
		const int row = pos / WIDTH;
		const int col = pos % WIDTH;
		const complex<double> c(col * STEP + MIN_X,
								row * STEP + MIN_Y);

		int iter = 1;
		complex<double> z(0, 0);
		for (; iter <= ITERATIONS; iter++)
		{
			z = (z * z) + c;

			// If the magnitude exceeds 2, the point is not in the
			// Mandelbrot set if (abs(z) >= 2)
			if (((z.real() * z.real()) + (z.imag() * z.imag())) >=
				4)
			{
				sub_image[pos - start_index] = iter;
				break;
			}
		}
	}

	// Gather results from all processes to the root process
	err =
		MPI_Gather(sub_image, pixels_per_process, MPI_INT, image,
				   pixels_per_process, MPI_INT, 0, MPI_COMM_WORLD);
	checkMPIError(err, "MPI_Gather failed.");

	if (myid == 0)
	{
		auto end_time = chrono::steady_clock::now();
		double elapsed_seconds =
			chrono::duration<double>(end_time - start_time).count();
		cout << "Time elapsed: " << fixed << setprecision(2)
			 << elapsed_seconds << " seconds." << endl;
		//? Create csv file
		// Create CSV filename
		std::string csv_filename = createCsvFilename(argv[1], "");
		std::cout << "Created csv file: " << csv_filename
				  << std::endl;

		// Define header
		std::string header =
			"DateTime,Program,Iterations,Resolution,Width,Height,"
			"Step,NProcesses,Threads,Time (seconds)";

		// Check if CSV has header

		ofstream csv_stream(csv_filename, std::ios::app);
		bool has_header = csvFileHasHeader(csv_filename, header);
		if (csv_stream.is_open())
		{
			if (!has_header)
			{
				std::cout << "Adding header to csv file."
						  << std::endl;
				csv_stream << header << std::endl;
			}
			csv_stream << getCurrentTimestamp() << "," << argv[0]
					   << "," << iterations << ","
					   << resolution_value << "," << WIDTH << ","
					   << HEIGHT << "," << STEP << "," << nproc
					   << "," << "," << threads_used << ","
					   << elapsed_seconds << endl;
			csv_stream.close();
			cout << "CSV entry added successfully." << endl;

			csv_stream.close();
		}
		else
		{
			std::cerr << "Unable to open csv file." << endl;
		}

		//? Create log file path
		string log_file =
			create_log_file_name(argv[1], "_openMPI_");
		ofstream log(log_file, std::ios::app);
		std::cout << "LOG: log stream opened" << std::endl;
		if (log.is_open())
		{
			log << "\tProgram:\t" << fileName << "\tIterations:\t"
				<< iterations << "\tResolution:\t"
				<< resolution_value << "\tWidth:\t" << WIDTH
				<< "\tHeight:\t" << HEIGHT << "\tStep:\t" << STEP
				<< "\tNodes:\t" << nproc
				<< "\tProcesses per Node:\t"
				<< (nproc > 0 ? (total_pixels / nproc) : 0)
				<< "\tTime:\t" << elapsed_seconds << " seconds"
				<< endl;

			log.close();
		}
		else
		{
			std::cerr << "Unable to open log file." << endl;
		}
		// Write the result to a file
		ofstream matrix_out;

		matrix_out.open(argv[1], ios::trunc);
		std::cout << "LOG: matrix out stream opened" << std::endl;
		if (!matrix_out.is_open())
		{
			cerr << "Unable to open file." << endl;
			MPI_Abort(MPI_COMM_WORLD, -3);
		}
		auto start_time_out = chrono::steady_clock::now();
		std::cout << "Starting writing to out file..." << std::endl;
		for (int row = 0; row < HEIGHT; row++)
		{
			for (int col = 0; col < WIDTH; col++)
			{
				matrix_out << image[row * WIDTH + col];
				if (col < WIDTH - 1)
					matrix_out << ',';
			}
			if (row < HEIGHT - 1)
				matrix_out << endl;
		}
		auto end_time_out = chrono::steady_clock::now();
		double elapsed_seconds_out =
			chrono::duration<double>(end_time_out - start_time_out)
				.count();
		std::cout << "Finished writing to out file in "
				  << elapsed_seconds << "seconds" << std::endl;
		matrix_out.close();
		delete[] image;
		std::cout << "Exiting..." << std::endl;
	}
	delete[] sub_image;
	err = MPI_Finalize();
	checkMPIError(err, "MPI_Finalize failed.");
	return 0;
}
