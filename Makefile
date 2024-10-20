$(info Running Makefile for HPC project)
# CFLAGS = -Wall -diag-disable=10441

BIN_DIR = ./bin/
OUT_DIR = ./out/
SRC_SEQ_DIR = ./src/standard-sequential/
SRC_OPENMP_DIR = ./src/openmp/
SRC_CUDA_DIR = ./src/cuda/
SRC_OPEN_MPI_DIR = ./src/mpi/

SRC_SEQ_FILE = $(SRC_SEQ_DIR)mandelbrot.cpp
MB = mandelbrot

LIB_LOGCPP = ./lib/LogUtils.cpp

CC = clang++
GCC = g++
NVC = nvc++
MPICC = mpiicc
CFLAGS = -Wall -I./lib
CLANG_FLAGS =-Ofast -march=znver2 -mtune=znver2 #-g
CLANG_FLAGS_EXT = -ffp-contract=fast -zopt -mllvm  -enable-strided-vectorization -mllvm -global-vectorize-slp=true
GCC_FLAGS = -Ofast -march=znver2 -mtune=znver2
# Unified uses the same memory for CPU and GPU
#  -mp is for OpenMP
NVC_FLAGS = -tp=znver2 -fast -O4 -Xcompiler -Wall -I./lib --c++23 #-gpu=mem:unified -mp
# NVC_FLAGS = -fast -O4 -Minfo -Xcompiler -Wall -I./lib #-gpu=mem:unified -mp
MPI_FLAGS = -std=c++17 -Ofast -march=native -xHost -Wall -qopt-report=5 -qopt-report-phase=vec -qopt-report-file=optimization_report.txt


MKDIR = mkdir -p ${1}
RM = rm -rf ${1}

ITERATIONS := 1000 2000 4000
RESOLUTIONS := 1000 2000 4000 8000
THREAD_COUNTS := 2 4 8 16
CUDA_THREADS := 2 4 8 16 32
SCHEDULERS := DYNAMIC STATIC GUIDED RUNTIME

$(BIN_DIR):
	@$(call MKDIR,$(BIN_DIR))

$(OUT_DIR):
	@$(call MKDIR,$(OUT_DIR))

seq: $(BIN_DIR) amd-seq gcc-seq
#! SEQ
amd-seq-default: $(BIN_DIR)
	$(CC) $(CFLAGS) -O1 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_seq_O1.exe
	$(CC) $(CFLAGS) -O2 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_seq_O2.exe
	$(CC) $(CFLAGS) -O3 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_seq_O3.exe
	$(GCC) $(CFLAGS) -O1 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_seq_O1.exe
	$(GCC) $(CFLAGS) -O2 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_seq_O2.exe
	$(GCC) $(CFLAGS) -O3 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_seq_O3.exe

#! SEQ TUNED
amd-seq: $(BIN_DIR)
	$(CC) $(CFLAGS) $(CLANG_FLAGS) $(CLANG_FLAGS_EXT) $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_seq.exe

gcc-seq: $(BIN_DIR)
	$(GCC) $(CFLAGS) $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_seq.exe

run-amd-seq:
	@for res in $(RESOLUTIONS); do \
		for iter in $(ITERATIONS); do \
			echo "Running AMD SEQ with $$res resolution and $$iter iterations"; \
			$(BIN_DIR)$(MB)_amd_seq.exe $(OUT_DIR)$(MB)_amd_seq_$$res.out --iterations $$iter --resolution $$res; \
		done \
	done

run-g++-seq:
	@for res in $(RESOLUTIONS); do \
		for iter in $(ITERATIONS); do \
			echo "Running G++ SEQ with $$res resolution and $$iter iterations"; \
			$(BIN_DIR)$(MB)_g++_seq.exe $(OUT_DIR)$(MB)_g++_seq_$$res.out --iterations $$iter --resolution $$res; \
		done \
	done

run-amd-seq-full: amd-seq run-amd-seq
run-g++-seq-full: gcc-seq run-g++-seq



