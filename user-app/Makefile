CC = gcc
#CFLAGS = -Wall -Werror -O2

all: fungi

fungi: fungi.c
	@$(CC) $(CFLAGS) -pthread -o $@ $<
	@echo "$<"
	@echo "Build Successful" || echo "Build Failed"

.PHONY: clean

clean:
	rm ./fungi

