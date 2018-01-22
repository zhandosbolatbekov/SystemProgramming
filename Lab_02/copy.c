#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>

#define BUF_SIZE 128
#define MAX_READ 20

int bs[] = {
	1,
	1024,
	1130
};
char *files[] = {
	"file_2.5mb.jpg",
	"file_28.5mb.pdf",
	"file_208.7mb.zip"
};

double start = 0;

double time_to_copy(int buf_size, int id) {
	clock_t start = clock();

	char *fname_from = files[id];
	char *fname_to = "copy";

	if(strcmp(fname_from, fname_to) == 0) {
		perror("Can not copy the file to itself.\n");
		exit(0);
	}

	int inputFd = open(fname_from, O_RDONLY);
	if(inputFd == -1) {
		printf("Error while opening file %s\n", fname_from);
		exit(0);
	}

	int outputFd = open(fname_to, 
						O_CREAT | O_WRONLY, 
						S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	if(outputFd == -1) {
		close(inputFd);
		printf("Error while opening file %s\n", fname_to);
		exit(0);
	}

	char buffer[buf_size];
	int read_cnt;
	while((read_cnt = read(inputFd, buffer, buf_size)) > 0) {
		if(read_cnt == -1) {
			close(inputFd);
			close(outputFd);
			printf("Error while reading file %s\n", fname_from);
			exit(0);
		}
		int write_cnt = write(outputFd, buffer, read_cnt);
		if(write_cnt == -1) {
			close(inputFd);
			close(outputFd);
			printf("Error while writing file %s\n", fname_to);
			exit(0);
		}
	}

	close(inputFd);
	close(outputFd);


	return (clock() - start) * 1.0 / CLOCKS_PER_SEC;
}

int get_size(int id) {
	struct stat st;
	stat(files[id], &st);
	return st.st_size / 1024 / 1024;
}

int main(int argc, char **argv) {

	for(int i = 0; i < 3; ++i) {
		for(int j = 0; j < 3; ++j) {
			printf("BUFFER SIZE = %d B,        FILE SIZE = %d MB,        ELAPSED TIME = %.10f s.\n", 
				bs[i], get_size(j), time_to_copy(bs[i], j));
		}
	}

	return 0;
}

