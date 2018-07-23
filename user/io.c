#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

void *writer(void *arg) {
	int fd = open((char * )arg, O_WRONLY);
	char *text[6] = {"Sempre","caro","mi", "fu", "quest'ermo", "colle"};		
	int i;
	int size = sizeof(text)/sizeof(char *);
	for (i = 0; i < size; i++) {
		printf("Writing %s\n", *(text+i));
		write(fd, *(text+i), strlen(*(text+i)));
	} 
}

void *reader(void *arg) {
	int fd = open((char *)arg, O_RDONLY);
        char buff[256];
	int i=0;
        while (read(fd, buff, 256) != 0 && i<5) {
        	printf("%s\n", buff);
		i++;
		memset(&buff[0], 0, sizeof(buff));
	}	
}

void *controller(void *arg) {
	int fd = open((char*) arg, O_RDWR);
	ioctl(fd, 0, 0); 
}

int main(int argc, char **argv) {

	if (argc != 4) {
		printf("Usage <device> <major> <minor>\n");
		return -1;
	}	
	
	char *prefix = "/dev/";
	char *name = (char *) malloc(strlen(prefix) + strlen(argv[1]) + 1);
	strcpy(name, prefix);
	strcat(name, argv[1]);
	int major = atoi(argv[2]);
	int minor = atoi(argv[3]);

	mknod(name, 0666 | S_IFCHR, makedev(major, minor));
	
	int status = 0;

	pid_t child;
	//int w = open(name, O_RDWR);
	if ((child = fork()) == 0) {
		writer((void *) name);
		return 0;
	}
	waitpid(child, &status, 0);
	//printf("Written\n");
	reader((void *) name);

	controller((void*)name);
	return 0;
}
