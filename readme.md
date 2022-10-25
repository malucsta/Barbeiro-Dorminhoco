# Solução do Problema

### Definindo os semáforos

#### Semáforos de lotação 
O problema nos diz, inicialmente, que a barbearia possui uma lotação máxima e ela varia conforme o período de forma que, se for festivo, então a lotação é de, no mínimo, 50 pessoas, enquanto em períodos que não o são, o máximo é de 20 pessoas. 

Sendo assim, isso nos indica a necessidade de um semáforo que controle a quantidade de pessoas que podem ou não entrar no estabelecimento. Definimos então: 

> sem_t barbearia;

Sendo este um semáforo de acesso global, ele é inicializado na main da seguinte forma: 

> sem_init(&barbearia, 0 , ocupacao);

Assim, definimos de forma dinâmica quantas pessoas (threads) podem ou não entrar no estabelecimento; 

De forma semelhante, definimos outros seguintes semáforos: 

>
> sem_t sofa;  
> sem_t cadeiras;
>

E os inicializamos: 
> sem_init(&sofa, 0 , 4);  
> sem_init(&cadeiras, 0 , 3);

Relembrando do conceito dos semáforos, quando inicializamos com valores pré-definidos, isso representará, no nosso caso, a lotação. Assim, qtoda vez que invocamos um **sem_wait** em qualquer um desses, o contador que definimos será decrementado. Se chegar em 0, isso significará que não há mais espaço e, portanto, outros clientes (isto é, outras threds) serão obrigados a esperarem pelo sinal de que vagou um lugar que corresponde ao semáforo.   

#### Semáforos de disponibilidade
Diante do escopo do nosso problema, necessitamos também da criação de dois outros semáforos: 
>sem_t clientes;  
>sem_t barbeiros;

Esses servirão para indicar a disponibilidade dos clientes e dos barbeiros e, por conta disso, são inicializados da seguinte forma: 
> sem_init(&barbeiros, 0 , 0);  
> sem_init(&clientes, 0, 0);  
> sem_init(&clientesNoCaixa, 0, 0); 

Como relembramos acima, quando um semáforo está com seu contador definido em 0, isso faz com que as threds que tentam acessá-lo não o consigam e, por conseguinte, esperam até que seja enviado um sinal que incremente o contador para que a thred possa acessá-lo. 

Tendo isso em mente, o semáforo dos clientes funciona da seguinte forma: 
- Sempre que um cliente ocupa a cadeira do barbeiro, ele faz um **sem_post(&clientes)**, incrementando esse semáforo e, por conseguinte, enviando o sinal para a thred barbeiro de que há um cliente a sua espera.

O semáforo de barbeiro, por sua vez, funciona assim: 
- Na thread do barbeiro, conferimos se há um cliente a sua espera por meio de um **sem_wait(&clientes)**. Se houver, com esse comando ele retira um cliente da "fila" que esse semáforo cria, e faz, logo em seguida, um **sem_post(&barbeiros)** para indicar que há um barbeiro disponível para o corte.
  
E, por fim, o semáforo de clientes no caixa: 
- Na thread do caixa, conferimos se há um cliente esperando na fila para poder pagar por meio de um **sem_wait(&clientesNoCaixa)**. Entretanto, como no escopo do problema nos é dito que os barbeiros se revezam na função de caixa, então também temos que fazer um **sem_wait(&barbeiros)** para saber se existe algum barbeiro disponível (isto é, um que não esteja cortando cabelo no momento). Se houver tanto um cliente a espera e um barbeiro disponível, com esses comandos de sem_wait, a thread caixa retira esse cliente da fila, o atende e, após isso, libera o barbeiro novamente.

Dessa forma, podemos perceber que esses três semáforos desempenharão um papel fundamental na sincronização entre os barbeiros, os clientes e o caixa.

#### Mutexes
Para o nosso problema também precisaremos de mutexes. Definimos um como um semáforo binário e outro como um mutex de fato apenas para utilizarmos todos os conceitos que envolvem esse assunto, embora, de fato, se tratem do mesmo princípio: 

> sem_t regiaoCritica  
> pthread_mutex_t freeClient

