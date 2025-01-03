#include <chrono>
#include <complex>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <mpi.h>

namespace MandelbrotSet
{
// Ranges of the set
constexpr float MIN_X = -2.0;
constexpr float MAX_X = 1.0;
constexpr float MIN_Y = -1.0;
constexpr float MAX_Y = 1.0;
}
using namespace std;
using namespace MandelbrotSet;
namespace fs = std::filesystem;

void checkMPIError(int errCode, const string &msg)
{
    if (errCode != MPI_SUCCESS)
    {
        cerr << "MPI Error: " << msg << " Error Code: " << errCode << endl;
        MPI_Abort(MPI_COMM_WORLD, errCode);
    }
}

bool isValidOutputPath(const string& output_file) {
    fs::path outputPath(output_file);
    
    // Check if the parent directory exists
    fs::path parentPath = outputPath.parent_path();
    if (!parentPath.empty() && !fs::exists(parentPath)) {
        cerr << "Error: The directory " << parentPath << " does not exist." << endl;
        return false;
    }

    // Check if the path is writable
    if (!parentPath.empty() && !fs::is_directory(parentPath)) {
        cerr << "Error: The path " << parentPath << " is not a directory." << endl;
        return false;
    }

    // Check if we can write to the directory
    std::ofstream testFile(output_file);
    if (!testFile.is_open()) {
        cerr << "Error: Cannot write to the file " << output_file << endl;
        return false;
    }
    testFile.close();
    return true;
}
int castInput(std::string& input) {
    int result{};
    // Check, if there is any letter in the input string
    if (std::any_of(input.begin(), input.end(), isalpha)) {
        // If so, than throw
        throw NoNumber(input);
    }
    // Check, if string has more than 10 characters
    if (input.length() > 10) {
        // If so, than throw
        throw Overflow(input);
    }
    result = std::stoi(input);
    return result;
}

int main(int argc, char **argv)
{
	cout.sync_with_stdio(false);
	fs::path filePath = argv[0];
	string fileName = filePath.filename().string();
	
	int err, nproc, myid;
	err = MPI_Init(&argc, &argv);
	checkMPIError(err, "MPI_Init failed.");
	err = MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	checkMPIError(err, "MPI_Comm_size  failed.");
	err = MPI_Comm_rank(MPI_COMM_WORLD, &myid);
	checkMPIError(err, "MPI_Comm_rank  failed.");
	
	
	if (argc < 4)
	{
		if (myid == 0){
			
		cerr << "Usage: " << fileName
			 << " (path) <output_file> (int) <iterations> (int) <resolution>" << endl;
		}
		MPI_Finalize();
		return -1;
	}

	try {
		// Convert
		const int iterations = castInput(argv[2]);
		// This will only be shown , if there is no exception
		if (iterations <= 0)
		{
			cerr << "Please specify a positive number of iterations."<< endl;
			MPI_Finalize();
			return -2;
		}
	}
	// Catch all exceptions
	catch (const NoNumber & e) {
		cerr << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
	catch (const Overflow & e) {
		cerr << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
	// Standard exceptions for stoi
	catch (const std::invalid_argument & e) {
		cerr << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
	catch (const std::out_of_range & e) {
		cerr << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
	    // Parse and validate resolution
	try {
		const int resolution = castInput(argv[3]);
		if (resolution <= 0)
		{
			cerr << "Please specify a positive resolution." << endl;
			MPI_Finalize();
			return -3;
		}
	}
		// Catch all exceptions
	catch (const NoNumber & e) {
		cerr << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
	catch (const Overflow & e) {
		cerr << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
	// Standard exceptions for stoi
	catch (const std::invalid_argument & e) {
		cerr << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
	catch (const std::out_of_range & e) {
		cerr << e.what() << "\n";
		MPI_Finalize();
		return -2;
	}
    // Check if the output file path is valid
    if (myid == 0 && !isValidOutputPath(argv[1]))
    {
        MPI_Finalize();
        return -4;
    }
	// Call from first id
	if (myid == 0 )
	{
		cout << "Number of nodes: " << nproc << endl;
		cout << "Resolution: " << RESOLUTION << endl;
	}

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

	auto start = chrono::steady_clock::now();

	// Each process computes its portion of the image
	for (int pos = start_index; pos < end_index; pos++)
	{
		const int row = pos / WIDTH;
		const int col = pos % WIDTH;
		const complex<double> c(col * STEP + MIN_X,
								row * STEP + MIN_Y);

		complex<double> z(0, 0);
		for (int i = 1; i <= ITERATIONS; i++)
		{
			z = pow(z, 2) + c;

			// If it is convergent
			if (abs(z) >= 2)
			{
				sub_image[pos - start_index] = i;
				break;
			}
		}
	}

	// Gather results from all processes to the root process
	err = MPI_Gather(sub_image, pixels_per_process, MPI_INT, image,
			   pixels_per_process, MPI_INT, 0, MPI_COMM_WORLD);

	if (myid == 0)
	{
		auto end = chrono::steady_clock::now();
		cout << "Time elapsed: " << fixed << setprecision(2)
			 << chrono::duration<double>(end - start).count()
			 << " seconds." << endl;
		// Write the result to a file
		ofstream matrix_out;

		matrix_out.open(argv[1], ios::trunc);
		if (!matrix_out.is_open())
		{
			cout << "Unable to open file." << endl;
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