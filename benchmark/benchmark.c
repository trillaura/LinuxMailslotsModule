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

int OPERATION_NUM;
int THREAD_NUM = 4;

void *execute_writer(void *arg) {
	char *name = (char *) arg;

	int fd = open((char * )arg, O_RDWR);
        char *message = "abcdefghijklmnopqrstuvwxyz\0";
        int i;
	int j =0 ;
        for (i = 0; i < OPERATION_NUM; i++) {
                if (write(fd, message, strlen(message)) != -1)
			++j;
        }
	printf("Written %d times\n", j);
}

void worker_writer(char *name) {
        int fd = open(name, O_WRONLY);
        char *message = "abcdefghijklmnopqrstuvwxyz\0";
        int i;
	int j=0;
        for (i = 0; i < THREAD_NUM*OPERATION_NUM; i++) {
                if (write(fd, message, strlen(message))!=-1)
			++j;
                if (i % 10 == 0)
                        usleep(10);
        }
	printf("Written %d times\n", j);
}


void *execute_reader(void *arg) {
        char *name = (char *) arg;

        int fd = open((char * )arg, O_RDWR);
        char *message = "abcdefghijklmnopqrstuvwxyz\0";
        int i;
	int j = 0;
        for (i = 0; i < OPERATION_NUM; i++) {
                char buff[256];
                if (read(fd, buff, 256) > 0) {
			++j;
                } else 
			--i;         
      	}	
	
	printf("Read %d times\n", j);
}

void worker_reader(char * name) {	
	int i;
	int m = 4;
	int j = 0;
        int fd = open(name, O_RDONLY);
        for (i=0; i<THREAD_NUM*OPERATION_NUM; i++) {
        	char buff[256];
                if (read(fd, buff, 256) > 0) {
			++j;
                } else {
			usleep(10);
		}
	}	
	printf("Read %d times\n", j);
}

int main(int argc, char **argv) {
	
	if (argc < 5) {
		printf("Usage <num_operations> <[r/w]> <[mkfifo/mailslot]> <device_name> *<major> *<minor>\n");
		return -1;
	}
	
	OPERATION_NUM = atoi(argv[1]);
	char *op_type = argv[2];
	char *driver = argv[3];

	char *prefix = "/dev/";
        char *name = (char *) malloc(strlen(prefix) + strlen(argv[4]) + 1); 
        strcpy(name, prefix);
        strcat(name, argv[4]);
	
	if (!strcmp(driver, "mkfifo")) {
		if (mkfifo(name, O_CREAT | 0666) == -1) {
                	printf("[ERROR] in mkfifo creation.\n");
                	return -1; 
        	}
	
                int t;
                pthread_t threads[THREAD_NUM];
                printf("Creating %d threads...\n", THREAD_NUM);

		if (!strcmp(op_type, "w")) {
			if (fork() == 0) {
				execute_reader((void *)name);
				//worker_reader(name);
				return 0;
			}
		} else {
			if (fork() == 0) {
				execute_writer((void *)name);
				//worker_writer(name);
				return 0;
			}	
		}
                for (t=0; t < THREAD_NUM; t++) {
			if (!strcmp(op_type, "w")) {
                        	pthread_create(&threads[t], NULL, execute_writer, (void *) name);
			} else {
                        	pthread_create(&threads[t], NULL, execute_reader, (void *) name);
			}
                }
                printf("%d threads created\n", THREAD_NUM);
		for (t=0; t < THREAD_NUM; t++) {
			pthread_join(threads[t], NULL);
		}
		remove(name);
	} else if (!strcmp(driver, "mailslot")) {
		int major = atoi(argv[5]);
		int minor = atoi(argv[6]);
		if (mknod(name, 0666 | S_IFCHR, makedev(major, minor)) == -1) {
                	printf("[ERROR] in mailslot creation.\n");
                	return -1; 	
		}
		
		int t;
                pthread_t threads[THREAD_NUM];
		printf("Creating %d threads...\n", THREAD_NUM);
		
		if (!strcmp(op_type, "w")) {
			if (fork() == 0) {
				printf("worker reader pid %d\n", getpid());
				//execute_reader((void *)name);
				worker_reader(name);
				return 0;
			}
		} else {
			if (fork() == 0) {
				printf("worker writer pid %d\n", getpid());
				//execute_writer((void *)name);
				worker_writer(name);
				return 0;
			}	
		}
                for (t=0; t < THREAD_NUM; t++) {
			if (!strcmp(op_type, "w")) {
				printf("Write op pid %d\n", getpid());
                        	pthread_create(&threads[t], NULL, execute_writer, (void *) name);
			} else {
				printf("Read op pid %d\n", getpid());
                        	pthread_create(&threads[t], NULL, execute_reader, (void *) name);
			}
                }
                printf("%d threads created\n", THREAD_NUM);
		
		for (t=0; t < THREAD_NUM; t++) {
			pthread_join(threads[t], NULL);
		}

		remove(name);
	} else {
		printf("Driver %s not supported\n", driver);	
		return -1;
	}
	return 0;
}
