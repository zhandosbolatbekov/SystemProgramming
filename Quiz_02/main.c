#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>

static int serving = 0;
static int queue_size = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

static void *server() {
	int s;
	while(1) {
		if(serving) {
			sleep(rand() % 5 + 1);
		}
		s = pthread_mutex_lock(&mtx);
		if(s != 0) {
			perror("pthread_mutex_lock");
		} 
		if(serving == 1) {
			serving = 0;
			printf("serving finished\n");
		}
		if(queue_size > 0) {
			serving = 1;
			int val = queue_size;
			val--;
			queue_size = val;
			printf("one from waiting room sit to barber chair\n");
		} else {
			serving = 0;
			printf("barber now is asleep\n");
		}
		printf("SERVING = %d,  QUEUE_SIZE = %d\n\n", serving, queue_size);
		s = pthread_mutex_unlock(&mtx);
		if(s != 0) {
			perror("pthread_mutex_unlock");
		}
	}
}

static void *pusher() {
	int s;
	while(1) {
		sleep(rand() % 3 + 1);
		s = pthread_mutex_lock(&mtx);
		if(s != 0) {
			perror("pthread_mutex_lock");
		} 
		if(queue_size == 0) {
			if(serving == 0) {
				serving = 1;
				printf("one entered barbershop and immediately sit to barber chair\n");
			} else {
				int val = queue_size;
				val++;
				queue_size = val;
				printf("one entered barbershop and occupied a waiting chair\n");		
			}
		} else if(queue_size < 3) {
			int val = queue_size;
			val++;
			queue_size = val;
			printf("one entered barbershop and occupied a waiting chair\n");
		} else {
			printf("one entered barbershop and not found vacant waiting chair\n");
		}
		printf("SERVING = %d,  QUEUE_SIZE = %d\n\n", serving, queue_size);
		s = pthread_mutex_unlock(&mtx);
		if(s != 0) {
			perror("pthread_mutex_unlock");
		}
	}
}

int main(int argc, char **argv) {

	pthread_t tid[2];
	int code;
	if((code = pthread_create(&tid[0], NULL, server, NULL))) {
		printf("error: pthread_create, code: %d\n", code);
  		return EXIT_FAILURE;
	}
	if((code = pthread_create(&tid[1], NULL, pusher, NULL))) {
		printf("error: pthread_create, code: %d\n", code);
		return EXIT_FAILURE;
	}

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);

	return EXIT_SUCCESS;
}

