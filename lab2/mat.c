/* Multiplicacao de matriz-vetor (considerando matrizes quadradas) */
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include "timer.h"

float *mat2; //matriz de entrada
float *vet2; //vetor de entrada
float *saida2; //vetor de saida

int nthreads; //numero de threads

typedef struct {
    int id; //identificador do elemento que a thread ira processar
    int dim; //dimensao das estruturas de entrada
} tArgs;
/*

//funcao que as threads executarao
void * tarefa(void *arg) {
    tArgs *args = (tArgs*) arg;
    //printf("Thread %d\n", args->id);
    for(int i=args->id; i<args->dim; i+=nthreads)
        for(int j=0; j<args->dim; j++) 
        saida[i] += mat[i*(args->dim) + j] * vet[j];
    pthread_exit(NULL);
}

void *MultiplicarConcorrente(void *param) {
    tArgs *args = (tArgs*) param;
    //printf("Thread %d\n", args->id);
    for(int i = args->id; i < args->dim; i += nthreads)
        for(int j = 0; j < args->dim; j++) 
        saida[i] += mat[i*(args->dim) + j] * vet[j];
    pthread_exit(NULL);
}
*/

void *Multiplicar(int dim) {
    float acc = 0;
    //printf("Thread %d\n", args->id);
    for(int line = 0; line < dim; line++)
    {
        for(int column  = 0; column < dim; column++)
        {
            for(int k = 0; k < dim; k++)
            {
                //int a = mat2[line*(dim) + column];
                //int b = vet2[line*(dim) + column];
                acc += 
                    mat2[line*(dim) + k] *
                    vet2[(k)*(dim) + column]; 
            }
            saida2[line*(dim) + column] = acc;
            acc = 0;
        }
    }
}

void PrintSaida(int dim)
{
    printf("\n");
    for(int i = 0; i < dim; i++)
    {
        for(int j = 0; j < dim; j++)
        {
            printf("%.2f ", *(saida2 + i*dim + j));
        }
        printf("\n");
    }
}


//fluxo principal
int main(int argc, char* argv[]) {
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
    printf("cogito\n");
    //alocacao de memoria para as estruturas de dados
    mat2 = (float *) malloc(sizeof(float) * dim * dim);
    vet2 = (float *) malloc(sizeof(float) * dim * dim);
    saida2 = (float *) malloc(sizeof(float) * dim * dim);
    if (mat2 == NULL){printf("ERRO--malloc(matriz2)\n"); return 2;}
    if (vet2 == NULL){printf("ERRO--malloc(vetor2)\n"); return 2;}
    if (saida2 == NULL){printf("ERRO--malloc(saida2)\n"); return 2;}
    //inicializacao monothreaded
    printf("ergo\n");
    for(int i = 0; i < dim; i++)
    {
        for(int j = 0; j < dim; j++)
        {
            mat2[i*dim+j] = 2;    //equivalente mat[i][j]
            vet2[i*dim+j] = (i == j)? 2: 0;//identidade
        }
        saida2[i] = 0;
    }
    printf("inicializacao\n");
    
    GET_TIME(fim);
    delta = fim - inicio;
    //printf("Tempo inicializacao:%lf\n", delta);
    
    //multiplicacao da matriz pelo vetor
    GET_TIME(inicio);
    
    //faz a multiplicacao
    tid = (pthread_t*) malloc(sizeof(pthread_t)*nthreads);
    args = (tArgs*) malloc(sizeof(tArgs)*nthreads);
    if(tid == NULL) {puts("ERRO--malloc(thread_id)"); return 2;}
    if(args == NULL) {puts("ERRO--malloc(thread_args)"); return 2;}
    printf("multiplicacao\n");
    Multiplicar(dim);
    printf("encerramento\n");
    
    GET_TIME(fim)   
        delta = fim - inicio;
    printf("Tempo multiplicacao (dimensao %d) (nthreads %d): %lf\n", dim, nthreads, delta);
    PrintSaida(dim);
    
    //liberacao da memoria
    GET_TIME(inicio);
    free(mat2);
    free(vet2);
    free(saida2);
    free(args);
    free(tid);
    GET_TIME(fim)   
        delta = fim - inicio;
    //printf("Tempo finalizacao:%lf\n", delta);
    
    return 0;
}
