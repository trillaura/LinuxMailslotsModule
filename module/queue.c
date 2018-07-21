
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

#define DEBUG if(1)

static mailslot_t mailslot[DEVICE_FILE_SIZE];

void print_queue(int i) {
    
        mailslot_msg_t *head = mailslot[i].first;
        mailslot_msg_t *tail = mailslot[i].last;
    
	printf("Queue:\n");
        while (head != tail) {
                printf(", "); 
                printf("msg: %s size: %u", head->data, head->size);
                head = head->next;
        }
        printf("\n");
}

void initialize(int minor) {
	
	mailslot[minor].size = 0;
	mailslot[minor].first = NULL;
	mailslot[minor].last = NULL;
}

mailslot_msg_t *newMessage(char *text, size_t len) {
	
	mailslot_msg_t *new = (mailslot_msg_t *) malloc(sizeof(mailslot_msg_t));
	
	if (new == NULL) {
		DEBUG printf("Error in malloc.\n");
		return NULL;
	}

	strcpy(new->data, text);
	new->size = len;
	new->next = NULL;
	
	return new;
}

int is_empty(int minor) {
	return mailslot[minor].last == NULL;
}

int is_full(int minor, size_t len) {
	return mailslot[minor].size + len > MAXIMUM_MAILSLOT_SIZE; 
}

int enqueue(int minor, char *new_msg) {
	
	size_t len = strlen(new_msg);

        if (len > MAXIMUM_MESSAGE_SIZE) {
                DEBUG printf("Message too large.\n");
                return -1;
        } 
	
	if (is_full(minor, len)) {
		DEBUG printf("Full mailslot.\n");
		return -1;
	}
	
	mailslot_msg_t *new = newMessage(new_msg, len);

	if (new == NULL)
		return -1;

	if (!is_empty(minor)) {
		mailslot[minor].last->next = new;
		mailslot[minor].last = new;
	} else {
		DEBUG printf("Empty mailslot.\n");
		mailslot[minor].first = mailslot[minor].last = new;
	}
	mailslot[minor].size += len;

	DEBUG print_queue(minor);
	
	return 0;
}

char *dequeue(int minor, mailslot_msg_t *head) {
	
	DEBUG printf("Before...");
	DEBUG print_queue(minor);
	
	char *message = head->data;
	mailslot[minor].first = mailslot[minor].first->next;
	mailslot[minor].size--;
	free(head);
	
	// TODO sveglia writer bloccati perché full
		
	DEBUG printf("After...");
	DEBUG print_queue(minor);
	
	return message;
}

char *dequeueByLen(int minor, ssize_t len) {

	if (is_empty(minor)) {
		DEBUG printf("Empty mailslot.\n");
		return (char *) '\0';	
	}

	mailslot_msg_t *head = mailslot[minor].first;

	if (strlen(head->data) <= len) {
		return dequeue(minor, head);
	} else {
		DEBUG print_queue(minor);	
		return NULL;
	}
}

int main(int argc, char **argv) {
	
	initialize(0);
	
	dequeueByLen(0, 0);
	
	enqueue(0, "hello1");
	enqueue(0, "hello2");
	enqueue(0, "hello3");
	enqueue(0, "hello4");

	dequeueByLen(0, strlen("hello2\0"));
	dequeueByLen(0, sizeof("hello1"));
	dequeueByLen(0, sizeof("hello1ffff"));
		
	return 0;
}	
