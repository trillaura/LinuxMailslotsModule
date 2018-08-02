#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define FIFO_NAME "mkfifo"

typedef struct {
	long type;
	char fifo_msg[256];
} message;


int fifo_writer() {

	char msg[256];
	
	int ret;
	if ((ret = mkfifo(FIFO_NAME, O_CREAT|0666)) == -1) {
		printf("[ERROR] in mkfifo creation.\n");
		return -1;
	}

	int fd;
	if ((fd = open(FIFO_NAME, O_WRONLY)) ==-1) {
		printf("[ERROR] in fifo opening.\n");
		return -1;
	}

	printf("PROC %d writing... Write a message\n", getpid());
	
	while (scanf("%256s[^\n]%c", msg, (char *)'\n') == 1) {
		
		if (strcmp(msg, "quit") == 0) {
			break;
		}
	
		write(fd, msg, sizeof(msg));

		printf("Written %s\n", msg);
	}
		
	close(fd);
	remove(FIFO_NAME);
	
	return 0;
}


int fifo_reader() {
	int fd = open(FIFO_NAME, O_RDONLY);
	if (fd == -1) {
		printf("ERR\n");
		return -1;
	}
	
	printf("PROC -%d reading...\n", getpid());
	
	char buf[256];
	int ret = 0;
	while (ret <= 0) {
		ret = read(fd, buf, sizeof(buf));	
	}
	
	printf("Read %s \n", buf);
	
	close(fd);
	
	remove(FIFO_NAME);
	return 0;
}


int main(int argc, char **argv) {

	int pid = fork();
	if (pid == -1) {
		return -1;
	}

	if (pid != 0) {
		fifo_writer();
	} else {
		fifo_reader();
	}

	return 0;
}
