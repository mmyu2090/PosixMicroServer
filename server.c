#include "stems.h"
#include "request.h"
#include "mysql/mysql.h"

//
// To run:
// 1. Edit config-ws.txt with following contents
//    <port number>
// 2. Run by typing executable file
//    ./server
// Most of the work is done within routines written in request.c
//

typedef struct conData
{
  int connfd;
  double arrivalTime;
} conData;

int queueIter = -1;
conData *queue;
sem_t empty, full, mutex;

void getargs_ws(int *port, int *poolSize, int *queueSize)
{
  FILE *fp;
  //config-ws.txt 의 정보를 읽어 프로그램이 실행될 포트를 초기화
  if ((fp = fopen("config-ws.txt", "r")) == NULL)
    unix_error("config-ws.txt file does not open.");

  fscanf(fp, "%d", port);
  fscanf(fp, "%d", poolSize);
  fscanf(fp, "%d", queueSize);
  fclose(fp);
}

void consumer()
{
  while (1)
  {
    int connfd;
    double arrivalTime;

    sem_wait(&full);
    sem_wait(&mutex);
    connfd = queue[queueIter].connfd;
    arrivalTime = queue[queueIter].arrivalTime;
    queueIter--;
    sem_post(&mutex);
    sem_post(&empty);

    requestHandle(connfd, arrivalTime);
    Close(connfd);
  }
}

int main(void)
{
  int pid;
  int pfd[2];
  char *argv[] = {NULL};

  if (pipe(pfd) < 0)
  {
    fprintf(stderr, "pipe failed: %s\n", strerror(errno));
    exit(1);
  }

  pid = fork();
  if (pid == 0)
  {
    execve("alarmClient", argv, environ);
  }
  else if (pid > 0)
  {
    int listenfd, connfd, port, clientlen, poolSize, queueSize;

    getargs_ws(&port, &poolSize, &queueSize); //server.c, 서버 포트 초기화

    pthread_t *conThreads;
    struct sockaddr_in clientaddr;

    conThreads = (pthread_t *)malloc(sizeof(pthread_t) * poolSize);
    queue = (conData *)malloc(sizeof(conData) * queueSize);

    printf("port : %d, poolSize : %d, queueSize : %d\n", port, poolSize, queueSize);

    sem_init(&empty, 0, queueSize);
    sem_init(&full, 0, 0);
    sem_init(&mutex, 0, 1);

    initWatch(); //request.c, 실행시작시간 초기화

    listenfd = Open_listenfd(port); //stems.c, 포트를 사용하도록 초기화

    for (int i = 0; i < poolSize; i++)
    {
      printf("create thread\n");
      if (pthread_create(&conThreads[i], NULL, (void *)consumer, NULL))
      {
        fprintf(stderr, "ERR : ");
        exit(1);
      }
    }

    while (1)
    {
      clientlen = sizeof(clientaddr);
      connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen); //stems.c, socket.h : accept 소켓의 연결을 허용

      sem_wait(&empty);
      sem_wait(&mutex);
      queueIter++;
      queue[queueIter].connfd = connfd;
      queue[queueIter].arrivalTime = getWatch();
      sem_post(&mutex);
      sem_post(&full);
    }
    return (0);
  }
}