E eles são inicializados da seguinte forma: 

> sem_init (&regiaoCritica, 0, 1)  
> pthread_mutex_t freeClient = PTHREAD_MUTEX_INITIALIZER;

Pela definição do semáforos, quando começam com seus respectivos contadores em 1, esse torna-se o maior valor que podem assumir. Assim, quando decrementados por um sem_wait, eles fazem as outras threds esperarem pela sinal de liberação, garantindo a exclusão mútua do recurso. 

Este semáforo regiaoCritica será utilizado para isolar a seção crítica do código em que vamos invocar o sem_post dos clientes e o sem_post dos barbeiros, pois ele irá garantir que apenas um cliente, de fato, sente-se na cadeira do barbeiro e tenha seu cabelo cortado por um barbeiro, prezando assim, não só pela exclusão mútua, mas principalmente pela ordenação da fila de chegada. Assegurando, assim, que os clientes que esperam a mais tempo serão atendidos antes.

O mutex freeClient também desempenhará esse papel de isolar uma seção crítica, porém ele atua da seguinte forma: quando consegue acesso ao semáforo do caixa, o cliente emite a mensagem de que foi para o caixa e dá um lock nesse mutex. Isso será necessário para sincronizar as mensagens que o caixa emitirá sobre ter recebido seu pagamento. Assim, depois que o caixa executa todas as suas funcionalidades, ele indica que o pagamento foi realizado e libera esse mutex.  


# O programa:

## As variáveis globais

    pthread_mutex_t freeClient = PTHREAD_MUTEX_INITIALIZER;
    
    sem_t barbearia;
    sem_t clientes; 
    sem_t sofa;
    sem_t cadeiras; 
    sem_t barbeiros; 
    sem_t regiaoCritica;
    sem_t caixa;
    int ocupacao;
    int periodoFestivo = 0;
    int doLadoDeFora = 5;
    

# Main:

Definimos variáveis que utilizaremos e o número de clientes que queremos que tentem frequentar a barbearia. Colocamos as pessoas excedentes em uma outra variável para que se tornasse mais legível o que tentamos fazer aqui:

> long i;  
> int num_pessoas = ocupacao + doLadoDeFora;

Criamos arrays de threds para clientes e barbeiros: 
>  pthread_t threadsClientes [num_pessoas];  
> pthread_t threadsBarbeiros [3];

Inicializamos os semáforos que explicamos acima: 
>sem_init(&barbearia, 0 , ocupacao);  
>sem_init(&sofa, 0 , 4);  
>sem_init(&cadeiras, 0 , 3);
>
>sem_init(&barbeiros, 0 , 0);  
>sem_init(&clientes, 0, 0);  
>sem_init(&clientesNoCaixa, 0, 0);
>
>sem_init (&regiaoCritica, 0, 1);  

Criamos cada uma das threds dos arrays em loops:   

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

Como a thread de caixa é única, executamos apenas um comando: 

    pthread_create (&threadCaixa, NULL, caixaBody, (void *)i);
    
Damos um join nas threds para que a main espere a execução das threds dos clientes antes de finalizar sua execução: 

      for(i = 0; i < num_pessoas; i++) {
        pthread_join(threadsClientes[i], NULL); 
      }


# clienteBody
Essa é a função que define o comportamento das threds dos clientes: 

Requisitamos acesso à barbearia:  

    sem_wait (&barbearia);

Se o cliente consegue entrar na barbearia, ele procura um assento no sofá. Caso não consiga sentar-se no sofá, fica em pé, esperando:

    sem_wait (&sofa);

Se ele conseguiu um assento no sofá, ele procura assento na cadeira do barbeiro. Caso não consiga, permanece sentado, esperando 

    sem_wait (&cadeiras);

Se ele consegue acesso a uma cadeira de corte, então ele envia um sinal para informar que há um assento livre no sofá. Quem estava esperando em pé a mais tempo ocupará esse assento: 

    sem_post(&sofa);

