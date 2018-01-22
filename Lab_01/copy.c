#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#define BUF_SIZE 128
#define MAX_READ 20

int main(int argc, char **argv) {

	if(argc != 3) {
		perror("Incorrect arguments.\n");
		return 0;
	}
	
	char *fname_from = argv[1];
	char *fname_to = argv[2];

	if(strcmp(fname_from, fname_to) == 0) {
		perror("Can not copy the file to itself.\n");
		return 0;
	}

	int inputFd = open(fname_from, O_RDONLY);
	if(inputFd == -1) {
		printf("Error while opening file %s\n", fname_from);
		return 0;
	}

	int outputFd = open(fname_to, 
						O_CREAT | O_WRONLY | O_TRUNC, 
						S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	if(outputFd == -1) {
		close(inputFd);
		printf("Error while opening file %s\n", fname_to);
		return 0;
	}

	char buffer[MAX_READ];
	int read_cnt;
	while((read_cnt = read(inputFd, buffer, MAX_READ)) > 0) {
		if(read_cnt == -1) {
			close(inputFd);
			close(outputFd);
			printf("Error while reading file %s\n", fname_from);
			return 0;
		}
		int write_cnt = write(outputFd, buffer, read_cnt);
		if(write_cnt == -1) {
			close(inputFd);
			close(outputFd);
			printf("Error while writing file %s\n", fname_to);
			return 0;
		}
	}

	close(inputFd);
	close(outputFd);

	printf("Successfully copied.\n");
}

