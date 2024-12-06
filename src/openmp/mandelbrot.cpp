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

void computeMandelbrot(int *image, int _iterations, int _WIDTH,
					   int _HEIGHT, float _STEP)
{
	// region provided by *image is shared among threads, the
	// pointer is private
#pragma omp parallel for schedule(SCHEDULING_TYPE) default(none)   \
	firstprivate(image, _iterations)                               \
	shared(_WIDTH, _HEIGHT, _STEP)
	for (int pos = 0; pos < _HEIGHT * _WIDTH; pos++)
	{
		image[pos] = 0;
		const int row = pos / _WIDTH;
		const int col = pos % _WIDTH;
		const complex<double> c(col * _STEP + MIN_X,
								row * _STEP + MIN_Y);

		// z = z^2 + c
		complex<double> z(0, 0);
		for (int i = 1; i <= _iterations; i++)
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
}

int main(int argc, char **argv)
{
	cout.sync_with_stdio(false);
	fs::path filePath = argv[0];
	string fileName = filePath.filename().string();
	// Parse command line arguments
	cmdParse::ParsedArgs args =
		cmdParse::parse_cmd_arguments(argc, argv);
	const int iterations = args.iterations;
	const int resolution_value = args.resolution;
	fs::path output_file_path(args.output_file);
	int threads_used = omp_get_max_threads();
	if (args.threads_num > 0)
	{
		threads_used = args.threads_num;
		omp_set_num_threads(threads_used);
	}

	// Image size
	const int WIDTH =
		static_cast<int>(MandelbrotSet::RATIO_X * resolution_value);
	const int HEIGHT =
		static_cast<int>(MandelbrotSet::RATIO_Y * resolution_value);
	const float STEP = MandelbrotSet::RATIO_X / WIDTH;

	int *const image = new int[HEIGHT * WIDTH];
	const size_t image_size = HEIGHT * WIDTH;
	fill_n(image, image_size, -1);
	cout << "Calculating Mandelbrot set with " << threads_used
		 << " threads with " << iterations << " iterations."
		 << endl;

	const auto start = std::chrono::steady_clock::now();
	computeMandelbrot(image, iterations, WIDTH, HEIGHT, STEP);
	const auto end = std::chrono::steady_clock::now();

	chrono::duration<double> duration = end - start;
	cout << "Time elapsed: " << duration.count() << " seconds."
		 << endl;
	//? CSV
	const string scheduling_type = SCHEDULING_STRING;
	const string additinonalName = "_openmp_";

	const string csvFile =
		logutils::createCsvFilename(argv[1], additinonalName);
	const string header =
		"DateTime,Program,Iterations,Resolution,Width,Height,Step,"
		"Scheduling,Threads,Time (seconds)";
	bool has_header = logutils::csvFileHasHeader(csvFile, header);
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
			<< WIDTH << "," << HEIGHT << "," << STEP << ","
			<< SCHEDULING_STRING << "," << threads_used << ","
			<< duration.count() << endl;
		csv.close();
		cout << "CSV entry added successfully." << endl;
	}
	else
	{
		cerr << "Unable to open CSV file." << endl;
	}
	//? log
	const string log_file =
		logutils::create_log_file_name(argv[1], additinonalName);
	ofstream log(log_file, ios::app);
	if (log.is_open())
	{
		log << "Date:\t" << __DATE__ << " " << __TIME__
			<< "\tProgram:\t" << fileName << "\t\tIterations:\t"
			<< iterations << "\tResolution:\t" << resolution_value
			<< "\tWidth:\t" << WIDTH << "\tHeight:\t" << HEIGHT
			<< "\tStep:\t" << STEP << "\tScheduling:\t"
			<< SCHEDULING_STRING << "\tThreads:\t" << threads_used
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
	string new_name = to_string(threads_used) + "_threads_" +
					  to_string(iterations) + "_iterations_" +
					  to_string(resolution_value) + "_resolution";
	// Get the original filename stem and extension
	string filename_stem = output_file_path.stem().string();
	string extension = output_file_path.extension().string();
	string new_filename =
		filename_stem + "_" + new_name + extension;
	// Update the output_file_path with the new filename
	output_file_path =
		output_file_path.parent_path() / new_filename;
	ofstream matrix_out(output_file_path, ios::trunc);
	cout << "Writing to file: " << output_file_path << endl << endl;
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