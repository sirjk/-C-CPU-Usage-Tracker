#include "components.h"

void setVar(void){

	cpu_percentage = malloc(sizeof(float) * NUM_OF_CPUS);

	log_msg = malloc(sizeof(Log));

	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutex_log, NULL);

	pthread_cond_init(&reading, NULL);
	pthread_cond_init(&analyzing, NULL);
	pthread_cond_init(&printing, NULL);
	pthread_cond_init(&logging, NULL);

	pthread_cond_init(&reader_time_cond, NULL);
	pthread_cond_init(&analyzer_time_cond, NULL);
	pthread_cond_init(&printer_time_cond, NULL);

	file_consumed = true;
	file_analyzed = false;
	log_created = false;

	struct sigaction action;
	memset(&action,0,sizeof(struct sigaction));
	action.sa_handler = terminate;
	sigaction(SIGTERM, &action, NULL);
}

/* ----------- QUEUE FUNCTIONS ----------- */
Queue* createQueue(int capacity) {

	Queue *queue = malloc(sizeof(Queue));
	for (int i=0; i<capacity; i++) {
		queue->cpu_info[i] = malloc(NUM_OF_CPUS * sizeof(CpuInfo));

		if (queue->cpu_info[i] == NULL) {
			printf("\nfailed while allocating memory in createQueue function\n");
		}
	}
	queue->capacity = capacity;
	queue->size = queue->head = 0;
	queue->tail = queue->capacity - 1;

	return queue;
}

// Populating queue with one element (which in fact is array of CpuInfo structs),
// element is being added to the end (FIFO)
void enqueue(Queue *queue, CpuInfo cpu_info[]) {

	if (isFull(queue)) {
		printf("\nCannot enqueue - queue is full.\n");
		return;
	}

	queue->tail = (queue->tail + 1) % queue->capacity;

	for (int i=0; i<NUM_OF_CPUS; i++) {
		queue->cpu_info[queue->tail][i].cpu_id = cpu_info[i].cpu_id;
		queue->cpu_info[queue->tail][i].user = cpu_info[i].user;
		queue->cpu_info[queue->tail][i].nice = cpu_info[i].nice;
		queue->cpu_info[queue->tail][i].system = cpu_info[i].system;
		queue->cpu_info[queue->tail][i].idle = cpu_info[i].idle;
		queue->cpu_info[queue->tail][i].iowait = cpu_info[i].iowait;
		queue->cpu_info[queue->tail][i].irq = cpu_info[i].irq;
		queue->cpu_info[queue->tail][i].softirq = cpu_info[i].softirq;
		queue->cpu_info[queue->tail][i].steal = cpu_info[i].steal;
		queue->cpu_info[queue->tail][i].guest = cpu_info[i].guest;
		queue->cpu_info[queue->tail][i].guest_nice = cpu_info[i].guest_nice;
	}

	queue->size++;
}

// Removing head from the queue
void dequeue(Queue *queue) {
	if (isEmpty(queue)){
		printf("\nCannot dequeue - queue is empty.\n");
		return;
	}

	queue->head = (queue->head + 1) % queue->capacity;
	queue->size--;
}

bool isFull(Queue *queue) {

	return (queue->size == queue->capacity);
}

bool isEmpty(Queue *queue) {

	return (queue->size == 0);
}

CpuInfo* getHead(Queue *queue) {

	return queue->cpu_info[queue->head];
}

CpuInfo* getTail(Queue *queue) {

	return queue->cpu_info[queue->tail];
}

void printQueue(Queue *queue) {
	printf("\nQUEUE INFO");
	printf("\n-------------");
	printf("\nsize: \t%d",queue->size);
	printf("\ncapacity: \t%d",queue->capacity);
	printf("\nhead: \t%d",queue->head);
	printf("\ntail: \t%d",queue->tail);
	printf("\nelements:");
	int index;
	for (int i=0; i<queue->size; i++) {
		//queue elements are not stored in the order from 0 to size-1
		index = (queue->head + i) % queue->capacity;
		printf("\n%d.", i+1);
		for (int j=0; j<NUM_OF_CPUS;j++) {
			printf("\n\t%d %d %d %d %d %d %d %d %d %d %d",
				queue->cpu_info[index][j].cpu_id, queue->cpu_info[index][j].user,queue->cpu_info[index][j].nice,
				queue->cpu_info[index][j].system,queue->cpu_info[index][j].idle,queue->cpu_info[index][j].iowait,
				queue->cpu_info[index][j].irq,queue->cpu_info[index][j].softirq,queue->cpu_info[index][j].steal,
				queue->cpu_info[index][j].guest,queue->cpu_info[index][j].guest_nice);
		}
	}
}


/* ----------- THREADS FUNCTIONS ----------- */

