#include <omp.h>

#include <LogUtils.h>
#include <chrono>
#include <complex>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <complex>

#include <cuComplex.h>
#include <cuda.h>

// Alias to the global namespace for CUDA functions
namespace cuda
{
cudaError_t malloc(void **devPtr, size_t size)
{
	return cudaMalloc(devPtr, size);
}

cudaError_t free(void *devPtr) { return cudaFree(devPtr); }
} // namespace cuda

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
				cerr << "Please specify a positive number of "
						"threads."
					 << endl;
				exit(-4);
			}
			omp_set_num_threads(threads_used);
		}
	}
	return threads_used;
}

__device__ int devMandelbrot(int col, int row, double step,
							 int minX, int minY, int iterations)
{
	// cuDoubleComplex c =
	// 	make_cuDoubleComplex(minX + col * step, minY + row * step);
	// cuDoubleComplex z = make_cuDoubleComplex(0, 0);
	const complex<double> c(minX + col * step, minY + row * step);
	complex<double> z(0, 0);
	int count = 0;
	while (abs(z) < 2.0 && count < iterations)
	{
		// z = cuCadd(cuCmul(z, z),
		// 		   c);
		z = pow(z, 2) + c;
		count++;
	}

	return (count < iterations) ? count : 0;
}

__global__ void mandelbrotKernel(int *image, double step, int minX,
								 int minY, int iterations,
								 int WIDTH, int HEIGHT)
{
	int col = blockIdx.x * blockDim.x + threadIdx.x;
	int row = blockIdx.y * blockDim.y + threadIdx.y;

	if (col >= WIDTH || row >= HEIGHT)
		return;

	int index = row * WIDTH + col;

	image[index] =
		devMandelbrot(col, row, step, minX, minY, iterations);
}

int main(int argc, char **argv)
{
	// cout.sync_with_stdio(false);
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
	const size_t image_size = HEIGHT * WIDTH;
	// const size_t image_size = HEIGHT * WIDTH * sizeof(int);
	fill_n(image, image_size, -1);
	int *device_image;
	// cuda::malloc((void **)&device_image, HEIGHT * WIDTH *
	// sizeof(int));
	cudaError_t	err = cuda::malloc((void **)&device_image, HEIGHT * WIDTH * sizeof(int));
	
	if (err != cudaSuccess || device_image == nullptr)
	{
		cerr << "Error allocating memory on device." << endl;
		return -33;
	}
	cudaMemcpy(device_image, image, image_size,
			   cudaMemcpyHostToDevice);
	dim3 blockSize(threads_used, threads_used);
	dim3 gridSize((WIDTH + blockSize.x - 1) / blockSize.x,
				  (HEIGHT + blockSize.y - 1) / blockSize.y);

	cout << "Calculating Mandelbrot set with " << threads_used
		 << " threads with " << iterations << " iterations."
		 << endl;

	const auto start = std::chrono::steady_clock::now();
	mandelbrotKernel<<<gridSize, blockSize>>>(
		device_image, STEP, MIN_X, MIN_Y, iterations, WIDTH,
		HEIGHT);
	cudaError_t errSync = cudaGetLastError();
	cudaError_t errAsync = cudaDeviceSynchronize();
	if (errSync != cudaSuccess) {
		fprintf(stderr, "Sync kernel error: %s\n", cudaGetErrorString(errSync));
		return -1;
	}
	if (errAsync != cudaSuccess) {
		fprintf(stderr, "Async kernel error: %s\n", cudaGetErrorString(errAsync));
		return -1;
	}
	cudaMemcpy(image, device_image, image_size,
			   cudaMemcpyDeviceToHost);
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
			<< "\tProgram:\t" << fileName << "\t\tIterations:\t"
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