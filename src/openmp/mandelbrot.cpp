#include <omp.h>

#include <LogUtils.h>
#include <chrono>
#include <complex>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
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

#define SCHEDULING_STRING "RUNTIME"
#define SCHEDULING_TYPE runtime
#ifdef DYNAMIC_SCHED
#undef SCHEDULING_STRING
#undef SCHEDULING_TYPE
#define SCHEDULING_TYPE dynamic
#define SCHEDULING_STRING "DYNAMIC"
#elif defined(STATIC_SCHED)
#undef SCHEDULING_STRING
#undef SCHEDULING_TYPE
#define SCHEDULING_TYPE static
#define SCHEDULING_STRING "STATIC"
#elif defined(GUIDED_SCHED)
#undef SCHEDULING_STRING
#undef SCHEDULING_TYPE
#define SCHEDULING_TYPE guided
#define SCHEDULING_STRING "GUIDED"
#endif

using namespace std;
using namespace MandelbrotSet;

string createLogFileName(const string &outputFile,
						 const string &additionalName = "")
{
	const fs::path outputPath(outputFile);
	std::string filename = outputPath.stem().string();
	size_t underscorePos = filename.find('_');
	underscorePos = filename.find('_', underscorePos + 2);
	if (underscorePos != string::npos)
	{
		filename = filename.substr(0, underscorePos);
	}
	const fs::path logFilePath =
		// out/..
		outputPath.parent_path().parent_path() / "logs" /
		(filename + additionalName + ".log");
	fs::create_directories(logFilePath.parent_path());
	return logFilePath.string();
}

int getThreadsUsed(int argc, char **argv)
{
	int threads_used = omp_get_max_threads();
	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (arg == "-threads" && i + 1 < argc)
		{
			threads_used = stoi(argv[++i]);
			if (threads_used <= 0)
			{
				cout << "Please specify a positive number of "
						"threads."
					 << endl;
				return -4;
			}
			omp_set_num_threads(threads_used);
		}
	}
	return threads_used;
}

void computeMandelbrot(int *image, int iterations)
{
#pragma omp parallel for schedule(SCHEDULING_TYPE) default(none) shared(image) firstprivate(iterations) 
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
}

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
	const int iterations = stoi(argv[2]);
	if (iterations <= 0)
	{
		cout << "Please specify a positive number of iterations."
			 << endl;
		return -2;
	}
	const int threads_used = getThreadsUsed(argc, argv);
	int *const image = new int[HEIGHT * WIDTH];
	cout << "Calculating Mandelbrot set with " << threads_used
		 << " threads with " << iterations << " iterations."
		 << endl;

	const auto start = std::chrono::steady_clock::now();
	computeMandelbrot(image, iterations);
	const auto end = std::chrono::steady_clock::now();

	chrono::duration<double> duration = end - start;
	cout << endl
		 << "Time elapsed: " << duration.count() << " seconds."
		 << endl;

	const string logFile =
		logutils::createLogFileName(argv[1], "_openmp_");
	ofstream log(logFile, ios::app);
	if (log.is_open())
	{
		log << "Date:\t" << __DATE__ << " " << __TIME__
			<< "\tProgram:\t" << fileName << "\tIterations:\t"
			<< iterations << "\tResolution:\t" << RESOLUTION_VALUE
			<< "\tWidth:\t" << WIDTH << "\tHeight:\t" << HEIGHT
			<< "\tStep:\t" << STEP << "\tScheduling:\t"
			<< SCHEDULING_STRING << "\tThreads:\t" << threads_used
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