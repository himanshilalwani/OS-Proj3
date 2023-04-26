#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

typedef struct {
    long id;
    char lastname[20];
    char firstname[20];
    float grades[8];
    float gpa;
} Student;

int main() {
    int i, j, shmid;
    key_t key;
    Student *shm, *s;

    // Generate the same unique key for the shared memory segment
    // key = ftok("Dataset-500.bin", 'R');
    // printf("key: %d\n",key);

    // Find the existing shared memory segment
    shmid = shmget(1234, 0, 0666);

    // Attach to the shared memory segment
    shm = (Student*)shmat(shmid, NULL, 0);
    printf("memorry attached\n");

    // Loop through the shared memory segment and print the data
    s = shm;
    while (s->id > 0) {
        printf("%ld %s %s ", s->id, s->lastname, s->firstname);
        for (j = 0; j < 8; j++) printf("%.2f ", s->grades[j]);
        printf("%.2f\n", s->gpa);
        s++;
    }

    // Detach from the shared memory segment
    shmdt(shm);

    return 0;
}
