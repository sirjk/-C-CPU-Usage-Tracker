# CPU Usage Tracker

## Compilation
* Compilation with clang:
	```
	make CC=clang
	```

* Compilation with gcc:
	```
	make CC=gcc
	```

### Note

As mentioned in the comment started at line 340 in components.c: using 2s timeout it occured quickly,
so I suggest setting the WAIT_TIME_SECS macro (defined in components.h) to 4. Of course timeout 
appearance depends on hardware capabilities, so You may not encounter that problem.

