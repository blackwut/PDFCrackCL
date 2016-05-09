CFLAGS=-std=c99 -Wall -Wextra -g -O3 -march=native

ifeq ($(OS), Darwin)
	LDLIBS=-framework OpenCL
else
	LDLIBS=-lrt -lOpenCL -lcrypto -lssl
endif

PDFCrackCL:
	$(CC) $(LDLIBS) $(CFLAGS) *.c -o PDFCrackCL

clean:
	rm PDFCrackCL