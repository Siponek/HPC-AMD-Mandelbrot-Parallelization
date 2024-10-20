#include <LogUtils.h>
#include <chrono>
#include <complex>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace MandelbrotSet
{
// Ranges of the set
constexpr float MIN_X = -2.0;
constexpr float MAX_X = 1.0;
constexpr float MIN_Y = -1.0;
constexpr float MAX_Y = 1.0;

// Image ratio
constexpr float RATIO_X = (MAX_X - MIN_X);
constexpr float RATIO_Y = (MAX_Y - MIN_Y);

} // namespace MandelbrotSet
namespace fs = std::filesystem;

using namespace std;

int main(int argc, char **argv)
{
	cout.sync_with_stdio(false);
	fs::path filePath = argv[0];
	string fileName = filePath.filename().string();
	if (argc < 3)
	{
		cout << "Usage: " << fileName
			 << " <output_file> <iterations>" << endl;
		return -1;
	}
	if (argc < 2)
	{
		cout << "Please specify the output file as a parameter."
			 << endl;
		return -1;
	}
	// Parse command line arguments
	cmdParse::ParsedArgs args =
		cmdParse::parse_cmd_arguments(argc, argv);
	const int iterations = args.iterations;
	const int resolution_value = args.resolution;
	const fs::path output_file_path(args.output_file);

	// Image size
	const int WIDTH =
		static_cast<int>(MandelbrotSet::RATIO_X * resolution_value);
	const int HEIGHT =
		static_cast<int>(MandelbrotSet::RATIO_Y * resolution_value);

	const float STEP = MandelbrotSet::RATIO_X / WIDTH;

	if (iterations <= 0)
	{
		cout << "Please specify a positive number of iterations."
			 << endl;
		return -2;
	}

	int *const image = new int[HEIGHT * WIDTH];
	cout << "Calculating Mandelbrot set with " << iterations
		 << " iterations." << endl;
	const auto start = chrono::steady_clock::now();

	//! Calculate the Mandelbrot set
	int row = 0, col = 0;
	for (int pos = 0; pos < HEIGHT * WIDTH; pos++)
	{
		image[pos] = 0;
		row = pos / WIDTH;
		col = pos % WIDTH;
		const complex<double> c(col * STEP + MandelbrotSet::MIN_X,
								row * STEP + MandelbrotSet::MIN_Y);

		// z = z^2 + c
		complex<double> z(0, 0);
		for (int i = 1; i <= iterations; i++)
		{
			z = z * z + c;
			// If it is convergent
			if (abs(z) >= 2)
			{
				image[pos] = i;
				break;
			}
		}
	}
	const auto end = chrono::steady_clock::now();
	const string csvFile =
		logutils::createCsvFilename(argv[1], "_seq_");
	const string header = "DateTime,Program,Iterations,"
						  "Resolution,Width,Height,Step,"
						  "Scheduling,Time (seconds)";
	bool has_header = logutils::csvFileHasHeader(csvFile, header);

	chrono::duration<double> duration = end - start;
	cout << endl
		 << "Time elapsed: " << duration.count() << " seconds."
		 << endl;

	const string log_file =
		logutils::create_log_file_name(argv[1], "_seq_");
	ofstream csv(csvFile, ios::app);
	if (csv.is_open())
	{
		if (!has_header)
		{
			cout << "Adding header to csv file." << endl;
			csv << header << endl;
		}
		csv << logutils::getCurrentTimestamp() << "," << fileName
			<< "," << iterations << "," << resolution_value << ","
			<< WIDTH << "," << HEIGHT << "," << STEP << "," << ""
			<< "," << duration.count() << endl;
		csv.close();
		cout << "CSV entry added successfully." << endl;
	}
	else
	{
		cerr << "Unable to open CSV file." << endl;
	}
	ofstream log(log_file, ios::app);
	if (log.is_open())
	{
		log << "Date:\t" << __DATE__ << " " << __TIME__
			<< "\tProgram:\t" << fileName << "\t\tIterations:\t"
			<< iterations << "\tResolution:\t" << resolution_value
			<< "\tWidth:\t" << WIDTH << "\tHeight:\t" << HEIGHT
			<< "\tStep:\t" << STEP << "\tScheduling:\t"
			<< "\tTime:\t" << duration.count() << "\tseconds"
			<< endl;
		log.close();
		cout << "Log entry added successfully." << endl;
	}
	else
	{
		cerr << "Unable to open log file." << endl;
	}

	try
	{
		fs::create_directories(output_file_path.parent_path());
	}
	catch (const fs::filesystem_error &e)
	{
		cout << "Error creating directories: " << e.what() << endl;
		return -13;
	}
	// Write the result to a file
	ofstream matrix_out(output_file_path, ios::trunc);
	cout << "Writing to file: " << argv[1] << endl;
	if (!matrix_out.is_open())
	{
		cout << "Unable to open file." << endl;
		return -14;
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

	delete[] image; // It's here for coding style, but useless
	return 0;
}