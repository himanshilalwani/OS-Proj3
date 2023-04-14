#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

#define SEM_MUTEX "/mutex_sem"
#define SEM_WRITE "/write_sem"
#define SEM_READ "/read_sem"

#define NUM_READERS 3    // this is readers at a time
#define MAX_LINES_READ 9 // this is maximum number of lines a reader can read at a time

struct analytics
{
    int reader_count;         // number of readers processed
    int avg_duration_readers; // average duration of readers
    int writer_count;         // number of writes processed
    int avg_duration_writers; // average duration of writers
    int max_delay;            // maximum delay recorded for starting the work of either a reader or a writer.
    int num_records;          // number of records accessed / modified
};

typedef struct
{
    int studentID;
    char lastName[20];
    char firstName[20];
    float g1;
    float g2;
    float g3;
    float g4;
    float g5;
    float g6;
    float g7;
    float g8;
    float GPA;
} student;

void create_semaphore(const char *name, sem_t **sem, int value)
{
    *sem = sem_open(name, O_CREAT | O_EXCL, 0666, value);
    if (*sem == SEM_FAILED)
    {
        perror("Failed to create semaphore");
        exit(EXIT_FAILURE);
    }
}

void remove_semaphore(const char *name, sem_t *sem)
{
    if (sem_close(sem) == -1)
    {
        perror("Failed to close semaphore");
        exit(EXIT_FAILURE);
    }
    if (sem_unlink(name) == -1)
    {
        perror("Failed to unlink semaphore");
        exit(EXIT_FAILURE);
    }
}

int main()
{
    // sem_unlink("resource_control");
    // sem_unlink("synchronization");
    // sem_unlink("queue");
    // sem_unlink("analytic_semaphore");

    sem_t *sem_mutex;
    sem_t *sem_write;
    sem_t *sem_read;

    int shmid1;
    key_t key = 1234;
    struct analytics *a;

    // Create shared memory segment 1 with ID 1234
    if ((shmid1 = shmget(key, sizeof(struct analytics), IPC_CREAT | 0666)) == -1)
    {
        perror("Failed to create shared memory");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory segment
    if ((a = shmat(shmid1, NULL, 0)) == (void *)-1)
    {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    int shmid2;
    key_t key2 = 9999;
    student *data;

    // Create shared memory segment 2 with ID 9999
    if ((shmid2 = shmget(key2, sizeof(student), IPC_CREAT | 0666)) == -1)
    {
        perror("Failed to create shared memory");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory segment
    if ((data = shmat(shmid2, NULL, 0)) == (void *)-1)
    {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    // Read product details from CSV file
    FILE *fp;
    char line[1024];
    char *token;
    int i = 0;

    fp = fopen("students.csv", "r");
    if (fp == NULL)
    {
        perror("Failed to open CSV file");
        exit(EXIT_FAILURE);
    }

    // load data in the student struct
    while (fgets(line, 1024, fp))
    {
        token = strtok(line, ",");
        data[i].studentID = atoi(token);

        token = strtok(NULL, ",");
        strcpy(data[i].lastName, token);

        token = strtok(NULL, ",");
        strcpy(data[i].firstName, token);

        token = strtok(NULL, ",");
        data[i].g1 = atof(token);

        token = strtok(NULL, ",");
        data[i].g2 = atof(token);

        token = strtok(NULL, ",");
        data[i].g3 = atof(token);

        token = strtok(NULL, ",");
        data[i].g4 = atof(token);

        token = strtok(NULL, ",");
        data[i].g5 = atof(token);

        token = strtok(NULL, ",");
        data[i].g6 = atof(token);

        token = strtok(NULL, ",");
        data[i].g7 = atof(token);

        token = strtok(NULL, ",");
        data[i].g8 = atof(token);

        token = strtok(NULL, ",");
        data[i].GPA = atof(token);
        i++;
    }

    fclose(fp);
    // initialize struct values
    a->reader_count = 0;
    a->avg_duration_readers = 0;
    a->writer_count = 0;
    a->avg_duration_writers = 0.0;
    a->max_delay = 0;
    a->num_records = 0;

    // Fork writer process
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        // Writer processs
        // Wait for write semaphore
        // if (sem_wait(sem_write) == -1)
        // {
        //     perror("Failed to wait on write semaphore");
        //     exit(EXIT_FAILURE);
        // }
        int random_time = (rand() % 5) + 1;
        char random_time_str[10];
        sprintf(random_time_str, "%d", random_time);

        char key2_str[10];
        sprintf(key2_str, "%d", key2);

        int random_record = rand() % i; // generate a random number between 0 and i-1
        char random_record_str[10];
        sprintf(random_record_str,"%d",random_record);

        if (execlp("./writer", "./writer", "-f", "students.csv", "-l", random_record_str, "-d", random_time_str, "-s", key2_str, NULL) < 0)
        {
            perror("Exec Error");
            exit(EXIT_FAILURE);
        }
        // // Signal write semaphore
        // if (sem_post(sem_write) == -1)
        // {
        //     perror("Failed to signal on write semaphore");
        //     exit(EXIT_FAILURE);
        // }
        sleep(1);
        exit(EXIT_SUCCESS);
    }
    if (pid > 0)
    {
        // Fork multiple reader processes
        for (int j = 0; j < NUM_READERS; j++)
        {
            pid_t reader_pid = fork();

            if (reader_pid < 0)
            {
                perror("Failed to fork");
                exit(EXIT_FAILURE);
            }

            if (reader_pid == 0)
            {
                // Reader process

                // Read student details
                srand(time(NULL));
                int x = rand() % MAX_LINES_READ; // x is the random num of records the reader j can read
                int *list = malloc(x * sizeof(int));
                int num_to_read = 0;

                // for random line number
                srand(time(NULL));

                // update the list to include all the line numbers to read
                while (num_to_read < x)
                {
                    int line_num = rand() % (i - 1);
                    int exists = 0;
                    // Check if the number already exists in the list
                    for (int k = 0; k < num_to_read; k++)
                    {
                        if (list[k] == line_num)
                        {
                            exists = 1;
                            break;
                        }
                    }
                    // If the number is not in the list, add it
                    if (!exists)
                    {
                        list[num_to_read] = line_num;
                        num_to_read++;
                    }
                }

                // Create a string to combine all values in list separated by comma
                char list_string[100]; // assuming the maximum length of the resulting string is 100 characters
                int index = 0;

                for (int i = 0; i < num_to_read; i++)
                {
                    index += sprintf(list_string + index, "%d,", list[i]);
                }
                // Remove the trailing comma
                if (num_to_read > 0)
                {
                    list_string[index - 1] = '\0';
                }

                int random_time = (rand() % 5) + 1;
                char random_time_str[10];
                sprintf(random_time_str, "%d", random_time);

                char key2_str[10];
                sprintf(key2_str, "%d", key2);
                // printf("List String: %s \n", list_string);
                // printf("Time: %s\n", random_time_str);
                // invoke reader
                printf("Reader Number %d Executing:", j);
                if (execlp("./reader", "./reader", "-f", "students.csv", "-l", list_string, "-d", random_time_str, "-s", key2_str, NULL) < 0)
                {
                    perror("Exec Error");
                    exit(EXIT_FAILURE);
                }

                free(list);

                exit(EXIT_SUCCESS);
            }
        }
    }
}