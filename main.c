#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>

#define SEM_MUTEX "/mutex_sem"
#define SEM_WRITE "/write_sem"
#define SEM_READ "/read_sem"

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
    int g1;
    int g2;
    int g3;
    int g4;
    int g5;
    int g6;
    int g7;
    int g8;
    int GPA;
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
        data[i].g1 = atoi(token);

        token = strtok(NULL, ",");
        data[i].g2 = atoi(token);

        token = strtok(NULL, ",");
        data[i].g3 = atoi(token);

        token = strtok(NULL, ",");
        data[i].g4 = atoi(token);

        token = strtok(NULL, ",");
        data[i].g5 = atoi(token);

        token = strtok(NULL, ",");
        data[i].g6 = atoi(token);

        token = strtok(NULL, ",");
        data[i].g7 = atoi(token);

        token = strtok(NULL, ",");
        data[i].g8 = atoi(token);

        token = strtok(NULL, ",");
        data[i].GPA = atoi(token);
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

    // Create semaphores
    create_semaphore(SEM_MUTEX, &sem_mutex, 1);
    create_semaphore(SEM_WRITE, &sem_write, 1);
    create_semaphore(SEM_READ, &sem_read, 1);

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
        if (sem_wait(sem_write) == -1)
        {
            perror("Failed to wait on write semaphore");
            exit(EXIT_FAILURE);
        }

        // Signal write semaphore
        if (sem_post(sem_write) == -1)
        {
            perror("Failed to signal on write semaphore");
            exit(EXIT_FAILURE);
        }
        sleep(1);
    }

    exit(EXIT_SUCCESS);
}