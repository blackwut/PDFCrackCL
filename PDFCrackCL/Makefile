CFLAGS=-std=c99 -Wall -Wextra -g -O3 -march=native

ifeq ($(OS), Linux)
	LDLIBS=-lrt -lOpenCL -lcrypto -lssl
else 
	LDLIBS=-framework OpenCL
endif

PDFCrackCL:
	$(CC) $(LDLIBS) $(CFLAGS) *.c -o PDFCrackCL

clean:
	rm PDFCrackCL