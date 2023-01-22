CC = gcc
#CFLAGS = -Wall -Werror -O2

all: test_app fungi hrib

hrib: hrib.c
	@$(CC) $(CFLAGS) -pthread -o $@ $<
	@echo "$<"
	@echo "Build Successful" || echo "Build Failed"

test_app: test_app.c
	@$(CC) $(CFLAGS) -o $@ $<
	@echo "$<"
	@echo "Build Successful" || echo "Build Failed"

fungi: fungi.cpp
	@$(CC) $(CFLAGS) -pthread -o $@ $<
	@echo "$<"
	@echo "Build Successful" || echo "Build Failed"

.PHONY: clean

clean:
	rm ./test_app ./fungi ./hrib

