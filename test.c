// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/types.h>
// #include <sys/ipc.h>
// #include <sys/shm.h>

// #define NUM_RECORDS 500
// #define SHM_SIZE 1024

// typedef struct {
//     long id;
//     char lastname[20];
//     char firstname[20];
//     float grades[8];
//     float gpa;
// } Student;

// int main() {
//     FILE *fp;
//     int i, j, shmid;
//     key_t key;
//     Student *shm, *s;
//     int sd = sizeof(Student) * NUM_RECORDS;
//     printf("sd %d\n", sd);


//     // Generate a unique key for the shared memory segment
//     key = ftok("Dataset-500.bin", 'R');
//     printf("key: %d\n",key);
//     // Create the shared memory segment
//     shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);

//     // Attach to the shared memory segment
//     shm = (Student*)shmat(shmid, NULL, 0);
//     printf("Memory attached\n");

//     // Read data from the binary file and store it in shared memory
//     fp = fopen("Dataset-500.bin", "rb");
//     int x=1;
//     while (fread(s = shm++, sizeof(Student), 1, fp) == 1) {
//         // Increment the pointer to the next available shared memory slot
//         if ((char*)shm - (char*)s == SHM_SIZE - sizeof(Student)) break;
//         printf("read %d\n",x);
//         x++;
//     }
//     printf("all file done\n");
//     fclose(fp);

//     // Detach from the shared memory segment
//     shmdt(shm);
//     printf("Memory detached\n");

//     return 0;
// }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    FILE *fp;
    int n = 0;  // number of students in the file
    int shm_id;
    Student *array;

    // open the file
    fp = fopen("Dataset-50000.bin", "rb");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    // count the number of students in the file
    fseek(fp, 0L, SEEK_END);
    size_t size = ftell(fp);
    n = size / sizeof(Student);
    printf("n: %d\n",n);
    rewind(fp);

    // compute the size of the shared memory segment
    size_t shm_size = n * sizeof(Student);
    printf("shm_Size: %d\n",shm_size);

    // allocate the shared memory segment
    key_t key = 1234;  // choose a unique key
    // key_t key = ftok("Dataset-50000.bin", 'R');
    printf("key: %d\n",key);
    shm_id = shmget(key, shm_size, IPC_CREAT | 0666);
    printf("shmid: %d\n", shm_id);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    // attach to the shared memory segment
    array = (Student *) shmat(shm_id, NULL, 0);
    if (array == (Student *) -1) {
        perror("shmat");
        exit(1);
    }

    // read the file into the shared memory segment
    int i = 1;
    while (fread(&array[i], sizeof(Student), 1, fp) == 1) {
        printf("read line: %d\n",i);
        i++;
    }

    // detach from the shared memory segment
    shmdt(array);

    // clean up
    fclose(fp);

    return 0;
}
