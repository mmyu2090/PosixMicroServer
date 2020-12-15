/*
 * clientPost.c: A very, very primitive HTTP client for sensor
 * 
 * To run, prepare config-cp.txt and try: 
 *      ./clientPost
 *
 * Sends one HTTP request to the specified HTTP server.
 * Get the HTTP response.
 */

#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/time.h>
#include "stems.h"

char start[20];
char end[20];

void getdoubledt(char *dt)
{
  struct timeval val;
  struct tm *ptm;

  gettimeofday(&val, NULL);
  ptm = localtime(&val.tv_sec);

  memset(dt, 0x00, sizeof(dt));

  // format : YYMMDDhhmmssuuuuuu
  sprintf(dt, "%04d %02d %02d %02d %02d %02d %06ld", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, val.tv_usec);
}

void clientSend(int fd, char *filename, char *body)
{
  char buf[MAXLINE];
  char hostname[MAXLINE];

  Gethostname(hostname, MAXLINE);

  /* Form and send the HTTP request */
  sprintf(buf, "POST %s HTTP/1.1\n", filename);
  sprintf(buf, "%sHost: %s\n", buf, hostname);
  sprintf(buf, "%sContent-Type: text/plain; charset=utf-8\n", buf);
  sprintf(buf, "%sContent-Length: %d\n\r\n", buf, strlen(body));
  sprintf(buf, "%s%s\n", buf, body);
  Rio_writen(fd, buf, strlen(buf));
}

/*
 * Read the HTTP response and print it out
 */
void clientPrint(int fd)
{
  rio_t rio;
  char buf[MAXBUF];
  int length = 0;
  int n;

  Rio_readinitb(&rio, fd);

  /* Read and display the HTTP Header */
  n = Rio_readlineb(&rio, buf, MAXBUF);
  while (strcmp(buf, "\r\n") && (n > 0))
  {
    /* If you want to look for certain HTTP tags... */
    if (sscanf(buf, "Content-Length: %d ", &length) == 1)
    {
      //printf("Length = %d\n", length);
    }
    //printf("Header: %s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);
  }

  /* Read and display the HTTP Body */
  n = Rio_readlineb(&rio, buf, MAXBUF);
  printf("PID : %ld ", (long)getpid());
  while (n > 0)
  {
    printf("%s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);
  }
  printf("\n");
}

/* currently, there is no loop. I will add loop later */
void userTask(char *myname, char *hostname, int port, char *filename, long time, float value)
{
  int clientfd;
  char msg[MAXLINE];

  sprintf(msg, "name=%s&time=%ld&value=%f", myname, time, value);
  clientfd = Open_clientfd(hostname, port);
  clientSend(clientfd, filename, msg);
  clientPrint(clientfd);
  Close(clientfd);
}

void getargs_cp(char *myname, char *hostname, int *port, char *filename, float *time, float *value)
{
  FILE *fp;

  fp = fopen("config-cp.txt", "r");
  if (fp == NULL)
    unix_error("config-cp.txt file does not open.");

  fscanf(fp, "%s", myname);
  fscanf(fp, "%s", hostname);
  fscanf(fp, "%d", port);
  fscanf(fp, "%s", filename);
  fscanf(fp, "%f", time);
  fscanf(fp, "%f", value);
  fclose(fp);
}

void myShell(char *sensorName, char *hostname, int *port, char *filename, float *Time, float *value)
{
  char input[MAXLINE];
  char fun[MAXLINE];
  char data[MAXLINE];

  memset(&input[0], 0, sizeof(input));
  memset(&fun[0], 0, sizeof(fun));
  memset(&data[0], 0, sizeof(data));

  printf(">> ");
  scanf("%[^\n]%*c", input);
  sscanf(input, "%s %s", fun, data);
  if (strstr(fun, "help"))
  {
    //todo : shell 명령어 추가시 추가할것
    printf("help : list available commands\n");
    printf("name : print current sensor name\n");
    printf("name <sensor> : change sensor name to <sensor>\n");
    printf("value : print current value of sensor\n");
    printf("value <n> : set sensor value to <n>\n");
    printf("send : send (current sensor name, time, value) to server\n");
    printf("random <n> : send (name, time, random value) to server <n> times\n");
    printf("stress <n> : send <n> messages at same time");
    printf("quit : quit the program\n");
    return;
  }
  else if (strstr(fun, "quit"))
  {
    exit(0);
  }
  else if (strstr(fun, "name"))
  {
    if (data[0] == NULL)
    {
      printf("Current sensor is '%s'\n", sensorName);
      return;
    }
    else
    {
      strncpy(sensorName, data, sizeof(data));
      printf("Sensor name is changed to '%s'\n", sensorName);
      return;
    }
  }
  else if (strstr(fun, "value"))
  {
    if (data[0] == NULL)
    {
      printf("Current value of '%s' is '%f'\n", sensorName, *value);
      return;
    }
    else
    {
      *value = atof(data);
      printf("'%s' value is changed to '%f'\n", sensorName, *value);
      return;
    }
  }
  else if (strstr(fun, "send"))
  {
    userTask(sensorName, hostname, *port, filename, time(0), *value);
    printf("\n");
    return;
  }
  else if (strstr(fun, "random"))
  {
    if (data[0] != NULL)
    {
      int loop = atoi(data);
      float tValue = *value;
      srand(time(NULL));
      for (int i = 0; i < loop; i++)
      {
        sleep(1);
        if (rand() % 2 == 0)
        {
          *value = *value + (rand() % 11);
        }
        else
        {
          *value = *value - (rand() % 11);
        }
        userTask(sensorName, hostname, *port, filename, time(0), *value);
        *value = tValue;
        printf("\n");
      }
      return;
    }
    else
      return;
  }
  else if (strstr(fun, "stress"))
  {
    if (data[0] != NULL)
    {
      int iter = atoi(data);
      int *pid = (int *)malloc(sizeof(int) * iter);

      for (int i = 0; i < iter; i++)
      {
        pid[i] = fork();
        if (pid[i] == 0)
        {
          float tValue = *value;
          srand(time(NULL));
          if (rand() % 2 == 0)
          {
            *value = *value + (rand() % 11);
          }
          else
          {
            *value = *value - (rand() % 11);
          }
          getdoubledt(start);
          printf("PID : %ld, start : %s\n", (long)getpid(), start);
          userTask(sensorName, hostname, *port, filename, time(0), *value);
          getdoubledt(end);
          printf("PID : %ld, end : %s\n", (long)getpid(), end);
          getdoubledt(end);
          *value = tValue;
          exit(1);
        }
      }
    }
    else
    {
      return;
    }
  }
}
int main(void)
{
  char myname[MAXLINE], hostname[MAXLINE], filename[MAXLINE];

  int port;
  float time, value;

  getargs_cp(myname, hostname, &port, filename, &time, &value);

  printf("if you want to see commands, type 'help'\n");
  while (1)
  {
    myShell(myname, hostname, &port, filename, &time, &value);
  }

  return (0);
}