# ! OpenMP
define compile_amd_openmp_ext
	$(CC) $(CFLAGS) -fopenmp -D $(1)_SCHED $(CLANG_FLAGS) $(CLANG_FLAGS_EXT) $(SRC_OPENMP_DIR)mandelbrot.cpp $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_ext_$(1).exe
endef

define compile_amd_openmp
	$(CC) $(CFLAGS) -fopenmp -D $(1)_SCHED $(CLANG_FLAGS) $(SRC_OPENMP_DIR)mandelbrot.cpp $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_$(1).exe
endef

define compile_g++_openmp
	$(GCC) $(CFLAGS) -fopenmp -D $(1)_SCHED $(GCC_FLAGS) $(SRC_OPENMP_DIR)mandelbrot.cpp $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_$(1).exe
endef

amd-openmp-ext: $(BIN_DIR)
	@for sched in $(SCHEDULERS); do \
		echo "Compiling AMD OpenMP with extended flags for sched $$sched"; \
		$(call compile_amd_openmp_ext,"$$sched"); \
	done
	echo "AMD OpenMP with extended flags compiled."


run-amd-openmp-ext:
	echo "Running AMD OpenMP with extended flags..."
	# echo "Running AMD OpenMP with extended flags with one thread..."
	# 	for res in $(RESOLUTIONS); do \
	# 		for iter in $(ITERATIONS); do \
	# 			exe=$(BIN_DIR)$(MB)_amd_ext_GUIDED.exe; \
	# 			out=$(OUT_DIR)$(MB)_amd_ext_openmp.out; \
	# 			echo "Running $$exe with scheduler GUIDED, 1 thread, resolution $$res, iterations $$iter "; \
	# 			$$exe $$out --iterations $$iter --resolution $$res --threads 1; \
	# 		done \
	# 	done \
	echo "Running AMD OpenMP with extended flags with multiple threads..."
	@for sched in $(SCHEDULERS); do \
		for threads in $(THREAD_COUNTS); do \
			for res in $(RESOLUTIONS); do \
				for iter in $(ITERATIONS); do \
					exe=$(BIN_DIR)$(MB)_amd_ext_$$sched.exe; \
					out=$(OUT_DIR)$(MB)_amd_ext_openmp.out; \
					echo "Running $$exe with scheduler $$sched, $$threads threads, resolution $$res, iterations $$iter "; \
					$$exe $$out --iterations $$iter --resolution $$res --threads "$$threads"; \
				done \
			done \
		done \
	done

run-amd-openmp-ext-full: amd-openmp-ext run-amd-openmp-ext

amd-openmp: $(BIN_DIR)
	@for sched in $(SCHEDULERS); do \
		echo "Compiling AMD OpenMP for sched $$sched"; \
		$(call compile_amd_openmp,"$$sched"); \
	done
	echo "AMD OpenMP compiled."



run-amd-openmp:
	echo "Running AMD OpenMP with extended flags..."
	@for sched in $(SCHEDULERS); do \
		for threads in $(THREAD_COUNTS); do \
			for res in $(RESOLUTIONS); do \
				for iter in $(ITERATIONS); do \
					exe=$(BIN_DIR)$(MB)_amd_$$sched.exe; \
					out=$(OUT_DIR)$(MB)_amd_openmp_$($$sched)_threads$($$threads).out; \
					echo "Running $$exe with $$threads threads and scheduler $$sched"; \
					$$exe $$out --iterations $$iter --resolution $$res --threads "$$threads"; \
				done \
			done \
		done \
	done



g++-openmp: $(BIN_DIR)
	@for sched in $(SCHEDULERS); do \
		$(call compile_g++_openmp,$$sched); \
	done


