#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>

#define NLEITORES 4
#define NESCRITORES 1
#define TIME 300

sem_t em_e, em_l, escr, leit; //semaforos
int e = 0, l = 0; //globais


int nescritores, nleitores;

void* leitores(void* leitor_id)
{
    long long int id = (long long int) leitor_id;
    while(1)
    {
        printf("Leitor\t\t(%lld) quer ler\n", id);
        sem_wait(&leit);
        sem_wait(&em_l);
        l++;
        if(l==1) {sem_wait(&escr);}
        sem_post(&em_l);
        sem_post(&leit);
        /* usleep(TIME*1000); */
        printf("Leitor\t\t(%lld) esta lendo\n", id);
        //le...
        sem_wait(&em_l);
        l--;
        if(l == 0)
        {
            sem_post(&escr);
        }
        sem_post(&em_l);
    }
}

void* escritores(void* escritor_id)
{
    long long int id = (long long int) escritor_id;
    while(1)
    {
        printf("Escritor\t(%lld) quer escrever\n", id);
        sem_wait(&em_e);
        e++;
        if(e==1) {sem_wait(&leit);}
        sem_post(&em_e);
        sem_wait(&escr);
        usleep(TIME*1000);
        printf("Escritor\t(%lld) esta escrevendo\n", id); //escreve
        sem_post(&escr);
        sem_wait(&em_e);
        e--;
        if(e == 0)
        {
            sem_post(&leit);
        }
        sem_post(&em_e);
        // Escritor tem vantagem, por isso de uma chance para os leitores
        usleep(TIME*1000);
    }
}

int main(int argc, char **argv)
{
    nleitores = NLEITORES;
    nescritores = NESCRITORES;
    if(argc == 3)
    {
       nleitores = atoi(argv[1]);
       nescritores = atoi(argv[2]);
    }
    else
    {
        puts("Use como parametro: <numero de leitores> <numero de de escritores>");
    }
    sem_init(&em_e, 0, 1);
    sem_init(&em_l, 0, 1);
    sem_init(&leit, 0, 1);
    sem_init(&escr, 0, 1);

    pthread_t *tid_escritor, *tid_leitor;
    tid_escritor = malloc(sizeof(pthread_t) * nescritores);
    tid_leitor = malloc(sizeof(pthread_t) * nleitores);

    for(long long int i = 0; i < nleitores; i++)
    {
        if(pthread_create(tid_leitor, NULL, leitores, (void*) i+1))
        {
            printf("ERRO--pthread _create\n");
            return 3;
        }
    }

    for(long long int i = 0; i < nescritores; i++)
    {
        if(pthread_create(tid_escritor, NULL, escritores, (void*) i+1))
        {
            printf("ERRO--pthread _create\n");
            return 3;
        }
    }

    pthread_exit(NULL);

}
