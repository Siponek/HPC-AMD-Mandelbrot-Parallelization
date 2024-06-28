$(info Running Makefile for HPC project)
CC = icx
CFLAGS = -Wall
# CFLAGS = -Wall -diag-disable=10441

BIN_DIR = ./bin/
OUT_DIR = ./out/

SRC_SEQ_DIR = ./src/standard-sequential/

# Check if we're on Windows or Unix-like system
ifeq ($(OS),Windows_NT)
    MKDIR = powershell -Command "if (!(Test-Path '$(subst /,\,${1}'))) { New-Item -ItemType Directory -Path '$(subst /,\,${1})' }"
    RM = powershell -Command "if (Test-Path '$(subst /,\,${1}')) { Remove-Item -Recurse -Force '$(subst /,\,${1})' }"
else
    MKDIR = mkdir -p ${1}
    RM = rm -rf ${1}
endif

.PHONY: all test seq clean

all: test seq

$(BIN_DIR):
	@$(call MKDIR,$(BIN_DIR))

$(OUT_DIR):
	@$(call MKDIR,$(OUT_DIR))

test: $(BIN_DIR)
	@echo "Test"
	$(CC) $(CFLAGS) $(SRC_SEQ_DIR)hello.cpp -o $(BIN_DIR)hello.exe

seq: $(BIN_DIR)
	$(CC) $(CFLAGS) $(SRC_SEQ_DIR)mandelbrot.cpp -o $(BIN_DIR)seq_mandelbrot.exe

clean:
	@echo "Cleaning up..."
	@$(call RM,$(BIN_DIR)) $(call RM,$(OUT_DIR))
