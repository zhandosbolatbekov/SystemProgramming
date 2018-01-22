#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

struct person {
	char name [10];
	char id [10];
	off_t pos;
} people[] = {
	{"aizhan", "123456789", 0}, 
	{"nurzhamal", "987654321", 10240}, 
	{"ainur", "192837465", 81920},
};

int main(int argc, char ** argv) {
	int fd;
	int i, j;
	struct stat sbuf;

	if (argc < 2) {
		fprintf(stderr, "usage: %s file\n", argv[0]);
		return 1;
	}

	fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		fprintf(stderr, "%s: %s: cannot open for read/write: %s\n", 
				argv[0], argv[1], strerror(errno));
		return 1;
	}

	j = sizeof(people) / sizeof(people[0]);

	for (i = 0; i < j; i++) {
		if (lseek(fd, people[i].pos, SEEK_SET) < 0) {
			fprintf(stderr, "%s: %s: seek error: %s\n", argv[0], argv[1], strerror(errno));
			(void) close(fd);
			return 1;
		}

		if (write(fd, &people[i], sizeof(people[i])) != sizeof(people[i])) {
			fprintf(stderr, "%s: %s: write error: %s\n", argv[0], argv[1], strerror(errno));
			(void)close(fd);
			return 1;
		}
	} 

	if (stat(argv[1], &sbuf) < 0) {
		fprintf(stderr, "%s: stat: %s\n", argv[1], strerror(errno));
		exit(EXIT_FAILURE);
	}

	printf("block size is %ld\n", (long)sbuf.st_blksize);
	exit(EXIT_SUCCESS);
	(void)close(fd);
	return 0;
}

