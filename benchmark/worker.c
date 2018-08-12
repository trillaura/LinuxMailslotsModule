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
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define DEBUG if (1)

void worker_writer(char *name) {

        int fd = open(name, O_WRONLY);
	if (fd == -1) {
		printf("Error opening file.\n");
		return;
	}
        char *message = "a";
        int j=0, i=0;
        while (1) {
		if (i % 40 == 0)
			printf("\rExecuting worker writer until you press Ctrl+C");
        	else if (i % 41 == 1) 
			printf("\rExecuting worker writer until you press Ctrl+C.");
        	else if (i % 42 == 2) 
			printf("\rExecuting worker writer until you press Ctrl+C..");
	       	else 
			printf("\rExecuting worker writer until you press Ctrl+C...");
	
		if (write(fd, message, strlen(message)) != -1)
                        ++j;
                ++i;
		if (i % 100000 == 0)
			printf("Written %d times\r", i);
        }
}

void worker_reader(char *name) {

        int fd = open(name, O_RDONLY);
	if (fd == -1) {
		printf("Error opening file.\n");
		return;
	}
        int j=0, i=0;
        while (1) {
		if (i % 40 == 0)
			printf("\rExecuting worker reader until you press Ctrl+C");
        	else if (i % 41 == 1) 
			printf("\rExecuting worker reader until you press Ctrl+C.");
        	else if (i % 42 == 2) 
			printf("\rExecuting worker reader until you press Ctrl+C..");
	       	else 
			printf("\rExecuting worker reader until you press Ctrl+C...");
                char buff[256];
                if (read(fd, buff, 256) > 0)
                        ++j;
		++i;        
		if (i % 100000 == 0)
			printf("Read %d times\r", i);
	}
}


int main(int argc, char **argv) {
	
        if (argc < 4) {
                printf("Usage <[reader/writer]> <[mkfifo/mailslot]> <device_name> *<major> *<minor>\n");
                return -1;
        }
		
	char *prefix = "/dev/";
        char *name = (char *) malloc(strlen(prefix) + strlen(argv[3]) + 1);
        strcpy(name, prefix);
        strcat(name, argv[3]);

        char *worker_type = argv[1];
        char *driver = argv[2];

        if (!strcmp(driver, "mkfifo")) {
                if (mkfifo(name, O_CREAT | 0666) == -1) {
                        printf("[ERROR] in mkfifo creation.\n");
                        return -1;
                }
        } else if (!strcmp(driver, "mailslot")) {
                int major = atoi(argv[4]);
                int minor = atoi(argv[5]);
                if (mknod(name, 0666 | S_IFCHR, makedev(major, minor)) == -1) {
                        printf("[ERROR] in mailslot creation.\n");
                        return -1;
                }
	} else {
                printf("Driver %s not supported\n", driver);
                return -1;
        }
	
	if (!strcmp(worker_type, "writer")) 
        	worker_writer(name);
        else 
        	worker_reader(name);
       
	return 0;
}
