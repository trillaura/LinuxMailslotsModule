/*
 * @license GPL
 * @author Laura Trivelloni
 * 
 * This application compare performances between standard mkfifo
 * and mailslot devices implementation, writing execution times in 
 * a file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define DEBUG if (0)

int OPERATION_NUM;
int THREAD_NUM = 4;

void *execute_writer(void *arg) {
	char *name = (char *) arg;

	int fd = open((char * )arg, O_RDWR);
        if (fd == -1) {
		printf("Error opening file.\n");
		return NULL;
	}
        char *message = "a";
        int i;
	int j =0 ;
        for (i = 0; i < OPERATION_NUM; i++) {
                if (write(fd, message, strlen(message)) != -1)
			++j;
        }
	close(fd);	
	DEBUG printf("Written %d times\n", j);
}

void *execute_reader(void *arg) {
        char *name = (char *) arg;

        int fd = open((char * )arg, O_RDWR);
        if (fd == -1) {
		printf("Error opening file.\n");
		return NULL;
	}
	int i;
	int j = 0;
        for (i = 0; i < OPERATION_NUM; i++) {
                char buff[256];
                if (read(fd, buff, 256) > 0)
			++j;
                else 
			--i;         
      	}	
	close(fd);	
	DEBUG printf("Read %d times\n", j);
}

int main(int argc, char **argv) {
	
	if (argc != 4) {
		printf("Usage <num_operations> <[r/w]> <device_name>\n");
		return -1;
	}
	
	OPERATION_NUM = atoi(argv[1]);
	char *op_type = argv[2];

	char *prefix = "/dev/";
	char *name = (char *) malloc(strlen(prefix) + strlen(argv[3]) + 1); 
    	strcpy(name, prefix);
    	strcat(name, argv[3]);

   	int t;
    	pthread_t threads[THREAD_NUM];
    	DEBUG printf("Creating %d threads...\n", THREAD_NUM);
 
	for (t=0; t < THREAD_NUM; t++) {
        	if (!strcmp(op_type, "w")) {
            		DEBUG printf("Write op pid %d\n", getpid());
            		pthread_create(&threads[t], NULL, execute_writer, (void *) name);
        	} else {
            		DEBUG printf("Read op pid %d\n", getpid());
            		pthread_create(&threads[t], NULL, execute_reader, (void *) name);
        	}
    	}
    	DEBUG printf("%d threads created\n", THREAD_NUM);

    	for (t=0; t < THREAD_NUM; t++) {
        	pthread_join(threads[t], NULL);
    	}

	return 0;
}
