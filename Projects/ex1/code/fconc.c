#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void doWrite(int fd, const char *buff, int len) {
//similar to write without the for loop
	size_t idx;
	ssize_t wcnt;
	idx = 0;
	do {
		wcnt = write(fd, buff + idx, len - idx);
		if (wcnt == -1){ /* error */
			perror("write");
			return 1;
		}
		idx += wcnt;
	} while (idx < len);
}

void write_file(int fd, const char *infile) {
//similar to open read and then read
	int fdopen;
	char buff[1024];
	ssize_t rcnt;
	fdopen = open(infile, O_RDONLY);
	if (fdopen == -1){
		perror(infile);
		exit(1);
	}
	// perform read(...)
	for (;;){
		rcnt = read(fdopen, buff, sizeof(buff)-1);
		if (rcnt == 0) /* end.of.file */
			return 0;
		if (rcnt == -1){ /* error */
			perror("read");
			return 1;
		}
		buff[rcnt] = '\0';
		int len = strlen(buff);
		doWrite(fd, buff, len);
	}
	close(fdopen);
}

int main(int argc, char **argv) {
	int fd, oflags, mode;
	oflags = O_CREAT | O_WRONLY | O_TRUNC;
	mode = S_IRUSR | S_IWUSR;
	if (argc != 3 && argc != 4) {
		printf("Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]\n");
		exit(1);
	}
	
	if (argc == 3) {
		fd = open("fconc.out", oflags, mode);
	}
	else {
		fd = open(argv[3], oflags, mode);
	}
	if (fd == -1){
		perror("open");
		exit(1);
	}
	else {
		write_file(fd, argv[1]);
		write_file(fd, argv[2]);
		close(fd);
	}
	return 0;
}

