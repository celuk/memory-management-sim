#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define PAGE_SIZE 512
#define CHARMAX 100

typedef struct
{
    long mtype;
    int pid;
    char process_name[CHARMAX];
    char file_name[CHARMAX];
    int page_table_size;
    char page_data[PAGE_SIZE];
    int stop;
} data_message_t;

typedef struct
{
    long mtype;
    int pid;
    int page;
    char process_name[CHARMAX];
} request_t;

typedef struct
{
    long mtype;
    char page_data[PAGE_SIZE];
} response_t;

typedef struct
{
    long mtype;
    int pid;
    int terminate;
} term_t;

char *process_name;
char *input_file;
int page_table_size;

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Kullanim sekli: %s <istemci_ismi> <dosya_ismi>\n", argv[0]);
        return 1;
    }

    process_name = argv[1];
    input_file = argv[2];

    FILE *file = fopen(input_file, "r");
    if (file == NULL)
    {
        perror("Dosya acilamadi!\n");
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    printf("%d bytes \n", file_size);

    page_table_size = file_size / PAGE_SIZE;
    if (file_size % PAGE_SIZE != 0)
    {
        page_table_size++;
    }

    page_table_size = file_size / PAGE_SIZE;
    if (file_size % PAGE_SIZE != 0)
    {
        page_table_size++;
    }

    char *file_data = malloc(file_size);
    fread(file_data, 1, file_size, file);

    int pid = getpid();

    key_t key = ftok(".", -1);
    int msgid = msgget(key, 0666 | IPC_CREAT);

    data_message_t data_message;
    data_message.mtype = 1;
    data_message.pid = pid;
    data_message.page_table_size = page_table_size;
    memcpy(data_message.process_name, process_name, CHARMAX);
    memcpy(data_message.file_name, input_file, CHARMAX);

    data_message.stop = 0;

    int i = 0;
    for (i = 0; i < page_table_size - 1; i++)
    {
        memcpy(data_message.page_data, file_data + i * PAGE_SIZE, PAGE_SIZE);
        msgsnd(msgid, &data_message, sizeof(data_message), 0);
    }

    memcpy(data_message.page_data, file_data + i * PAGE_SIZE, PAGE_SIZE);
    data_message.stop = 1;
    msgsnd(msgid, &data_message, sizeof(data_message), 0);

    free(file_data);
    fclose(file);

    while (1)
    {
        printf("Sayfa numarasini girin (0-%d arasinda): ", page_table_size - 1);
        int page_number;
        scanf("%d", &page_number);

        key_t key2 = ftok("..", -1);
        int msgid2 = msgget(key2, 0666 | IPC_CREAT);

        request_t request;
        request.mtype = 1;
        request.pid = getpid();
        request.page = page_number;
        memcpy(request.process_name, process_name, CHARMAX);
        msgsnd(msgid2, &request, sizeof(request), 0);

        if (page_number < 0 || page_number >= page_table_size)
        {
            printf("Hatali sayfa numarasi!\n");
            exit(1);
        }

        key_t key3 = ftok("../..", -1);
        int msgid3 = msgget(key3, 0666 | IPC_CREAT);

        response_t response;
        msgrcv(msgid3, &response, sizeof(response), getpid(), 0);
        printf("\n\n%s %d. Sayfa İcerigi:\n\n%s\n\n", process_name, page_number, response.page_data);
    }

    return 0;
}
