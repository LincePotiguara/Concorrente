#include <stdio.h> 
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <float.h>
#include "timer.h"

#define min(X_, Y_) ((X_) < (Y_))? (X_):(Y_)
#define max(X_, Y_) ((X_) > (Y_))? (X_):(Y_)

#define i64 long long int
i64 arraySize;
float *vectorSeq; //vetor sequencial
float *vectorConc; //vetor concorrente

// TODO excluir isso
typedef struct {
    float min, max;
} float2;

typedef struct {
    float *data;
    i64 size;
} array;

typedef struct {
    float min, max;
}threadRet;

typedef struct {
    //float min, max;
    float *data;
    i64 size;
    threadRet *ret;
}threadArgs;



void *tarefa(void *arg)
{
    //printf("tarefa\n");
    float min = FLT_MAX, max = FLT_MIN;
    threadArgs *args = (threadArgs*) arg;
    //printf("&data: %p \n", &args->data);
    //printf("&min: %p \n", &args->min);
    //printf("&max: %p \n", &args->max);
    //printf("&size: %p \n", &args->size);
    for(i64 i = 0; i < args->size; i++)
    {
        min = min(min, args->data[i]);
        max = max(max, args->data[i]);
        //printf("T: %p \n", &args->data[i]);
    }
    args->ret->min = min;
    args->ret->max = max;
    //fprintf(stdout, "\n***********************\nset %f %f\n", min, max);
    pthread_exit(arg);
    
    /*
       double *somaLocal; //variavel local da soma de elementos
       somaLocal = (double*) malloc(sizeof(double));
       if(somaLocal==NULL) {exit(1);}
       long int tamBloco = arraySize/nthreads;  //tamanho do bloco de cada thread 
       long int ini = id * tamBloco; //elemento inicial do bloco da thread
       long int fim; //elemento final(nao processado) do bloco da thread
       if(id == nthreads-1) fim = arraySize;
       else fim = ini + tamBloco; //trata o resto se houver
       //soma os elementos do bloco da thread
       for(long int i=ini; i<fim; i++)
          *somaLocal += vetor[i];
       //retorna o resultado da soma local
       pthread_exit((void *) somaLocal); 
       */
}

int main(int argc, char **argv)
{
    srand(time(0));
    double inicio = 0, fim = 0, delta = 0;
    if(argc < 3)
    {
        puts("Uso <tamanho do array> <numero de threads>");
        return 0;
    }
    char *argvPrev = argv[1], *end;
    errno = 0;
    i64 arraySize = strtoll(argv[1], &end, 10);
    if(argv[1] == end) { printf("there was an error\n"); }
    if(errno) {printf("Err: %s\n", strerror(errno)); }
    argv[1] = argvPrev;
    errno = 0;
    i64 nthreads = strtoll(argv[2], &end, 10);
    if(argv[2] == end) { printf("there was an error\n"); }
    if(errno) { printf("Err: %s\n", strerror(errno)); }
    
    if(nthreads > arraySize){nthreads = arraySize;}
    // alocação
    vectorSeq = (float*) malloc(sizeof(float)*arraySize);
    vectorConc = (float*) malloc(sizeof(float)*arraySize);
    threadArgs *args = (threadArgs*) malloc(sizeof(threadArgs)*arraySize);
    threadRet *ret = (threadRet*) malloc(sizeof(threadRet)*nthreads);

    if(vectorSeq == NULL) { fprintf(stderr, "ERRO--malloc(vectorSeq)\n");return 2; }
    if(vectorConc == NULL) { fprintf(stderr, "ERRO--malloc(vectorConc)\n");return 2; }
    if(args == NULL) { fprintf(stderr, "ERRO--malloc(args) {%lld*%ld} \n", arraySize, sizeof(threadArgs));return 2; }
    if(ret == NULL) { fprintf(stderr, "ERRO--malloc(ret)\n");return 2; }
    
    
    // sequencial 
    for(i64 i = 0; i < arraySize; i++)
    {
        vectorSeq[i] = rand();
        vectorConc[i] = vectorSeq[i];
    }
    for(i64 i = 0; i < arraySize; i++)
    {
        vectorConc[i] = vectorSeq[i];
    }
    GET_TIME(inicio);
    float minSeq = FLT_MAX, maxSeq = FLT_MIN;
    for(i64 i = 0; i < arraySize; i++)
    {
        minSeq = min(minSeq, vectorSeq[i]);
        maxSeq = max(maxSeq, vectorSeq[i]);
    }
    GET_TIME(fim);
    delta = fim - inicio;
    printf("Tempo de busca sequencial (tamanho %lld) (nthreads %lld): %lfs\n", arraySize, nthreads, delta);
    // concorrente
#if 1

    pthread_t *tid = (pthread_t *) malloc(sizeof(pthread_t) * nthreads);
    if(tid == NULL) { fprintf(stderr, "ERRO--malloc(tid)\n");return 2; }
    
    for(i64 i = 0; i < nthreads; i++)
    {
        args[i].ret = &ret[i];
    }
    
    
    i64 blockSize = arraySize / nthreads;
    i64 blockRemainder = arraySize % nthreads;
    i64 load;
    GET_TIME(inicio);
    for(i64 i = 0, p = 0; i < nthreads; i++)
    {
        load = (i < blockRemainder)? blockSize + 1 : blockSize;
        args[i].size = load;
        args[i].data = &vectorConc[p];
        p += load;
        if(pthread_create(&tid[i], NULL, tarefa, (void*) &args[i]))
        {
            fprintf(stderr, "ERRO--pthread_create()\n");
            return 3;
        }
    }
    float min = FLT_MAX, max = FLT_MIN;
    for(i64 i = 0; i < nthreads; i++)
    {
        if(pthread_join(tid[i], (void*) &args[i]))
        {
            fprintf(stderr, "ERRO--pthread_create()\n");return 3;
        }
    }
    for(i64 i = 0; i < nthreads; i++)
    {
        min = min(min, args[i].ret->min);
        max = max(max, args[i].ret->max);
    }
    GET_TIME(fim);
    delta = fim - inicio;
    printf("Tempo de busca concorrente (tamanho %lld) (nthreads %lld): %lfs\n", arraySize, nthreads, delta);
#endif
    free(vectorConc);
    free(vectorSeq);
    free(args);
    free(tid);
    free(ret);
    /*
    printf("nothing to complain about\n");
    printf("%lld, %lld\n", arraySize, nthreads);
    printf("%s, %s\n", argv[1], argv[2]);
    */
    //printf("nothing to complain about\n");
    printf("min: %.6f, max: %.6f - sequencial\n", minSeq, maxSeq);
    
    printf("min: %.6f, max: %.6f - concorrente\n", min, max);
    //printf("%s, %s\n", argv[1], argv[2]);
    return 0;
}