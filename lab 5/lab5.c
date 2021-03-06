#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NTHREADS  5
#define M1 (1)
#define M2 (2)
#define M3 (4)
#define M4 (8)
#define M5 (16)

#define PATTERN "%c%c%c%c%c"

#define REP(num)  \
(num & 0x10 ? '1' : '0'), \
(num & 0x08 ? '1' : '0'), \
(num & 0x04 ? '1' : '0'), \
(num & 0x02 ? '1' : '0'), \
(num & 0x01 ? '1' : '0') 

#define BINARY(message, x) \
message PATTERN" " \
BYTE_TO_BINARY_PATTERN"\n", \
BYTE_TO_BINARY(x)

void* A(void* t);
void* B(void* t);
void* C(void* t);
void* D(void* t);
void* E(void* t);

int mask = 0;
pthread_mutex_t x_mutex;
pthread_cond_t x_cond;

void* A(void* t)
{
    pthread_mutex_lock(&x_mutex);
    pthread_cond_signal(&x_cond);
    printf("A\n");
    while(!(mask == (M2|M3|M4|M5)))
    {
        printf("A block\n");
        pthread_cond_wait(&x_cond, &x_mutex);
    }
    mask |= M1;
    printf("Volte sempre!\n");
    pthread_cond_signal(&x_cond);
    pthread_mutex_unlock(&x_mutex);
    pthread_exit(NULL);
}

void* B(void* t)
{
    printf("B\n");
    pthread_mutex_lock(&x_mutex);
    while(!(mask & M5))
    {
        printf("B block\n");
        pthread_cond_wait(&x_cond, &x_mutex);
    }
    mask |= M2;
    printf("Fique a vontade.\n");
    pthread_cond_signal(&x_cond);
    pthread_mutex_unlock(&x_mutex);
    pthread_exit(NULL);
}

void* C(void* t)
{
    printf("C\n");
    pthread_mutex_lock(&x_mutex);
    while(!(mask & M5))
    {
        printf("C block\n");
        pthread_cond_wait(&x_cond, &x_mutex);
    }
    mask |= M3;
    printf("Sente-se por favor.\n");
    pthread_cond_signal(&x_cond);
    pthread_mutex_unlock(&x_mutex);
    pthread_exit(NULL);
}

void* D(void* t)
{
    printf("D\n");
    pthread_mutex_lock(&x_mutex);
    while(!(mask & M5))
    {
        printf("D block\n");
        pthread_cond_wait(&x_cond, &x_mutex);
    }
    mask |= M4;
    printf("Aceita um copo d\'agua?\n");
    pthread_cond_signal(&x_cond);
    pthread_mutex_unlock(&x_mutex);
    pthread_exit(NULL);
}

void* E(void* t)
{
    printf("E\n");
    pthread_mutex_lock(&x_mutex);
    mask |= M5;
    printf("Seja bem-vindo!\n");
    pthread_cond_broadcast(&x_cond);
    pthread_mutex_unlock(&x_mutex);
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    int i;
    mask = 0;
    pthread_t threads[NTHREADS];

    pthread_mutex_init(&x_mutex, NULL);
    pthread_cond_init (&x_cond, NULL);

    pthread_create(&threads[0], NULL, A, NULL);
    pthread_create(&threads[1], NULL, B, NULL);
    pthread_create(&threads[2], NULL, C, NULL);
    pthread_create(&threads[3], NULL, D, NULL);
    pthread_create(&threads[4], NULL, E, NULL);
    
    
    // Espera todas as threads completarem
    for (i = 0; i < NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&x_mutex);
    pthread_cond_destroy(&x_cond);
}
