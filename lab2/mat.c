/* Multiplicacao de matriz-vetor (considerando matrizes quadradas) */
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include "timer.h"

float *mat; //matriz de entrada
float *vet; //vetor de entrada
float *saida; //vetor de saida

float *mat2; //matriz de entrada
float *vet2; //vetor de entrada
float *saida2; //vetor de saida

int nthreads; //numero de threads

typedef struct {
    int start; // linha onde comeca a ser calculado
    int end;   // final exclusive
    int dim;   //dimensao das estruturas de entrada
} tArgs;



void *MultiplicarConcorrente(void *param) {
    tArgs *args = (tArgs*) param;
    int acc = 0;
    for(int line = args->start; line < args->end; line++)
    {
        for(int column  = 0; column < args->dim; column++)
        {
            for(int k = 0; k < args->dim; k++)
            {
                acc += 
                    mat[line*(args->dim) + k] *
                    vet[(k)*(args->dim) + column]; 
            }
            saida[line*(args->dim) + column] = acc;
            acc = 0;
        }
    }
    pthread_exit(NULL);
    // a funcao nao precisa desse retorno
    // isso apenas satisfaz a assinatura
    return 0;
}


void *Multiplicar(int dim) {
    float acc = 0;
    for(int line = 0; line < dim; line++)
    {
        for(int column  = 0; column < dim; column++)
        {
            for(int k = 0; k < dim; k++)
            {
                acc += 
                    mat2[line*(dim) + k] *
                    vet2[(k)*(dim) + column]; 
            }
            saida2[line*(dim) + column] = acc;
            acc = 0;
        }
    }
}
int verifica(int dim)
{
    int equals = 1;
    for (int i = 0; i < dim; i++)
    {
        for (int j = 0; j < dim; j++)
        {
            if(saida[i*dim + j] != saida2[i*dim + j])
            {
                equals = 0;
                return equals;
            }
        }
    }
    return equals;
}

void PrintSaida(int dim)
{
    printf("Mono\n");
    for(int i = 0; i < dim; i++)
    {
        for(int j = 0; j < dim; j++)
        {
            printf("%.2f ", *(saida2 + i*dim + j));
        }
        printf("\n");
    }
    printf("Poli\n");
    for(int i = 0; i < dim; i++)
    {
        for(int j = 0; j < dim; j++)
        {
            printf("%.2f ", *(saida + i*dim + j));
        }
        printf("\n");
    }
}

int main(int argc, char* argv[])
{
    int dim; //dimensao da matriz de entrada
    pthread_t *tid; //identificadores das threads no sistema
    tArgs *args; //identificadores locais das threads e dimensao
    double inicio, fim, delta;
    
    GET_TIME(inicio);
    //leitura e avaliacao dos parametros de entrada
    if(argc < 3) {
        printf("Digite: %s <dimensao da matriz> <numero de threads>\n", argv[0]);
        return 1;
    }
    dim = atoi(argv[1]);
    nthreads = atoi(argv[2]);
    if (nthreads > dim) {nthreads=dim;}
    //alocacao de memoria para as estruturas de dados
    // se houver um erro, havera vazamento de memoria
    mat = (float *) malloc(sizeof(float) * dim * dim);
    vet = (float *) malloc(sizeof(float) * dim * dim);
    saida = (float *) malloc(sizeof(float) * dim * dim);
    
    mat2 = (float *) malloc(sizeof(float) * dim * dim);
    vet2 = (float *) malloc(sizeof(float) * dim * dim);
    saida2 = (float *) malloc(sizeof(float) * dim * dim);
    if (mat2 == NULL){printf("ERRO--malloc(matriz2)\n"); return 2;}
    if (vet2 == NULL){printf("ERRO--malloc(vetor2)\n"); return 2;}
    if (saida2 == NULL){printf("ERRO--malloc(saida2)\n"); return 2;}
    
    if (mat == NULL){printf("ERRO--malloc(matriz)\n"); return 2;}
    if (vet == NULL){printf("ERRO--malloc(vetor)\n"); return 2;}
    if (saida == NULL){printf("ERRO--malloc(saida)\n"); return 2;}
    
    // inicializacao aleatoria
    srand(time(NULL));
    for (int i = 0; i < dim; i++)
    {
        for (int j = 0; j < dim; j++)
        {
            mat[i * dim + j] = rand() % 10;
            vet[i * dim + j] = rand() % 10;
            mat2[i * dim + j] = mat[i * dim + j];
            vet2[i * dim + j] = vet[i * dim + j];
        }
    }
    GET_TIME(fim);
    delta = fim - inicio;
    //printf("Tempo inicializacao:%lf\n", delta);
    
    //multiplicacao da sequencial
    GET_TIME(inicio);
    Multiplicar(dim);
    GET_TIME(fim)   
        delta = fim - inicio;
    printf("Tempo multiplicacao sequencial (dimensao %d) (nthreads %d): %lf\n", dim, nthreads, delta);
    
    //bloco concorrente
    int taskLoad = dim / nthreads;
    int taskRemainder = dim % nthreads;
    
    tid = (pthread_t*) malloc(sizeof(pthread_t)*nthreads);
    args = (tArgs*) malloc(sizeof(tArgs)*nthreads);
    if(tid == NULL) {puts("ERRO--malloc(thread_id)"); return 2;}
    if(args == NULL) {puts("ERRO--malloc(thread_args)"); return 2;}
    
    GET_TIME(inicio);
    for(int i = 0,  start = 0,end = 0, load = 0;
        i < nthreads;
        i++)
    {
        if(i < taskRemainder)
        {
            load = taskLoad + 1;
        }
        else
        {
            load = taskLoad;
        }
        start = end;
        end = end + load;
        (args + i)->start = start;
        (args + i)->end = end;
        (args + i)->dim = dim;
        if (pthread_create(tid + i, NULL, MultiplicarConcorrente, (void *)(args + i)))
        {
            puts("ERRO--pthread_create()");
            return 3;
        }
    }
    // sincronizacao
    for (int i = 0; i < nthreads; i++)
    {
        pthread_join(*(tid + i), NULL);
    }
    GET_TIME(fim);
    delta = fim - inicio;
    
    printf("Tempo multiplicacao concorrente (dimensao %d) (nthreads %d): %lf\n", dim, nthreads, delta);
    //PrintSaida(dim);
    
    // verifica se os resultados da multiplicacao sao iguais
    if(verifica(dim))
    {
        printf("as matrizes sao iguais\n");
    }
    else
    {
        printf("matrizes diferentes");
    }
    //liberacao da memoria
    GET_TIME(inicio);
    free(mat);
    free(vet);
    free(saida);
    free(mat2);
    free(vet2);
    free(saida2);
    free(args);
    free(tid);
    GET_TIME(fim);
    delta = fim - inicio;
    //printf("Tempo finalizacao:%lf\n", delta);
    
    return 0;
}
