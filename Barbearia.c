#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

// COMPILAR:
//  gcc -Wall Barbearia.c -o Barbearia -lpthread
// ./Barbearia

pthread_mutex_t freeClient = PTHREAD_MUTEX_INITIALIZER;

sem_t barbearia;
sem_t clientes; 
sem_t sofa;
sem_t cadeiras; 
sem_t barbeiros; 
sem_t regiaoCritica;
sem_t clientesNoCaixa;
int ocupacao;
int periodoFestivo = 0;
int doLadoDeFora = 5;


void espera (int n)
{
  // pausa entre 0 e n segundos (inteiro)
  sleep (random() % n) ;	

  // pausa entre 0 e 1 segundo (float)
  usleep (random() % 1000000) ;	
}

void *clienteBody (void *id) {
  
  long tid = (long) id;
  
  sem_wait (&barbearia);
  printf ("P%02ld: entrou na barbearia\n",tid);  

  //Espera um assento no sofá vagar
  sem_wait (&sofa);

  //Espera uma cadeira vagar
  sem_wait (&cadeiras);

  //Libera o assento do sofá
  sem_post(&sofa);

  //Adentra na região crítica, mandando o sinal de que há um cliente a espera
  sem_wait(&regiaoCritica); 
  sem_post(&clientes); 
  sem_post(&regiaoCritica); 
  
  //Espera um barbeiro acordar
  sem_wait(&barbeiros); 
  printf ("\t\tP%02ld: Sentou na cadeira de corte\n", tid);
  espera(2); //cortando o cabelo...

  //Libera a cadeira depois que termina o corte 
  sem_post(&cadeiras);
  
  //Libera o barbeiro
  sem_post(&barbeiros);
  
  //Pagamento  
  sem_post(&clientesNoCaixa); 
  printf ("\t\t\t\tP%02ld: foi para o caixa\n", tid);
  pthread_mutex_lock(&freeClient);
  

  //Sai da barbearia
  sem_post (&barbearia);
}

void *barbeiroBody (void *id) {
  long tid = (long) id;
  while(1){
    //Espera o sinal de que há clientes a espera 
    sem_wait(&clientes); 
  
    //Adentra na região crítica, informando que há um barbeiro pronto
    sem_wait(&regiaoCritica);  
    sem_post(&barbeiros); 
    sem_post(&regiaoCritica);
    espera(1); 
    
    //printf ("\t\t\t\t\t\t\tB%02ld: cortou o cabelo\n", tid); 
  }
}

void *caixaBody (void *id) {

  while(1) {
    sem_wait(&clientesNoCaixa); 
    sem_wait(&barbeiros); 
    printf("Imprimindo recibo...\n");
    pthread_mutex_unlock(&freeClient);
    sem_wait(&regiaoCritica);  
    sem_post(&barbeiros); 
    sem_post(&regiaoCritica);
  }

  
  //Continuamente recebe os pagamentos.
  //Toda vez que receber um pagamento, deixa um sinal aberto que quaisquer dos barbeiros pode atender.
  //Tal sinal é para que o barbeiro emita o recibo, entregue ao cliente para que o mesmo seja liberado.
}

int main(void) {

  if (periodoFestivo) {
    ocupacao = 50;
  }
  else {
    ocupacao = 20;
  }
  
  long i; 
  int num_pessoas = ocupacao + doLadoDeFora;
  
  pthread_t threadsClientes [num_pessoas];
  pthread_t threadsBarbeiros [3];
  pthread_t threadCaixa;

  //Semáforos que indicam ocupação máxima
  sem_init(&barbearia, 0 , ocupacao);
  sem_init(&sofa, 0 , 4);
  sem_init(&cadeiras, 0 , 3);

  //Iniciamos com 0 barbeiros disponíveis e 0 clientes a espera
  sem_init(&barbeiros, 0 , 0);
  sem_init(&clientes, 0, 0); 
  sem_init(&clientesNoCaixa, 0, 0); 

  //mutexes para acesso à seções críticas
  sem_init (&regiaoCritica, 0, 1);

  
  //Criando as threads de clientes
  for (i=0; i<num_pessoas; i++)
    if (pthread_create (&threadsClientes[i], NULL, clienteBody, (void *)i))
    {
      perror ("pthread_create") ;
      exit (1) ;
    }

  //Criando as threads de barbeiros
  for (i=0; i<3; i++)
    if (pthread_create (&threadsBarbeiros[i], NULL, barbeiroBody, (void *)i))
    {
      perror ("pthread_create") ;
      exit (1) ;
    }

  //Criando a thread do caixa
  pthread_create (&threadCaixa, NULL, caixaBody, (void *)i);

  
  for(i = 0; i < num_pessoas; i++) {
    pthread_join(threadsClientes[i], NULL); 
  }

  return 0; 

   // main encerra aqui
  //pthread_exit (NULL) ;
}