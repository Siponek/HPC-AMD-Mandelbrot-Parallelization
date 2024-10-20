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

} // namespace MandelbrotSet
namespace fs = std::filesystem;

using namespace std;
using namespace MandelbrotSet;

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
	const string &log_file, const string &file_name, int iterations,
	int resolution_value, int WIDTH, int HEIGHT, double STEP,
	int cuda_threads_used, chrono::duration<double> duration)
{
	ofstream log(log_file, ios::app);
	if (log.is_open())
	{
		log << "Date:\t" << __DATE__ << " " << __TIME__
			<< "\tProgram:\t" << file_name << "\t\tIterations:\t"
			<< iterations << "\tResolution:\t" << resolution_value
			<< "\tWidth:\t" << WIDTH << "\tHeight:\t" << HEIGHT
			<< "\tStep:\t" << STEP << "\tCUDA threads:\t"
			<< cuda_threads_used << "\tTime:\t" << duration.count()
			<< "\tseconds" << endl;
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
	fs::path file_path = argv[0];
	string file_name = file_path.filename().string();
	if (argc < 3)
	{
		cout << "Usage: " << file_name
			 << " <output_file> --iterations <iterations> "
				"--resolution <resolution> --threads <threads>"
			 << endl;
		return -1;
	}

	// Parse command line arguments
	cmdParse::ParsedArgs args =
		cmdParse::parse_cmd_arguments(argc, argv);
	const int iterations = args.iterations;
	const int resolution_value = args.resolution;
	fs::path output_file_path(args.output_file);
	const int cuda_threads_used = args.threads_num;

	// Image size
	const int WIDTH = static_cast<int>(RATIO_X * resolution_value);
	const int HEIGHT = static_cast<int>(RATIO_Y * resolution_value);

	const float STEP = RATIO_X / WIDTH;
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
	dim3 threads_per_block(cuda_threads_used, cuda_threads_used);
	dim3 blocks_per_grid(
		(WIDTH + threads_per_block.x - 1) / threads_per_block.x,
		(HEIGHT + threads_per_block.y - 1) / threads_per_block.y);

	cout << "Image size: " << WIDTH << "x" << HEIGHT << endl;
	cout << "Calculating Mandelbrot set with " << cuda_threads_used
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

	//? CSV
	const string additinonalName = "_cuda_";
	const string csvFile = logutils::createCsvFilename(
		output_file_path, additinonalName);
	const string header =
		"DateTime,Program,Iterations,Resolution,"
		"Width,Height,Step,CUDAThreads,Time (seconds)";
	bool has_header = logutils::csvFileHasHeader(csvFile, header);
	ofstream csv(csvFile, ios::app);
	if (csv.is_open())
	{
		if (!has_header)
		{
			cout << "Adding header to csv file." << endl;
			csv << header << endl;
		}
		csv << logutils::getCurrentTimestamp() << "," << file_name
			<< "," << iterations << "," << resolution_value << ","
			<< WIDTH << "," << HEIGHT << "," << STEP << ","
			<< cuda_threads_used << "," << duration.count() << endl;
		csv.close();
		cout << "CSV entry added successfully." << endl;
	}
	else
	{
		cerr << "Unable to open CSV file." << endl;
	}
	//? Log
	const string log_file =
		logutils::create_log_file_name(output_file_path);
	ofstream log(log_file, ios::app);
	log_execution_details(log_file, file_name, iterations,
						  resolution_value, WIDTH, HEIGHT, STEP,
						  cuda_threads_used, duration);
	//
	//? Logging the execution details
	//
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
	string new_name = to_string(cuda_threads_used) + "_threads_" +
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
	cout << "Writing to file: " << output_file_path << endl;
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