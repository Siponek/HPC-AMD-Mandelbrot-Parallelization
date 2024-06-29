$(info Running Makefile for HPC project)
# CFLAGS = -Wall -diag-disable=10441

BIN_DIR = ./bin/
OUT_DIR = ./out/
ITERATIONS = 1000
SRC_SEQ_DIR = ./src/standard-sequential/
SRC_OPENMP_DIR = ./src/openmp/
SRC_CUDA_DIR = ./src/cuda/
SRC_OPEN_MPI_DIR = ./src/open-mpi/

SRC_SEQ_FILE = $(SRC_SEQ_DIR)mandelbrot.cpp
MB = mandelbrot

LIB_LOGCPP = ./lib/LogUtils.cpp

CC = clang++
GCC = g++
CFLAGS = -Wall -Ofast -I./lib
CLANG_FLAGS = -march=znver2 -mtune=native -ffp-contract=fast -zopt -mllvm  -enable-strided-vectorization -mllvm -global-vectorize-slp=true
GCC_FLAGS = -march=znver2 -mtune=znver2


MKDIR = mkdir -p ${1}
RM = rm -rf ${1}

.PHONY: compile-all test seq clean amd-seq

compile-all: amd-seq-default seq

$(BIN_DIR):
	@$(call MKDIR,$(BIN_DIR))

$(OUT_DIR):
	@$(call MKDIR,$(OUT_DIR))

seq: $(BIN_DIR) amd-seq gcc-seq

amd-seq-default: $(BIN_DIR)
	$(CC) -wall -O1 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)seq_mandelbrot_amd_O1.exe
	$(CC) -wall -O2 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)seq_mandelbrot_amd_O2.exe
	$(CC) -wall -O3 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)seq_mandelbrot_amd_O3.exe
	$(GCC) -wall -O1 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)seq_mandelbrot_g++_O1.exe
	$(GCC) -wall -O2 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)seq_mandelbrot_g++_O2.exe
	$(GCC) -wall -O3 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)seq_mandelbrot_g++_O3.exe


amd-seq: $(BIN_DIR)
	@echo "Test"
	$(CC) $(CFLAGS) $(CLANG_FLAGS) $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)seq_mandelbrot_amd.exe

gcc-seq: $(BIN_DIR)
	$(GCC) $(CFLAGS) $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)seq_mandelbrot_g++.exe

run-amd-seq:
	$(BIN_DIR)seq_mandelbrot_amd.exe $(OUT_DIR)$(MB)_amd_seq.out $(ITERATIONS)

run-g++-seq:
	$(BIN_DIR)seq_mandelbrot_g++.exe $(OUT_DIR)$(MB)_gcc_seq.out $(ITERATIONS)

define compile_amd_openmp
	$(CC) $(CFLAGS) -fopenmp -D $(1)_SCHED $(CLANG_FLAGS) $(SRC_OPENMP_DIR)mandelbrot.cpp $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_$(1).exe
endef

define compile_g++_openmp
	$(GCC) $(CFLAGS) -fopenmp -D $(1)_SCHED $(GCC_FLAGS) $(SRC_OPENMP_DIR)mandelbrot.cpp $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_$(1).exe
endef

amd-openmp: $(BIN_DIR)
	$(CC) $(CFLAGS) -fopenmp -D DYNAMIC_SCHED $(CLANG_FLAGS) $(SRC_OPENMP_DIR)mandelbrot.cpp $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_dynamic.exe
	$(CC) $(CFLAGS) -fopenmp -D STATIC_SCHED $(CLANG_FLAGS) $(SRC_OPENMP_DIR)mandelbrot.cpp $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_static.exe
	$(CC) $(CFLAGS) -fopenmp -D GUIDED_SCHED $(CLANG_FLAGS) $(SRC_OPENMP_DIR)mandelbrot.cpp $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_guided.exe
	$(CC) $(CFLAGS) -fopenmp -D RUNTIME_SCHED $(CLANG_FLAGS) $(SRC_OPENMP_DIR)mandelbrot.cpp $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_runtime.exe

run-amd-openmp:
	$(BIN_DIR)$(MB)_amd_dynamic.exe $(OUT_DIR)$(MB)_amd_openmp_dynamic.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_amd_static.exe $(OUT_DIR)$(MB)_amd_openmp_static.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_amd_guided.exe $(OUT_DIR)$(MB)_amd_openmp_guided.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_amd_runtime.exe $(OUT_DIR)$(MB)_amd_openmp_runtime.out $(ITERATIONS)

g++-openmp: $(BIN_DIR)
	$(GCC) $(CFLAGS) -fopenmp -D DYNAMIC_SCHED $(GCC_FLAGS) $(SRC_OPENMP_DIR)mandelbrot.cpp $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_dynamic.exe
	$(GCC) $(CFLAGS) -fopenmp -D STATIC_SCHED $(GCC_FLAGS) $(SRC_OPENMP_DIR)mandelbrot.cpp $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_static.exe
	$(GCC) $(CFLAGS) -fopenmp -D GUIDED_SCHED $(GCC_FLAGS) $(SRC_OPENMP_DIR)mandelbrot.cpp $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_guided.exe
	$(GCC) $(CFLAGS) -fopenmp -D RUNTIME_SCHED $(GCC_FLAGS) $(SRC_OPENMP_DIR)mandelbrot.cpp $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_runtime.exe

run-g++-openmp:
	$(BIN_DIR)$(MB)_g++_dynamic.exe $(OUT_DIR)$(MB)_g++_openmp_dynamic.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_g++_static.exe $(OUT_DIR)$(MB)_g++_openmp_static.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_g++_guided.exe $(OUT_DIR)$(MB)_g++_openmp_guided.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_g++_runtime.exe $(OUT_DIR)$(MB)_g++_openmp_runtime.out $(ITERATIONS)

clean:
	@echo "Cleaning up..."
	@$(call RM,$(BIN_DIR)) $(call RM,$(OUT_DIR))