run-g++-openmp:
	echo "Running G++ OpenMP"
	@for sched in $(SCHEDULERS); do \
		for threads in $(THREAD_COUNTS); do \
			for iter in $(ITERATIONS); do \
				exe=$(BIN_DIR)$(MB)_g++_$$sched.exe; \
				out=$(OUT_DIR)$(MB)_g++_openmp_$$sched_threads$$threads.out; \
				echo "Running $$exe with $$threads threads and scheduler $$sched"; \
				$$exe $(OUT_DIR)$(MB)_g++_openmp_$$sched_threads$$threads.out $$iter --threads "$$threads"; \
			done \
		done \
	done

.PHONY: run-amd-openmp-full
run-amd-openmp-full: amd-openmp run-amd-openmp

.PHONY: run-custom-amd-ext
run-custom-amd-ext: $(BIN_DIR)
	@for threads in 4 8 16; do \
		for res in 1000 2000 4000 8000; do \
			for iter in 1000 2000 4000; do \
				exe=$(BIN_DIR)$(MB)_amd_ext_RUNTIME.exe; \
				out=$(OUT_DIR)$(MB)_amd_ext_openmp.out; \
				echo "Running $$exe with scheduler RUNTIME, $$threads threads, resolution $$res, iterations $$iter "; \
				$$exe $$out --iterations $$iter --resolution $$res --threads $$threads; \
			done \
		done \
	done


#! CUDA
cuda: $(BIN_DIR)
	$(NVC) $(NVC_FLAGS) -g $(SRC_CUDA_DIR)mandelbrot.cu $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_cuda.exe
# $(NVC) $(NVC_FLAGS) $(SRC_CUDA_DIR)mandelbrot.cu $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_cuda.exe

run-cuda:
	@for threads in $(CUDA_THREADS); do \
		for res in $(RESOLUTIONS); do \
			for iter in $(ITERATIONS); do \
				echo "Running CUDA with $$threads threads $$res resolution $$iter iterations"; \
				$(BIN_DIR)$(MB)_cuda.exe $(OUT_DIR)$(MB)_cuda.out --iterations $$iter --resolution $$res --threads $$threads; \
			done \
		done \
	done

.PHONY: compile-all-openmp
compile-all-openmp: amd-openmp-ext amd-openmp g++-openmp


.PHONY: compile-all
compile-all: amd-seq-default seq compile-all-openmp cuda


.PHONY: run-all-openmp
run-all-openmp: run-amd-openmp-ext run-amd-openmp run-g++-openmp


.PHONY: run-all
run-all: run-amd-seq run-g++-seq run-amd-openmp-ext run-amd-openmp run-g++-openmp run-cuda


clean:
	@echo "Cleaning up..."
	@$(call RM,$(BIN_DIR)) $(call RM,$(OUT_DIR))

# ! OpenMPI on OCAPIE server
.PHONY: setup-mpi
setup-mpi:
	@echo "Setting up the environment..."
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OUT_DIR)

.PHONY: compile-mpi
compile-mpi:
	$(MPICC) $(MPI_FLAGS) $(SRC_OPEN_MPI_DIR)mandelbrot.cpp -o $(BIN_DIR)mandelbrot_mpi.exe

.PHONY: run-mpi
run-mpi: compile-mpi
	@echo "MPI mandelbrot binary compiled."
	mpiexec -hostfile ./machinefile.txt -perhost 1 -np 7 $(BIN_DIR)mandelbrot_mpi.exe $(OUT_DIR)mandelbrot_mpi.out

# TODO make a bsub job submissionn

.PHONY: benchmark
benchmark: compile-mpi
	@echo ""
	@for nodes in 16 32 64 128 256 512 1024 2048 3072; do \
		for resolution in 1000 2000 4000 8000; do \
			for iter in $(ITERATIONS); do \
				echo "Running MPI benchmark with $$nodes nodes, $$resolution resolution $$iter iterations"; \
				mpiexec -hostfile ./machinefile.txt -perhost 1 -np $$nodes $(BIN_DIR)mandelbrot_mpi.exe $(OUT_DIR)/mandelbrot_mpi_$$nodes_$$resolution.out $$iter $$resolution; \
			done \
		done \
	done
	@echo "MPI benchmark completed"

