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
				 << " <output_file> <iterations> <resolution>"
				 << endl;
		}
		MPI_Finalize();
		return -1;
	}

	int iterations = 0;
	int resolution = 0;
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

	// Parse and validate resolution input
	try
	{
		argToString = argv[3];
		resolution = castInput(argToString);
		if (resolution <= 0)
		{
			cerr << "Please specify a positive resolution." << endl;
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

	// Root process outputs number of nodes and resolution
	if (myid == 0)
	{
		cout << "Number of nodes: " << nproc << endl;
		cout << "Resolution: " << resolution << endl;
	}

	// Define HEIGHT, WIDTH, STEP based on resolution
	// Adjust these definitions as per your application's logic
	const int HEIGHT = resolution * RATIO_Y;
	const int WIDTH = resolution * RATIO_X;
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

	auto start_time = chrono::steady_clock::now();

	// Each process computes its portion of the image
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

		// Create log file path
		string log_file = create_log_file_name(argv[1], "_MPI_");
		ofstream log(log_file, std::ios::app);
		if (log.is_open())
		{
			log << "\tProgram:\t" << fileName << "\tIterations:\t"
				<< iterations << "\tResolution:\t" << resolution
				<< "\tWidth:\t" << WIDTH << "\tHeight:\t" << HEIGHT
				<< "\tStep:\t" << STEP << "\tNodes:\t" << nproc
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
		if (!matrix_out.is_open())
		{
			cerr << "Unable to open file." << endl;
			MPI_Abort(MPI_COMM_WORLD, -3);
		}

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
		matrix_out.close();
		delete[] image;
	}

	delete[] sub_image;
	err = MPI_Finalize();
	checkMPIError(err, "MPI_Finalize failed.");
	return 0;
}
