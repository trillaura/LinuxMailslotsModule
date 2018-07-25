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
int THREAD_NUM;

void *execute(void *arg) {
	char *name = (char *) arg;

	int fd = open((char * )arg, O_RDWR);
        char *message = "abcdefghijklmnopqrstuvwxyz\0";
        int i;

        printf("Executing...\n");

	clock_t time_r = 0;
	clock_t time_w = 0;
	int num_r = 0;
	int num_w = 0;

        for (i = 0; i < OPERATION_NUM; i++) {
                printf("Written\n");
		
		clock_t begin = clock();
                if (write(fd, message, strlen(message)) == -1) {
                	break;
		}
		num_w++;
		time_w += clock() - begin;
		
		char buff[256];
		begin = clock();
               	if (read(fd, buff, 256) > 0) {	
			printf("Read %s\n", buff);
			time_r += clock() - begin;
			memset(&buff[0], 0, sizeof(buff));
                	num_r++;
                	printf("Read\n");
        	}	
        }

        FILE *res = fopen("res.txt", "a");
        fprintf(res, "[%s] %d writes %d threads avg time [ms] = %lf \n", (char *)arg, num_w, THREAD_NUM, 100*((double)time_w/CLOCKS_PER_SEC)/num_w);
        fprintf(res, "[%s] %d reads %d threads avg time [ms] = %lf \n", (char *)arg, num_r, THREAD_NUM, 100*((double)time_r/CLOCKS_PER_SEC)/num_r);
        fclose(res);

        printf("[%s] %d writes %d threads avg time [ms] = %lf \n", (char *)arg, num_w, THREAD_NUM, 100*((double)time_w/CLOCKS_PER_SEC)/num_w);
        printf("[%s] %d reads %d threads avg time [ms] = %lf \n", (char *)arg, num_r, THREAD_NUM, 100*((double)time_r/CLOCKS_PER_SEC)/num_r);
	
}

int main(int argc, char **argv) {
	
	if (argc < 5) {
		printf("Usage <num_threads> <num_operations> <[mkfifo/mailslot]> <device_name> <major> <minor>\n");
		return -1;
	}
	
	THREAD_NUM = atoi(argv[1]);
	OPERATION_NUM = atoi(argv[2]);
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

                for (t=0; t < THREAD_NUM; t++) {
                        pthread_create(&threads[t], NULL, execute, (void *) name);
                }
                printf("%d threads created\n", THREAD_NUM);
                for (t=0; t < THREAD_NUM; t++) {
                        pthread_join(threads[t], NULL);
                }
                printf("%d threads completed\n", THREAD_NUM);
			
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
			
		for (t=0; t < THREAD_NUM; t++) {
                       	pthread_create(&threads[t], NULL, execute, (void *) name);
               	}
		printf("%d threads created\n", THREAD_NUM);
		for (t=0; t < THREAD_NUM; t++) {
                       	pthread_join(threads[t], NULL);
               	}
		printf("%d threads completed\n", THREAD_NUM);
		
		remove(name);
	} else {
		printf("Driver %s not supported\n", driver);	
		return -1;
	}
	return 0;
}
