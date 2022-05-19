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

/* Variaveis globais */
int x = 0;
pthread_mutex_t x_mutex;
pthread_cond_t x_cond;

// pthread_cond_signal(&x_cond);
void* A(void* t)
{
    pthread_mutex_lock(&x_mutex);
    pthread_cond_signal(&x_cond);
    printf("A\n");
    while(!(x ^ (M2|M3|M4|M5)))
    {
        printf("A block\n");
        pthread_cond_wait(&x_cond, &x_mutex);
    }
    x |= M1;
    printf("A-broadcast\n");
    pthread_cond_signal(&x_cond);
    pthread_mutex_unlock(&x_mutex);
    pthread_exit(NULL);
}

void* B(void* t)
{
    printf("B\n");
    pthread_mutex_lock(&x_mutex);
    while(!(x & M5))
    {
        printf("B block\n");
        pthread_cond_wait(&x_cond, &x_mutex);
    }
    x |= M2;
    printf("B-broadcast\n");
    pthread_cond_signal(&x_cond);
    pthread_mutex_unlock(&x_mutex);
    pthread_exit(NULL);
}

void* C(void* t)
{
    printf("C\n");
    pthread_mutex_lock(&x_mutex);
    while(!(x & M5))
    {
        printf("C block\n");
        pthread_cond_wait(&x_cond, &x_mutex);
    }
    x |= M3;
    printf("C-broadcast\n");
    pthread_cond_signal(&x_cond);
    pthread_mutex_unlock(&x_mutex);
    pthread_exit(NULL);
}

void* D(void* t)
{
    printf("D\n");
    pthread_mutex_lock(&x_mutex);
    while(!(x & M5))
    {
        printf("D block\n");
        pthread_cond_wait(&x_cond, &x_mutex);
    }
    x |= M4;
    printf("D-broadcast\n");
    pthread_cond_signal(&x_cond);
    pthread_mutex_unlock(&x_mutex);
    pthread_exit(NULL);
}

void* E(void* t)
{
    printf("E\n");
    pthread_mutex_lock(&x_mutex);
    x |= M5;
    pthread_cond_broadcast(&x_cond);
    printf("E-broadcast\n");
    pthread_mutex_unlock(&x_mutex);
    pthread_exit(NULL);
}

/* Funcao principal */
int main(int argc, char *argv[]) {
    int i;
    x = 0;
    pthread_t threads[NTHREADS];
    //aloca espaco para os identificadores das threads
    
    /* Inicilaiza o mutex (lock de exclusao mutua) e a variavel de condicao */
    pthread_mutex_init(&x_mutex, NULL);
    pthread_cond_init (&x_cond, NULL);
    
    /* Cria as threads */
    
    pthread_create(&threads[0], NULL, A, NULL);
    pthread_create(&threads[1], NULL, B, NULL);
    pthread_create(&threads[2], NULL, C, NULL);
    pthread_create(&threads[3], NULL, D, NULL);
    pthread_create(&threads[4], NULL, E, NULL);
    
    
    /* Espera todas as threads completarem */
    for (i = 0; i < NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf ("FIM.\n");
    
    /* Desaloca variaveis e termina */
    pthread_mutex_destroy(&x_mutex);
    pthread_cond_destroy(&x_cond);
}
