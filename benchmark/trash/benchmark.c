
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

void *writer(void *arg) {
	int fd = open((char * )arg, O_WRONLY);
        char *message = "abcdefghijklmnopqrstuvwxyz\0";    
        int i;
       
	printf("Writing...\n");	
	clock_t begin = clock();
	
	clock_t interrupt_begin = begin;	
	clock_t interrupted = 0;	

	for (i = 0; i < OPERATION_NUM; i++) { 
                printf("Written\n");
	        if (write(fd, message, strlen(message) == -1) {
			interrupt_begin = clock();
			// if full make 5 reads
			int j;
			for (j=0; j<5; j++) {
				reader(arg);
			}
			interrupted += clock() - interrupt_begin;
		}
	}
	
	clock_t end = clock();
	double time_spent = (double)(end - begin - interrupted) / CLOCKS_PER_SEC;

	FILE *res = fopen("res.txt", "a");
	fprintf(res, "[%s] %d writes %d threads avg time [ms] = %lf \n", (char *)arg, OPERATION_NUM, THREAD_NUM, 100*time_spent/OPERATION_NUM);
	fclose(res);
	
	printf("[%s] Average time spent for write operation: %lf\n", (char *)arg, 100*time_spent/OPERATION_NUM);
}

void *reader(void *arg) {
	int fd = open((char *)arg, O_RDONLY);
        char buff[256];
	int i = 0;

	printf("Reading...\n");	
        clock_t begin = clock();

	while (read(fd, buff, 256) != 0) {
                printf("Read %s\n", buff);
		memset(&buff[0], 0, sizeof(buff));
		i++;
        }
	clock_t end = clock();    
	double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
		
	FILE *res = fopen("res.txt", "a");
	fprintf(res, "[%s] %d reads %d threads avg time [ms] = %lf \n", (char *)arg, i, THREAD_NUM, 100*time_spent/i);
	fclose(res);
	
	printf("[%s] Average time spent for read operation [ms]: %lf\n", (char *)arg, 100*time_spent/i);
}

int create_threads(char *name) {
/*	int status = 0;
	
        pid_t child;
        //int w = open(name, O_RDWR);
        printf("ooo\n");
	if ((child = fork()) == 0) {
        	printf("aaao\n");
                writer((void *) name);
                return 0;
        }
//        waitpid(child, &status, 0); 
        printf("Written\n");
        reader((void *) name);
	return 0;
*/
		int t;
                pthread_t threads[THREAD_NUM];
		printf("Creating %d threads...\n", THREAD_NUM);
                for (t=0; t < THREAD_NUM; t++) {
                        pthread_create(&threads[t],  NULL, writer, (void *) name);
                }
		printf("%d writers created\n", THREAD_NUM);
/*                for (t=0; t < THREAD_NUM; t++) {
                        pthread_join(threads[t],  NULL);
                }
		sleep(10);
		printf("%d writers completed\n", THREAD_NUM);
*/
                for (t=0; t < THREAD_NUM; t++) {
                        pthread_create(&threads[t], NULL, reader, (void *) name);
                }
		printf("%d readers created\n", THREAD_NUM);

}

int main(int argc, char **argv) {
	
	if (argc < 6) {
		printf("Usage <num_threads> <num_operations> <[r/w]> <[mkfifo/mailslot]> <device_name> <major> <minor>\n");
		return -1;
	}
	
	THREAD_NUM = atoi(argv[1]);
	OPERATION_NUM = atoi(argv[2]);
	char *OPERATION = argv[3];
	char *driver = argv[4];

	char *prefix = "/dev/";
        char *name = (char *) malloc(strlen(prefix) + strlen(argv[5]) + 1); 
        strcpy(name, prefix);
        strcat(name, argv[5]);
	
	if (!strcmp(driver, "mkfifo")) {
		if (mkfifo(name, O_CREAT | 0666) == -1) {
                	printf("[ERROR] in mkfifo creation.\n");
                	return -1; 
        	}
		create_threads(name);	
		
		remove(name);
	} else if (!strcmp(driver, "mailslot")) {
		int major = atoi(argv[6]);
		int minor = atoi(argv[7]);
		if (mknod(name, 0666 | S_IFCHR, makedev(major, minor)) == -1) {
                	printf("[ERROR] in mailslot creation.\n");
                	return -1; 	
		}
		//create_threads(name);	
		
		int t;
                pthread_t threads[THREAD_NUM];
		printf("Creating %d threads...\n", THREAD_NUM);
		if (!strcmp(OPERATION, "r")) {
			for (t=0; t < THREAD_NUM; t++) {
                        	pthread_create(&threads[t], NULL, reader, (void *) name);
                	}
			printf("%d readers created\n", THREAD_NUM);
			for (t=0; t < THREAD_NUM; t++) {
                        	pthread_join(threads[t], NULL);
                	}
			printf("%d readers completed\n", THREAD_NUM);
		} else {
                	for (t=0; t < THREAD_NUM; t++) {
                        	pthread_create(&threads[t],  NULL, writer, (void *) name);
                	}
			printf("%d writers created\n", THREAD_NUM);
			for (t=0; t < THREAD_NUM; t++) {
                        	pthread_join(threads[t], NULL);
                	}
			printf("%d writers ccompleted\n", THREAD_NUM);
		}
		remove(name);
	} else {
		printf("Driver %s not supported\n", driver);	
		return -1;
	}
	return 0;
}
