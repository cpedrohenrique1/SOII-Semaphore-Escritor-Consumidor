/* The second program is the producer and allows us to enter data for consumers.
 It's very similar to shm1.c and looks like this. */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/shm.h>
#include <sys/sem.h>

#include "shm_com.h"
#include "semun.h"

static int semaphore_down(int);
static int semaphore_up(int);
static int set_semvalue(int);
static void del_semvalue(int);

static int semid_cheio;
static int semid_vazio;

int main()
{
    int running = 1;
    void *shared_memory = (void *)0;
    struct shared_use_st *shared_stuff;
    char buffer[BUFSIZ];
    int shmid;

    semid_cheio = semget((key_t)3030, 1, 0666 | IPC_CREAT);
    semid_vazio = semget((key_t)3060, 1, 0666 | IPC_CREAT);

    shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666 | IPC_CREAT);

    set_semvalue(semid_cheio);

    shared_memory = shmat(shmid, (void *)0, 0);

    printf("Memory attached at %X\n", (int)shared_memory);

    shared_stuff = (struct shared_use_st *)shared_memory;
    shared_stuff->written_by_you = 0;
    shared_stuff->pos_p = 0;

    while (running)
    {
        while (!semaphore_down(semid_vazio))
        {
            sleep(1);
            printf("waiting for client...\n");
        }
        printf("Enter some text: ");
        fgets(buffer, BUFSIZ, stdin);
        int pos = shared_stuff->pos_p;
        strncpy(shared_stuff->some_text[pos], buffer, TEXT_SZ);
        shared_stuff->pos_p = (pos + 1) % 10;
        shared_stuff->written_by_you = 1;
        semaphore_up(semid_cheio);

        if (strncmp(buffer, "end", 3) == 0)
        {
            running = 0;
        }
    }

    shmdt(shared_memory);
    del_semvalue(semid_cheio);
    del_semvalue(semid_vazio);
    exit(EXIT_SUCCESS);
}

static int set_semvalue(int sem_id)
{
    union semun sem_union;

    sem_union.val = 0;
    if (semctl(sem_id, 0, SETVAL, sem_union) == -1) return(0);
    return(1);
}

/* The del_semvalue function has almost the same form, except the call to semctl uses
 the command IPC_RMID to remove the semaphore's ID. */

static void del_semvalue(int sem_id)
{
    union semun sem_union;
    
    if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
        fprintf(stderr, "Failed to delete semaphore\n");
}

static int semaphore_down(int sem_id)
{
    struct sembuf sem_b;

    sem_b.sem_num = 0;
    sem_b.sem_op = -1; /* P() */
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id, &sem_b, 1) == -1)
    {
        fprintf(stderr, "semaphore_p failed\n");
        return (0);
    }
    return (1);
}

static int semaphore_up(int sem_id)
{
    struct sembuf sem_b;

    sem_b.sem_num = 0;
    sem_b.sem_op = 1; /* V() */
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id, &sem_b, 1) == -1)
    {
        fprintf(stderr, "semaphore_v failed\n");
        return (0);
    }
    return (1);
}