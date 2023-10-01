
CFLAGS=-std=c99 -g3 -O0 -Wall -pedantic -Wextra -Wshadow -Wpointer-arith \
	-Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wconversion -Wno-overlength-strings \
	-D_POSIX_SOURCE=1 -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64

.PHONY: all test clean

all:
	$(CC) $(CFLAGS) src/ttorrent.c src/file_io.c src/logger.c -o bin/ttorrent -lssl -lcrypto

test:
	$(CC) $(CFLAGS) -Werror src/ttorrent.c src/file_io.c src/logger.c -o bin/ttorrent -lssl -lcrypto
	bash test_script.sh
	
clean:
	rm -f bin/ttorrent