// That function writes to the log the parsed message and when the log is created
// it signals the waiting logger about the ready log,
// as a shared data, the log has to be protected by mutex
void createLog(Log *l, char *l_msg, pthread_mutex_t *l_mutex, pthread_cond_t *l_cond, bool *l_created) {
	
	l->msg = malloc(sizeof(char) * (strlen(l_msg) + 1));

	if (l->msg == NULL) {
			printf("\nfailed while allocating memory in createLog function\n");
		}

	l->msg = l_msg;
	time(&(l->time));

	pthread_mutex_lock(l_mutex);
	*l_created = true;
	pthread_mutex_unlock(l_mutex);	
	pthread_cond_signal(l_cond);
}

// Firstly, the reader has to read the values twice for the analyzer to calculate
// deltas. The data shared between reader and analyzer is the raw_data file.
// Mutex guarantees that no two functions can execute simultanously (critical 
// sections). It rather protects the condition variable than the data itself.
// Condition variable enables the synchronization, so the reader won't write to
// file when the analyzer haven't read it yet and vice versa.
void* reader(void) {

	FILE *stat;
	char c;

	while (!INTERRUPTED) {

		pthread_mutex_lock(&mutex);
		pthread_cond_signal(&reader_time_cond);
		while (!file_consumed) pthread_cond_wait(&reading,&mutex);

		createLog(log_msg, "[READER] *I begin to write the file*", &mutex_log, &logging, &log_created);

		stat = fopen("/proc/stat", "r");
		raw_data = fopen("./raw_data", "w");
	
		if (stat == NULL) {
			printf("\ncannot open the /proc/stat\n");
			exit(1);
		}
		if (raw_data == NULL) {
			printf("\ncannot open the raw_data file\n");
			exit(1);
		}
	
		while ((c=(char)fgetc(stat)) != EOF) {
			fputc(c, raw_data);
		}
	
		fclose(stat);
		fclose(raw_data);
		file_consumed = !file_consumed;
		
		createLog(log_msg, "[READER] *I finish writing the file*", &mutex_log, &logging, &log_created);
		
		pthread_mutex_unlock(&mutex);
		
		pthread_cond_signal(&analyzing);

		sleep(1);
	}

	return NULL;
}


// Analyzer creates the queue of size 2 at the beggining, and waits for it to be full.
// It populates the cpu_percentage array with calculated values and similarly it waits 
// for the printer the print out the values
void* analyzer(void) {

	Queue *queue = createQueue(2);
	CpuInfo *cpu_prev, *cpu_act;
	cpu_prev = cpu_act = NULL;
	char line[200];
	int prev_idle, idle, prev_non_idle, non_idle, prev_total, total,
		total_d, idle_d;

	while (!INTERRUPTED) {

		pthread_mutex_lock(&mutex);
		pthread_cond_signal(&analyzer_time_cond);

		while (file_consumed) pthread_cond_wait(&analyzing,&mutex);

		CpuInfo *cpu_info = malloc(sizeof(CpuInfo) * NUM_OF_CPUS);

		if (cpu_info == NULL) {
			printf("\nfailed while allocating memory in analyzer function\n");
		}
		
		createLog(log_msg, "[ANALYZER] *I begin to read the file*", &mutex_log, &logging, &log_created);

		raw_data = fopen("./raw_data","r");

		if (raw_data == NULL) {
			printf("\ncannot open the raw_data file\n");
			exit(1);
		}

		//in case of that task the first line
		//of /proc/stat is irrelevant
		fgets(line, 200, raw_data);
		
		for (int i=0; i<NUM_OF_CPUS; i++) {
			memset(line,0,200);
			fgets(line, 200, raw_data);
			cpu_info[i].cpu_id=i;
			sscanf(line,"%*s %d %d %d %d %d %d %d %d %d %d",
				&cpu_info[i].user,&cpu_info[i].nice,
				&cpu_info[i].system,&cpu_info[i].idle,&cpu_info[i].iowait,
				&cpu_info[i].irq,&cpu_info[i].softirq,&cpu_info[i].steal,
				&cpu_info[i].guest,&cpu_info[i].guest_nice);
		}

		enqueue(queue, cpu_info);

		if (isFull(queue)) {
			cpu_prev = getHead(queue);
			cpu_act = getTail(queue);
			
			for (int i=0; i<NUM_OF_CPUS; i++) {
				prev_idle = cpu_prev[i].idle + cpu_prev[i].iowait;
				idle = cpu_act[i].idle + cpu_act[i].iowait;
				
				prev_non_idle = cpu_prev[i].user + cpu_prev[i].nice +
					+ cpu_prev[i].system + cpu_prev[i].irq + cpu_prev[i].softirq +
					+ cpu_prev[i].steal;
				non_idle = cpu_act[i].user + cpu_act[i].nice + cpu_act[i].system +
					+ cpu_act[i].irq + cpu_act[i].softirq + cpu_act[i].steal;

				prev_total = prev_idle + prev_non_idle;
				total = idle + non_idle;

				total_d = total - prev_total;
				idle_d = idle - prev_idle;

				cpu_percentage[i] = 100 * (float)(total_d - idle_d) / (float)total_d;
			}

			dequeue(queue);	
		}

		free(cpu_info);
		file_consumed = !file_consumed;
		
		createLog(log_msg, "[ANALYZER] *I finish reading the file*", &mutex_log, &logging, &log_created);
		pthread_mutex_unlock(&mutex);

		pthread_cond_signal(&reading);
		pthread_cond_signal(&printing);
		sleep(1);
	}

	if (INTERRUPTED) {
		free(queue);
		free(cpu_prev);
		free(cpu_act);
	}

	return NULL;
}

