#include <math.h>
#include <stdio.h> 
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <float.h>
#include "timer.h"

#define i64 long long int
i64 arraySize, sharedi = 0;
pthread_mutex_t lock;

int *vectorSeqIn;
float *vectorSeqOut; //vetor sequencial
float *vectorConcOut; //vetor concorrente
int *vectorConcIn;
/*
typedef struct {
    float min, max;
}threadRet;
*/
typedef struct {
    i64 size;
    int id;
}threadArgs;

int ehPrimo(int n){
    int raiz = (int) sqrt(n);
    if(n <= 2){
        return 0;
    }
    for(int i = 2; i <= raiz; i++){
        if (!(n % i)){
            return 0;
        }
    }
    return 1;
}

void processaPrimos(int vetorEntrada[], float vetorSaida[], int dim) {
	for(int i = 0; i < dim; i++) {
		if (ehPrimo(vectorSeqIn[i])) {vectorSeqOut[i] = sqrt(vectorSeqIn[i]);}
        else {vectorSeqOut[i] = vectorSeqIn[i];}
	}
}

void *tarefa(void *arg)
{
    threadArgs *args = (threadArgs*) arg;
    i64 i;
    
    while(1)
    {
        pthread_mutex_lock(&lock);
        i = sharedi++;
        pthread_mutex_unlock(&lock);
        if(!(i < args->size)) {break;}
        //printf("id=%d, i=%lld, sharedi=%lld, size=%lld\n", args->id, i, sharedi, args->size);
        //fflush(stdin);
        if(ehPrimo(vectorConcIn[i])) {vectorConcOut[i] = sqrt(vectorConcIn[i]);}
        else {vectorConcOut[i] = vectorConcIn[i];}
    }
    pthread_exit(NULL);
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
    vectorSeqOut = (float*) malloc(sizeof(float)*arraySize);
    vectorConcOut = (float*) malloc(sizeof(float)*arraySize);
	vectorSeqIn = (int*) malloc(sizeof(int)*arraySize);
    vectorConcIn = (int*) malloc(sizeof(int)*arraySize);
	
    threadArgs *args = (threadArgs*) malloc(sizeof(threadArgs)*nthreads);
    //threadRet *ret = (threadRet*) malloc(sizeof(threadRet)*nthreads);
    
    if(vectorSeqOut == NULL) { fprintf(stderr, "ERRO--malloc(vectorSeqOut)\n");return 2; }
	if(vectorConcOut == NULL) { fprintf(stderr, "ERRO--malloc(vectorConcOut)\n");return 2; }
	if(vectorSeqIn == NULL) { fprintf(stderr, "ERRO--malloc(vectorSeqIn)\n");return 2; }
	if(vectorConcIn == NULL) { fprintf(stderr, "ERRO--malloc(vectorConcIn)\n");return 2; }
	
    if(args == NULL) { fprintf(stderr, "ERRO--malloc(args) {%lld*%ld} \n", arraySize, sizeof(threadArgs));return 2; }
    //if(ret == NULL) { fprintf(stderr, "ERRO--malloc(ret)\n");return 2; }
    
    
    // sequencial 
    for(i64 i = 0; i < arraySize; i++)
    {
		// limita a sequencia
        vectorSeqIn[i] = rand();
        vectorConcIn[i] = vectorSeqIn[i];
    }
	
    GET_TIME(inicio);
    
	processaPrimos(vectorSeqIn, vectorSeqOut, arraySize);
    
    GET_TIME(fim);
    delta = fim - inicio;
    //printf("Tempo sequencial (tamanho %lld) (nthreads %lld): %lf\n", arraySize, nthreads, delta);
    printf("Sequencial#%lld#%lld#%lf\n", arraySize, nthreads, delta);
	
    // concorrente
    pthread_mutex_init(&lock, NULL);
    pthread_t *tid = (pthread_t *) malloc(sizeof(pthread_t) * nthreads);
    if(tid == NULL) { fprintf(stderr, "ERRO--malloc(tid)\n");return 2; }
    GET_TIME(inicio);
#if 1
    for(i64 i = 0; i < nthreads; i++)
    {
        args[i].size = arraySize;
        args[i].id = i;
        
        if(pthread_create(&tid[i], NULL, tarefa, (void*) &args[i]))
        {
            fprintf(stderr, "ERRO--pthread_create()\n");
            return 3;
        }
    }
    
    for(i64 i = 0; i < nthreads; i++)
    {
        if(pthread_join(tid[i], NULL))
        {
            fprintf(stderr, "ERRO--pthread_create()\n");return 3;
        }
    }
    
#endif 
	pthread_mutex_destroy(&lock);
    GET_TIME(fim);
    delta = fim - inicio;
    //printf("Tempo concorrente (tamanho %lld) (nthreads %lld): %lf\n", arraySize, nthreads, delta);
    printf("Concorrente#%lld#%lld#%lf\n", arraySize, nthreads, delta);
    free(vectorConcIn);
	free(vectorConcOut);
    free(vectorSeqIn);
	free(vectorSeqOut);
    free(args);
    free(tid);
    //free(ret);
    
    return 0;
}