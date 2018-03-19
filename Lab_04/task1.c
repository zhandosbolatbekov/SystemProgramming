#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 11

char input[10][10];

void *check_cols(void *arg) {
	int exist[10];
	for(int j = 0; j < 9; ++j) {
		for(int i = 1; i <= 9; ++i)
			exist[i] = 0;
		for(int i = 0; i < 9; ++i) 
			exist[input[i][j] - '0'] = 1;
		for(int i = 1; i <= 9; ++i)
			if(!exist[i]) {
				printf("COL %d: number %d not found\n", j + 1, i);
			}
	}
	return NULL;
}

void *check_rows(void *arg) {
	int exist[10];
	for(int i = 0; i < 9; ++i) {
		for(int j = 1; j <= 9; ++j)
			exist[i] = 0;
		for(int j = 0; j < 9; ++j) 
			exist[input[i][j] - '0'] = 1;
		for(int j = 1; j <= 9; ++j)
			if(!exist[j]) {
				printf("ROW %d: number %d not found\n", i + 1, j);
			}
	}
	return NULL;
}

void *check_submatrix(void *arg) {
	
	int *sub_id = (int *)arg;

	int sx = (*sub_id) / 3 * 3;
	int sy = (*sub_id) % 3 * 3;

	// printf("%d %d %d\n", *sub_id, sx, sy);

	int exist[10] = {0};
	for(int i = 0; i < 3; ++i)
		for(int j = 0; j < 3; ++j)
			exist[input[sx + i][sy + j] - 'a'] = 1;
	for(int i = 1; i <= 9; ++i) {
		if(!exist[i]) {
			printf("SUBMATRIX %d: number %d not found\n", *sub_id, i);
		}
	}
	pthread_exit(NULL);
}

int main(int argc, char **argv) {

	FILE *fp;
	fp = fopen("input.txt", "r");

	for(int i = 0; i < 9; ++i) {
		fscanf(fp, "%s", input[i]);
	}	

	pthread_t tid[NUM_THREADS];
	int targ[NUM_THREADS];
	int code;

	// pthread_create(&tid[0], NULL, check_cols, NULL);
	// pthread_create(&tid[1], NULL, check_rows, NULL);

	for(int i = 0; i < 9; ++i) {
		targ[i] = i;
		if((code = pthread_create(&tid[i], NULL, check_submatrix, &targ[i]))) {
			printf("error: pthread_create, code: %d\n", code);
      		return EXIT_FAILURE;
		}
	}

	for(int i = 0; i < 9; ++i) {
		pthread_join(tid[i], NULL);
	}

	return EXIT_SUCCESS;
}