Já tendo garantido uma cadeira de corte, o cliente precisa enviar um sinal para o barbeiro de que está esperando por ele. Para isso, ele adentra em uma seção crítica, pois existem 3 cadeiras que podem ser ocupadas e o barbeiro só corta o cabelo de uma pessoa por vez. 

    sem_wait(&regiaoCritica); 
    sem_post(&clientes); 
    sem_post(&regiaoCritica); 

Depois de enviar esse sinal, ele espera para que o barbeiro esteja disponível para atendê-lo: 
 
     sem_wait(&barbeiros); 
     
Então, depois de conseguir acesso ao barbeiro, ele corta o cabelo. Após isso, deve liberar a cadeira para que outro cliente a ocupe: 

    sem_post(&cadeiras);

Como não pode deixar o recinto sem pagar, então ele libera o barbeiro e entra na fila do caixa, na qual apenas um cliente pode acessar por vez. Como o problema diz que qualquer barbeiro livre pode executar a função do caixa, então liberamos o barbeiro antes de entrar na fila:
 
    sem_post(&barbeiros); 
    
    sem_post(&clientesNoCaixa); 
    printf ("\t\t\t\tP%02ld: foi para o caixa\n", tid);
    pthread_mutex_lock(&freeClient); 


Então o cliente pode, agora, sair da barbearia

    sem_post (&barbearia);


# barbeiroBody
Essa é a função que define o comportamento das threds dos barbeiros: 

Primeiramente, é importante notar que barbeiros trabalham em loop. Assim, enquanto houverem clientes, ele continuará cortando seus cabelos. Por isso, colocamos todo esse bloco dentro de um **while(1)**

Dito isso, o barbeiro sempre começa esperando o sinal da chegada de um cliente. Lembrando que esse sinal o cliente envia quando consegue acesso à cadeira de corte. Quando em zero, isto é, quando não tem clientes, o barbeiro espera dormindo:

    sem_wait(&clientes); 

Quando há um cliente, finalmente, ele precisa enviar o sinal de que está disponível para o corte, o que, por conseguinte, irá liberar a thred do cliente que o espera. Essa é uma seção crítica, e, por isso, o acesso à ela deve garantir a exclusão mútua: 

    sem_wait(&regiaoCritica);  
    sem_post(&barbeiros); 
    sem_post(&regiaoCritica);

Assim, garante-se a sincronização, pois o barbeiro espera ter clientes e o cliente, ao chegar na cadeira, sinaliza para o barbeiro de que o espera. 


# caixaBody

Essa é a função que define o comportamento da thread do caixa: 

Primeiramente, é importante notar o caixa também trabalha em loop, pois o escopo do problema nos diz que ele continuamente recebe os pagamentos. Por isso, colocamos todo esse bloco dentro de um **while(1)**

Muito semelhante a funcionamento do barbeiro, o caixa espera pelo sinal de que há clientes no caixa - e eles o fazem ao darem um **sem_post(&clientesNoCaixa)**. Entretanto, não há como realizar o pagamento sem que o barbeiro esteja disponível para recebê-lo. Assim, também precisamos esperar pelo sinal de disponibilidade do barbeiro: 

    sem_wait(&clientesNoCaixa); 
    sem_wait(&barbeiros); 

Depois que conseguiu ambos os recursos, o caixa pode coletar o pagamento e ele o faz imprimindo a seguinte mensagem: 

    printf("Imprimindo recibo...\n");

Como citado anteriormente, para garantir que essa mensagem esteja sincronizada e seja impressa na ordem correta, utilizamos-nos do mutex freeClient. Se ele não existisse, o que aconteceria aqui é o seguinte: o cliente solicitaria o pagamento e, enquanto o caixa estivesse processando, o cliente teria ido embora. Sendo assim, depois de imprimir a mensagem, o caixa deve liberar esse mutex, indicando que já realizou suas tarefas e que, somente agora, o cliente está livre para ir embora. 

    pthread_mutex_unlock(&freeClient);

Para finalizar, de fato, suas atividades, o caixa precisa, também, liberar o barbeiro e ele o faz da seguinte forma:
    
    sem_wait(&regiaoCritica);  
    sem_post(&barbeiros); 
    sem_post(&regiaoCritica);