void* printer(void) {

	while (!INTERRUPTED) {

		pthread_mutex_lock(&mutex);
		pthread_cond_signal(&printer_time_cond);
		pthread_cond_wait(&printing, &mutex);

		createLog(log_msg, "[PRINTER] *I begin to print out the result*", &mutex_log, &logging, &log_created);

		printf("\n");
		for (int i=0; i<NUM_OF_CPUS; i++) {
			printf("\tCPU%d\t",i);
		}
		printf("\n");
		for (int i=0; i<NUM_OF_CPUS*16; i++) {
			printf("-");		
		}
		printf("\n");
		//printf("\n------------------------------------------------------------------\n");
		for (int i=0; i<NUM_OF_CPUS; i++) {
			printf("\t%.2f%%\t",(double)cpu_percentage[i]);
		}

		createLog(log_msg, "[PRINTER] *I finish printing out the result*", &mutex_log, &logging, &log_created);

		pthread_mutex_unlock(&mutex);
	}

	return NULL;
}


// Watchdog waits the fixed WAIT_TIME_SECS amount of seconds for every
// three threads to send signal
void* watchdog(void) {

	/* The question here is if every of the three threads (or rather 
		their functions) has its own two seconds to execute so in the
		worst case the program will not print the result in <6secs 
		OR if the 2secs is the global time in which every three
		operations should fit.
	IMPORTANT:
		I assumed that 2secs is the global time, but it turned out that
		in that scenario the timeout occured pretty quickly.
		Increasing the wait time I observed no timeout with its value
		set to 4secs. As I'm uploading that project I leave the value
		on 2secs because I suppose that is the required version.
		Just to make things clear :) */

	struct timespec ts;
	struct timeval tp;
	int ret;

	while (!INTERRUPTED) {
		/* pthread_cond_timedwait uses absolute time in the timespec format. 
			to calculate the time till the timeout the gettimeofday
			function is used - it gets the time since epoch in timeval format
			so it is necessary to convert it to timespec. */

		gettimeofday(&tp, NULL);
		ts.tv_sec = tp.tv_sec;
		ts.tv_nsec = tp.tv_usec * 1000;
		ts.tv_sec += WAIT_TIME_SECS;

		ret = pthread_cond_timedwait(&reader_time_cond, &mutex, &ts);
		ret = pthread_cond_timedwait(&analyzer_time_cond, &mutex, &ts);
		ret = pthread_cond_timedwait(&printer_time_cond, &mutex, &ts);

		if (ret == ETIMEDOUT) {
			printf("\n*TIMED OUT*\n");
			exit(1);
		}
	}

	return NULL;
}

// Logger waits for any log to be created and writes it to the file
void* logger(void) {

	FILE *log_file;
	bool b = true;
	time_t t;
	time(&t);

	while (!INTERRUPTED) {

		pthread_mutex_lock(&mutex_log);

		log_file = fopen("./log", "a");

		if (log_file == NULL) {
			printf("\ncannot open the log.txt file\n");
			perror("./log");
			exit(1);
		}
		
		if (b) {
			fputc(10, log_file);
			fputs(strcat(ctime(&t),
				"\t------ NEW EXECUTION OF THE PROGRAM ------\n"), 
				log_file);
		}

		while (!log_created) pthread_cond_wait(&logging, &mutex_log);

		fputs(ctime(&(log_msg->time)), log_file);
		fputc(9, log_file);
		fputs(log_msg->msg, log_file);
		fputc(10, log_file);
		
		log_created = false;
		b = false;		
		fclose(log_file);

		pthread_mutex_unlock(&mutex_log);
	}

	if (INTERRUPTED) {
		pthread_cond_destroy(&reader_time_cond);
		pthread_cond_destroy(&analyzer_time_cond);
		pthread_cond_destroy(&printer_time_cond);

		pthread_cond_destroy(&analyzing);

		pthread_cond_destroy(&printing);

		pthread_cond_destroy(&reading);
		pthread_mutex_destroy(&mutex);

		pthread_mutex_destroy(&mutex_log);
		pthread_cond_destroy(&logging);		
	}

	return NULL;
}

void terminate(int signum) {
	signum++;
	INTERRUPTED = 1;
}
