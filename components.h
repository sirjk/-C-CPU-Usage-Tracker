#ifndef COMPONENTS
#define COMPONENTS

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <signal.h>

#define NUM_OF_CPUS get_nprocs()
#define WAIT_TIME_SECS 2

// Information about one CPU from /proc/stat
typedef struct{
	int cpu_id;
	int user;
	int nice;
	int system;
	int idle;
	int iowait;
	int irq;
	int softirq;
	int steal;
	int guest;
	int guest_nice;
} CpuInfo;

// Queue for handling calculations in analyzer,
// one element in queue represents the dataset
// for NUM_OF_CPUS cpu's
typedef struct{
	int capacity;
	int size;
	int head;
	int tail;
	CpuInfo *cpu_info[];
} Queue;

// Single log data
typedef struct{
	time_t time;
	char *msg;
} Log;


// Necessary data:

// file "raw_data" is used as a shared data holder
// between reader and analyzer
extern FILE *raw_data;

// array with percentage values of cpu's usage,
// shared between analyzer and printer
extern float *cpu_percentage;

// boolean values to determine if the work related to 
// condition variables has benn done
extern bool file_consumed;
extern bool file_analyzed;
extern bool log_created;

// mutexes: 
// one shared between reader, analyzer and printer
extern pthread_mutex_t mutex;

// and one shared between logger and the three mentioned above but separately
extern pthread_mutex_t mutex_log;

// condition variables for thread synchronization
extern pthread_cond_t reading;
extern pthread_cond_t analyzing;
extern pthread_cond_t printing;
extern pthread_cond_t logging;

// and for signaling the watchdog
extern pthread_cond_t reader_time_cond;
extern pthread_cond_t analyzer_time_cond;
extern pthread_cond_t printer_time_cond;

// one global log struct
extern Log *log_msg;

// for SIGTERM handling
extern volatile sig_atomic_t INTERRUPTED;

// Functions executed by threads
void* reader(void);
void* analyzer(void);
void* printer(void);
void* watchdog(void);
void* logger(void);

// Functions handling the queue structure
Queue* createQueue(int);
void enqueue(Queue*, CpuInfo[]);
void dequeue(Queue*);
bool isFull(Queue*);
bool isEmpty(Queue*);
CpuInfo* getHead(Queue*);
CpuInfo* getTail(Queue*);
void printQueue(Queue*);

// Log creation
void createLog(Log*, char*, pthread_mutex_t*, pthread_cond_t*, bool*);

// SIGTERM handler
void terminate(int);

// Initializing variables
void setVar(void);

#endif
