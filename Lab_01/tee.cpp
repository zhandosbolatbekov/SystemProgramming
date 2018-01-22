#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#define MAX_READ 1000

int main(int argc, char **argv) {

	if(argc != 2) {
		perror("Incorrect arguments.\n");
		return 0;
	}
	
	char *fname = argv[1];

	int outputFd = open(fname, 
						O_CREAT | O_WRONLY | O_APPEND, 
						S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	if(outputFd == -1) {
		printf("Error while opening file %s\n", fname);
		return 0;
	}

	char buffer[MAX_READ];
	int read_cnt;
	while((read_cnt = read(0, buffer, MAX_READ)) > 0) {
		if(read_cnt == -1) {
			close(outputFd);
			perror("Error while reading %s\n");
			return 0;
		}
		printf("%s", buffer);
		int write_cnt = write(outputFd, buffer, read_cnt);
		if(write_cnt == -1) {
			close(outputFd);
			printf("Error while writing file %s\n", fname);
			return 0;
		}
	}

	close(outputFd);

	printf("Successfully appended.\n");
}

