CFLAGS=-std=c99 -Wall -Wextra -g -O3 -march=native

default:
	clang -framework OpenCL *.c -o PDFCrackCL