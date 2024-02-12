#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>

#define FRAME_SIZE 512
#define NUM_FRAMES 1000
#define MEMORY_SIZE (FRAME_SIZE * NUM_FRAMES)

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

char memory[MEMORY_SIZE];
char temp_memory[MEMORY_SIZE];
int frame_table[NUM_FRAMES];

pthread_mutex_t memory_lock;

data_message_t processes[NUM_FRAMES];
int num_processes = 0;

void *scheduler(void *arg)
{
    key_t key = ftok(".", -1);
    int msgid = msgget(key, 0666 | IPC_CREAT);

    for (int i = 0; i < NUM_FRAMES; i++)
    {
        frame_table[i] = -1;
    }

    while (1)
    {
        data_message_t data_message;
        int total_frames = 0;

        while (msgrcv(msgid, &data_message, sizeof(data_message), 1, IPC_NOWAIT) != -1)
        {
            for (int i = 0; i < FRAME_SIZE; i++)
            {
                temp_memory[total_frames * FRAME_SIZE + i] = data_message.page_data[i];
            }

            total_frames++;

            pthread_mutex_lock(&memory_lock);

            int success = 0;

            int free_frames = 0;
            for (int i = 0; i < NUM_FRAMES; i++)
            {
                if (frame_table[i] == -1)
                {
                    free_frames++;
                }
                if (free_frames == data_message.page_table_size)
                {
                    break;
                }
            }
            if (free_frames < data_message.page_table_size)
            {
                pthread_mutex_unlock(&memory_lock);
                success = 0;
            }
            else{
                int frame_index = 0;
                if (data_message.stop == 1)
                {
                    int frame_index = 0;
                    for (int i = 0; i < NUM_FRAMES && frame_index < data_message.page_table_size; i++)
                    {
                        if (frame_table[i] == -1)
                        {
                            frame_index++;
                        }
                    }
                }

                pthread_mutex_unlock(&memory_lock);

                success = 1;
            }

            if (data_message.stop == 1)
            {
                if (success == 0)
                {
                    kill(data_message.pid, SIGKILL);
                    printf("%s istemcisi, %s dosyasi icin %d pagelik yeterli frame olmadigi icin sisteme alinmadi\n", data_message.process_name, data_message.file_name, data_message.page_table_size);
                    continue;
                }
                else
                {
                    pthread_mutex_lock(&memory_lock);

                    int temp_memory_frame_index = 0;
                    for (int i = 0; i < NUM_FRAMES; i++)
                    {
                        if (frame_table[i] == -1 && temp_memory_frame_index < total_frames)
                        {
                            for (int j = 0; j < FRAME_SIZE; j++)
                            {
                                memory[i * FRAME_SIZE + j] = temp_memory[temp_memory_frame_index * FRAME_SIZE + j];
                            }
                            temp_memory_frame_index++;
                        }
                    }

                    pthread_mutex_unlock(&memory_lock);

                    int frame_index = 0;
                    for (int i = 0; i < NUM_FRAMES && frame_index < data_message.page_table_size; i++)
                    {
                        if (frame_table[i] == -1)
                        {
                            frame_table[i] = data_message.pid;
                            frame_index++;
                        }
                    }
                    printf("%s istemcisi, %s dosyasi icin %d page table ile sisteme alindi\n", data_message.process_name, data_message.file_name, data_message.page_table_size);
                    processes[num_processes++] = data_message;
                }
                total_frames = 0;
            }
        }

        key_t key2 = ftok("..", -1);
        int msgid2 = msgget(key2, 0666 | IPC_CREAT);

        request_t request;
        if (msgrcv(msgid2, &request, sizeof(request), 1, IPC_NOWAIT) != -1)
        {
            int pid = request.pid;
            int page_number = request.page;

            data_message_t *process = NULL;
            for (int i = 0; i < num_processes; i++)
            {
                if (processes[i].pid == pid)
                {
                    process = &processes[i];
                    break;
                }
            }
            if (process == NULL)
            {
                printf("%s istemcisi bulunamadi\n", process->process_name);
                continue;
            }

            int frame_index = -1;
            for (int i = 0; i < NUM_FRAMES; i++)
            {
                if (frame_table[i] == pid)
                {
                    frame_index++;
                    if (frame_index == page_number)
                    {
                        frame_index = i;
                        break;
                    }
                }
            }

            if (page_number > frame_index || page_number < 0)
            {
                frame_index = -1;
            }

            if (frame_index == -1)
            {
                pthread_mutex_lock(&memory_lock);

                for (int i = 0; i < NUM_FRAMES; i++)
                {
                    if (frame_table[i] == pid)
                    {
                        frame_table[i] = -1;
                    }
                }

                pthread_mutex_unlock(&memory_lock);

                printf("%s istemcisinde %d numarali sayfaya erisim istegi reddedildi, process sonlandi, kaynaklari geri alindi\n", process->process_name, page_number);
                continue;
            }

            response_t response;
            response.mtype = pid;
            memcpy(response.page_data, memory + frame_index * FRAME_SIZE, FRAME_SIZE);

            printf("\n\n%s %d. Sayfa İcerigi:\n\n%s\n\n", request.process_name, page_number, response.page_data);
            
            key_t key3 = ftok("../..", -1);
            int msgid3 = msgget(key3, 0666 | IPC_CREAT);
            msgsnd(msgid3, &response, sizeof(response), 0);
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t scheduler_thread;

    pthread_mutex_init(&memory_lock, NULL);
    
    pthread_create(&scheduler_thread, NULL, scheduler, NULL);

    pthread_join(scheduler_thread, NULL);

    return 0;
}
