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

#ifndef RESOLUTION
#define RESOLUTION 1000
#endif
constexpr int RESOLUTION_VALUE = (int)RESOLUTION;
// Image size
constexpr int WIDTH = static_cast<int>(RATIO_X * RESOLUTION_VALUE);
constexpr int HEIGHT = static_cast<int>(RATIO_Y * RESOLUTION_VALUE);

constexpr float STEP = RATIO_X / WIDTH;
} // namespace MandelbrotSet
namespace fs = std::filesystem;

using namespace std;
using namespace MandelbrotSet;

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

	int iterations = stoi(argv[2]);
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
	for (int pos = 0; pos < HEIGHT * WIDTH; pos++)
	{
		image[pos] = 0;
		const int row = pos / WIDTH;
		const int col = pos % WIDTH;
		const complex<double> c(col * STEP + MIN_X,
								row * STEP + MIN_Y);

		// z = z^2 + c
		complex<double> z(0, 0);
		for (int i = 1; i <= iterations; i++)
		{
			z = pow(z, 2) + c;
			// If it is convergent
			if (abs(z) >= 2)
			{
				image[pos] = i;
				break;
			}
		}
	}
	const auto end = chrono::steady_clock::now();
	const string csvFile = logutils::createCsvFilename(argv[1]);
	const string header =
		"Date,Time,Program,Iterations,Resolution,Width,Height,Step,"
		"Scheduling,Time (seconds)";
	bool hasHeader = logutils::csvFileHasHeader(csvFile, header);

	chrono::duration<double> duration = end - start;
	cout << endl
		 << "Time elapsed: " << duration.count() << " seconds."
		 << endl;

	const string logFile =
		logutils::createLogFileName(argv[1], "_seq_");
	ofstream csv(csvFile, ios::app);
	if (csv.is_open())
	{
		if (!hasHeader)
		{
			cout << "Adding header to csv file." << endl;
			csv << header << endl;
		}
		csv << __DATE__ << "," << __TIME__ << "," << fileName << ","
			<< iterations << "," << RESOLUTION_VALUE << "," << WIDTH
			<< "," << HEIGHT << "," << STEP << "," << "" << ","
			<< duration.count() << endl;
		csv.close();
		cout << "Log entry added successfully." << endl;
	}
	else
	{
		cerr << "Unable to open CSV file." << endl;
	}
	ofstream log(logFile, ios::app);
	if (log.is_open())
	{
		log << "Date:\t" << __DATE__ << " " << __TIME__
			<< "\tProgram:\t" << fileName << "\t\tIterations:\t"
			<< iterations << "\tResolution:\t" << RESOLUTION_VALUE
			<< "\tWidth:\t" << WIDTH << "\tHeight:\t" << HEIGHT
			<< "\tStep:\t" << STEP << "\tScheduling:\t"
			<< "\tTime:\t" << duration.count() << "\tseconds"
			<< endl;
		log.close();
	}
	else
	{
		cerr << "Unable to open log file." << endl;
	}

	const fs::path outputFilePath(argv[1]);
	try
	{
		fs::create_directories(outputFilePath.parent_path());
	}
	catch (const fs::filesystem_error &e)
	{
		cout << "Error creating directories: " << e.what() << endl;
		return -13;
	}
	// Write the result to a file
	ofstream matrix_out(outputFilePath, ios::trunc);
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