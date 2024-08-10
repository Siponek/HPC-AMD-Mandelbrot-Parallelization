#include <omp.h>

#include <LogUtils.h>
#include <chrono>
#include <complex>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

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

__host__ int get_threads_used(int argc, char **argv)
{
	int _threads_used = omp_get_max_threads();
	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		if (arg == "-threads" && i + 1 < argc)
		{
			_threads_used = stoi(argv[++i]);
			if (_threads_used <= 0)
			{
				cerr << "Please specify a positive number of "
						"threads."
					 << endl;
				exit(-4);
			}
			omp_set_num_threads(_threads_used);
		}
	}
	return _threads_used;
}

__host__ void check_cuda_errors(cudaError_t err_sync,
								cudaError_t err_async)
{
	if (err_sync != cudaSuccess)
	{
		cerr << "Sync kernel error: "
			 << cudaGetErrorString(err_sync) << endl;
		exit(-1);
	}
	if (err_async != cudaSuccess)
	{
		cerr << "Async kernel error: "
			 << cudaGetErrorString(err_async) << endl;
		exit(-1);
	}
}

__host__ void log_execution_details(
	const string &logFile, const string &file_name, int iterations,
	int RESOLUTION_VALUE, int WIDTH, int HEIGHT, double STEP,
	int _threads_used, chrono::duration<double> duration)
{
	ofstream log(logFile, ios::app);
	if (log.is_open())
	{
		log << "Date:\t" << __DATE__ << " " << __TIME__
			<< "\tProgram:\t" << file_name << "\t\tIterations:\t"
			<< iterations << "\tResolution:\t" << RESOLUTION_VALUE
			<< "\tWidth:\t" << WIDTH << "\tHeight:\t" << HEIGHT
			<< "\tStep:\t" << STEP << "\tScheduling:\t"
			<< SCHEDULING_STRING << "\tThreads:\t" << _threads_used
			<< "\tTime:\t" << duration.count() << "\tseconds"
			<< endl;
		log.close();
	}
	else
	{
		cerr << "Unable to open log file." << endl;
	}
}

__device__ int dev_Mandelbrot_kernel(int collumn, int row,
									 double step, int minX,
									 int minY, int iterations)
{
	int count = 0;

	cuDoubleComplex c = make_cuDoubleComplex(minX + collumn * step,
											 minY + row * step);
	cuDoubleComplex z = make_cuDoubleComplex(0, 0);
	while (cuCabs(z) < 2.0 && count < iterations)
	{
		z = cuCadd(cuCmul(z, z), c);
		count++;
	}
	// const complex<double> c(minX + collumn * step, minY + row *
	// step); complex<double> z(0, 0); while (abs(z) < 2.0 && count
	// < iterations)
	// {
	// 	z = pow(z, 2) + c;
	// 	count++;
	// }

	return (count < iterations) ? count : 0;
}

__global__ void mandelbrotKernel(int *image, double step, int minX,
								 int minY, int iterations,
								 int WIDTH, int HEIGHT)
{
	// idiomatic CUDA loop
	int col = blockIdx.x * blockDim.x + threadIdx.x;
	int row = blockIdx.y * blockDim.y + threadIdx.y;
	if (col >= WIDTH || row >= HEIGHT)
		return;

	int index = row * WIDTH + col;
	image[index] = dev_Mandelbrot_kernel(col, row, step, minX, minY,
										 iterations);
}

int main(int argc, char **argv)
{
	// cout.sync_with_stdio(false);
	fs::path file_path = argv[0];
	string file_name = file_path.filename().string();
	if (argc < 3)
	{
		cout << "Usage: " << file_name
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
	const int _threads_used = get_threads_used(argc, argv);
	const size_t image_size = HEIGHT * WIDTH;
	unique_ptr<int[]> image(new int[image_size]);

	fill_n(image.get(), image_size, -1);
	int *device_image;
	size_t free_mem, total_mem;
	cudaMemGetInfo(&free_mem, &total_mem);

	size_t required_mem = HEIGHT * WIDTH * sizeof(int);
	if (required_mem > free_mem)
	{
		cerr << "Error: Not enough memory on the device." << endl;
		return -1;
	}
	if (cuda::malloc((void **)&device_image, required_mem) !=
		cudaSuccess)
	{
		cerr << "Error allocating memory on device." << endl;
		return -33;
	}

	cudaMemcpy(device_image, image.get(), image_size * sizeof(int),
			   cudaMemcpyHostToDevice);
	dim3 threads_per_block(16, 16);
	dim3 blocks_per_grid(
		(WIDTH + threads_per_block.x - 1) / threads_per_block.x,
		(HEIGHT + threads_per_block.y - 1) / threads_per_block.y);

	cout << "Image size: " << WIDTH << "x" << HEIGHT << endl;
	cout << "Calculating Mandelbrot set with " << _threads_used
		 << " threads with " << iterations << " iterations." << endl
		 << "blocksize: " << blocks_per_grid.x << " "
		 << blocks_per_grid.y
		 << " threads_per_block: " << threads_per_block.x << " "
		 << threads_per_block.y << endl;

	const auto start = std::chrono::steady_clock::now();
	mandelbrotKernel<<<blocks_per_grid, threads_per_block>>>(
		device_image, STEP, MIN_X, MIN_Y, iterations, WIDTH,
		HEIGHT);
	cudaError_t err_sync = cudaGetLastError();
	cudaError_t err_async = cudaDeviceSynchronize();
	check_cuda_errors(cudaGetLastError(), cudaDeviceSynchronize());
	cudaMemcpy(image.get(), device_image, image_size * sizeof(int),
			   cudaMemcpyDeviceToHost);

	const auto end = std::chrono::steady_clock::now();
	cuda::free(device_image);
	if (any_of(image.get(), image.get() + image_size,
			   [](int val) { return val == -1; }))
	{
		cerr << "Error: Not all pixels were calculated." << endl;
		return -3;
	}

	chrono::duration<double> duration = end - start;
	cout << endl
		 << "Time elapsed: " << duration.count() << " seconds."
		 << endl;

	const string log_file =
		logutils::createLogFileName(argv[1], "_openmp_");
	ofstream log(log_file, ios::app);
	log_execution_details(log_file, file_name, iterations,
						  RESOLUTION_VALUE, WIDTH, HEIGHT, STEP,
						  _threads_used, duration);
	//
	//? Logging the execution details
	//
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
	image.reset(); // It's here for coding style, but useless
	// delete[] image; // It's here for coding style, but useless
	return 0;
}