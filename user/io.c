#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <string.h>
#include <sys/wait.h>

char *device = "/dev/mailslot1";

void *writer(void *arg) {
	int w = open(device, O_WRONLY);
	char *hello = "hello";
	write(w, hello, strlen(hello));
}

void *reader(void *arg) {
	int r = open(device, O_RDONLY);
        char buff[256];
        read(r, buff, 256);
        printf("Read %s\n", buff);	
}

int main() {

	mknod(device, 0666 | S_IFCHR, makedev(242, 1));
	
	int status = 0;

	writer(NULL);
	pid_t child;
	if ((child = fork()) == 0) {
		printf("oo\n");
		writer(NULL);
		return 0;
	}
	waitpid(child, &status, 0);
	printf("Written\n");
	reader(NULL);
	return 0;
}
