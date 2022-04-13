# Usage:
# make        # compile all binary
# make prog   # compile prog to call
# make clean  # remove ALL binaries and objects
CC = gcc # compiler to use

SRCMAIN := ./src/main.c
BINMAIN := main

all: build main

build:
	@echo "Creando carpeta"
	cd ${BUILDPATH}
	mkdir build

main:
	@echo "Compiling ..."
	${CC} -pthread ${SRCMAIN} -o build/${BINMAIN} -lm

clean:
	@echo "Deleting ..."
	rm -rf build
