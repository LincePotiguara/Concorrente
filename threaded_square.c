#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

// Quantidade de elementos no array de teste
// Reduza este número para entender melhor o código
#define NUM_ENTRIES 10000

// Padrão elaborado a partir de costume
typedef struct {
    long long *data;
    int size;
} Array;

void* quadrado(void* arg)
{
    Array *array = (void *)arg;
    //printf("Tamanho: %d \n", array->size);
    for(int i = 0; i < array->size; i++)
    {
        array->data[i] = array->data[i] * array->data[i];
        //printf("%lld \n", array->data[i]);
    }
    pthread_exit(NULL);
    return 0;
}

int main(int argc, char **argv)
{
    pthread_t thread1, thread2;
    Array *arg1, *arg2;
    arg1 = malloc(sizeof(Array));
    arg2 = malloc(sizeof(Array));
    // Popula um array para o teste
    // Pode-se usar um dos tipos de stdint.h ao invés de long long
    long long *arg = (long long *)malloc(sizeof(long long)*NUM_ENTRIES);

    for(int i = 0; i < NUM_ENTRIES; i++)
    {
        arg[i] = (long long)(i+1);
    }

    // Primeira metade dos valores
    arg1->size = NUM_ENTRIES/2;
    arg1->data = arg;

    // O restante dos valores
    arg2->size = NUM_ENTRIES - arg1->size;
    arg2->data = arg + arg1->size;


    if(pthread_create(&thread1, NULL, quadrado, (void *)arg1))
    {
        puts("Error: thread_create()\n");
        exit(-1);
    }
    if(pthread_create(&thread2, NULL, quadrado, (void *)arg2))
    {
        puts("Error: thread_create()\n");
        exit(-1);
    }
    if(pthread_join(thread1, NULL))
    {
        puts("Error: thread_join()\n");
        exit(-1);
    }
    if(pthread_join(thread2, NULL))
    {
        puts("Error: thread_join()\n");
        exit(-1);
    }
    // Executa o código após o encerramento dos demais threads
    int all_true = 1;
    for(int i = 0; i < NUM_ENTRIES; i++)
    {
         all_true &= (arg[i] == (i + 1)*(i + 1));
    }
    if(all_true)
    {
        puts("Resultado: sucesso");
    }
    else
    {
        puts("Resultado: falha");
    }
    pthread_exit(NULL);
    return 0;
}
