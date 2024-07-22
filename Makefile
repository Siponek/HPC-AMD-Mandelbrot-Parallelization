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
NVC = nvc++
CFLAGS = -Wall -I./lib
CLANG_FLAGS =-Ofast -march=znver2 -mtune=znver2 -g
CLANG_FLAGS_EXT = -ffp-contract=fast -zopt -mllvm  -enable-strided-vectorization -mllvm -global-vectorize-slp=true
GCC_FLAGS = -Ofast -march=znver2 -mtune=znver2
# Unified uses the same memory for CPU and GPU
#  -mp is for OpenMP
NVC_FLAGS = -fast -O4 -Xcompiler -Wall -I./lib #-gpu=mem:unified -mp
# NVC_FLAGS = -fast -O4 -Minfo -Xcompiler -Wall -I./lib #-gpu=mem:unified -mp

MKDIR = mkdir -p ${1}
RM = rm -rf ${1}

.PHONY: compile-all seq clean amd-seq

compile-all: amd-seq-default seq

$(BIN_DIR):
	@$(call MKDIR,$(BIN_DIR))

$(OUT_DIR):
	@$(call MKDIR,$(OUT_DIR))

seq: $(BIN_DIR) amd-seq gcc-seq

amd-seq-default: $(BIN_DIR)
	$(CC) -wall -O1 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_seq_O1.exe
	$(CC) -wall -O2 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_seq_O2.exe
	$(CC) -wall -O3 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_seq_O3.exe
	$(GCC) -wall -O1 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_seq_O1.exe
	$(GCC) -wall -O2 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_seq_O2.exe
	$(GCC) -wall -O3 $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_seq_O3.exe


amd-seq: $(BIN_DIR)
	$(CC) $(CFLAGS) $(CLANG_FLAGS) $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_amd_seq.exe

gcc-seq: $(BIN_DIR)
	$(GCC) $(CFLAGS) $(SRC_SEQ_FILE) $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_g++_seq.exe

run-amd-seq:
	$(BIN_DIR)$(MB)_amd_seq.exe $(OUT_DIR)$(MB)_amd_seq.out $(ITERATIONS)

run-g++-seq:
	$(BIN_DIR)$(MB)_g++_seq.exe $(OUT_DIR)$(MB)_gcc_seq.out $(ITERATIONS)



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
	$(call compile_amd_openmp_ext,DYNAMIC)
	$(call compile_amd_openmp_ext,STATIC)
	$(call compile_amd_openmp_ext,GUIDED)
	$(call compile_amd_openmp_ext,RUNTIME)


run-amd-openmp-ext:
	$(BIN_DIR)$(MB)_amd_ext_dynamic.exe $(OUT_DIR)$(MB)_amd_ext_openmp_dynamic.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_amd_ext_static.exe $(OUT_DIR)$(MB)_amd_ext_openmp_static.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_amd_ext_guided.exe $(OUT_DIR)$(MB)_amd_ext_openmp_guided.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_amd_ext_runtime.exe $(OUT_DIR)$(MB)_amd_ext_openmp_runtime.out $(ITERATIONS)


amd-openmp: $(BIN_DIR)
	$(call compile_amd_openmp,DYNAMIC)
	$(call compile_amd_openmp,STATIC)
	$(call compile_amd_openmp,GUIDED)
	$(call compile_amd_openmp,RUNTIME)


run-amd-openmp:
	$(BIN_DIR)$(MB)_amd_dynamic.exe $(OUT_DIR)$(MB)_amd_openmp_dynamic.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_amd_static.exe $(OUT_DIR)$(MB)_amd_openmp_static.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_amd_guided.exe $(OUT_DIR)$(MB)_amd_openmp_guided.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_amd_runtime.exe $(OUT_DIR)$(MB)_amd_openmp_runtime.out $(ITERATIONS)


g++-openmp: $(BIN_DIR)
	$(call compile_g++_openmp,DYNAMIC)
	$(call compile_g++_openmp,STATIC)
	$(call compile_g++_openmp,GUIDED)
	$(call compile_g++_openmp,RUNTIME)


run-g++-openmp:
	$(BIN_DIR)$(MB)_g++_dynamic.exe $(OUT_DIR)$(MB)_g++_openmp_dynamic.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_g++_static.exe $(OUT_DIR)$(MB)_g++_openmp_static.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_g++_guided.exe $(OUT_DIR)$(MB)_g++_openmp_guided.out $(ITERATIONS)
	$(BIN_DIR)$(MB)_g++_runtime.exe $(OUT_DIR)$(MB)_g++_openmp_runtime.out $(ITERATIONS)


cuda: $(BIN_DIR)
	$(NVC) $(NVC_FLAGS) -g $(SRC_CUDA_DIR)mandelbrot.cu $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_cuda.exe
# $(NVC) $(NVC_FLAGS) $(SRC_CUDA_DIR)mandelbrot.cu $(LIB_LOGCPP) -o $(BIN_DIR)$(MB)_cuda.exe

run-cuda:
	$(BIN_DIR)$(MB)_cuda.exe $(OUT_DIR)$(MB)_cuda.out $(ITERATIONS)

clean:
	@echo "Cleaning up..."
	@$(call RM,$(BIN_DIR)) $(call RM,$(OUT_DIR))
