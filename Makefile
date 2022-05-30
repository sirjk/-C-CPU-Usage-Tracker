ifeq ($(CC),gcc)
	CFLAGS = -c -g -Wall -Wextra
else ifeq ($(CC),clang)
	CFLAGS = -c -g -Weverything
endif

cpu_usage_tracker: cpu_usage_tracker.o components.o
	$(CC) cpu_usage_tracker.o components.o -pthread -o cpu_usage_tracker

cpu_usage_tracker.o: cpu_usage_tracker.c components.h
	$(CC) $(CFLAGS) cpu_usage_tracker.c

components.o: components.h components.c
	$(CC) $(CFLAGS) components.c

clean:
	rm -f *.o
