/*
 * CPU Usage Tracker
 * That program was made as a recruitment task for Junior C Developer position 
 * Author: Jedrzej Kozuch, Krakow 2022
*/

#include "components.h"

volatile sig_atomic_t INTERRUPTED = 0;

float *cpu_percentage;
FILE *raw_data;
pthread_mutex_t mutex, mutex_log;
pthread_cond_t reading, analyzing, printing, logging; 
pthread_cond_t reader_time_cond, analyzer_time_cond, printer_time_cond;
bool file_consumed, file_analyzed, log_created;
Log *log_msg;

int main(void) {

	setVar();
	
	pthread_t t1,t2,t3,t4,t5;

	pthread_create(&t1,NULL,reader,NULL);
	pthread_create(&t2,NULL,analyzer,NULL);
	pthread_create(&t3,NULL,printer,NULL);
	pthread_create(&t4,NULL,watchdog,NULL);
	pthread_create(&t5,NULL,logger,NULL);

	pthread_join(t1,NULL);
	pthread_join(t2,NULL);
	pthread_join(t3,NULL);
	pthread_join(t4,NULL);
	pthread_join(t5,NULL);

	return 0;
}
