
#ifndef DEVICE_FILE_SIZE
#define DEVICE_FILE_SIZE 256
#endif

#ifndef MAXIMUM_MESSAGE_SIZE
#define MAXIMUM_MESSAGE_SIZE 256
#endif

#ifndef MAXIMUM_MAILSLOT_SIZE
#define MAXIMUM_MAILSLOT_SIZE 256*100
#endif

typedef struct mailslot_msg_t {
        struct mailslot_msg_t *next;
        char data[MAXIMUM_MESSAGE_SIZE];
        unsigned int size;
} mailslot_msg_t;

typedef struct mailslot_t {
        mailslot_msg_t *first;
        mailslot_msg_t *last;
        size_t size;
} mailslot_t;

void print_queue(int);
void initialize(int minor);
mailslot_msg_t *newMessage(char *, size_t);
int is_empty(int);
int is_full(int, size_t);
int enqueue(int, char *);
char *dequeue(int, mailslot_msg_t *);
char *dequeueByLen(int, ssize_t);
