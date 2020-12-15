/*
 * clientGet.c: A very, very primitive HTTP client for console.
 * 
 * To run, prepare config-cg.txt and try: 
 *      ./clientGet
 *
 * Sends one HTTP request to the specified HTTP server.
 * Prints out the HTTP response.
 *
 * For testing your server, you will want to modify this client.  
 *
 * When we test your server, we will be using modifications to this client.
 *
 */

#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>
#include "stems.h"

/*
 * Send an HTTP request for the specified file 
 */
void clientSend(int fd, char *filename)
{
  char buf[MAXLINE];
  char hostname[MAXLINE];

  Gethostname(hostname, MAXLINE);

  /* Form and send the HTTP request */
  sprintf(buf, "GET %s HTTP/1.1\n", filename);
  sprintf(buf, "%shost: %s\n\r\n", buf, hostname);
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
    //printf("Header: %s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);

    /* If you want to look for certain HTTP tags... */
    if (sscanf(buf, "Content-Length: %d ", &length) == 1)
    {
      //printf("Length = %d\n", length);
    }
  }

  /* Read and display the HTTP Body */
  n = Rio_readlineb(&rio, buf, MAXBUF);
  while (n > 0)
  {
    printf("%s", buf);
    n = Rio_readlineb(&rio, buf, MAXBUF);
  }
}

/* currently, there is no loop. I will add loop later */
void userTask(char hostname[], int port, char webaddr[])
{
  int clientfd;

  clientfd = Open_clientfd(hostname, port);
  clientSend(clientfd, webaddr);
  clientPrint(clientfd);
  Close(clientfd);
}

void getargs_cg(char hostname[], int *port, char webaddr[])
{
  FILE *fp;

  fp = fopen("config-cg.txt", "r");
  if (fp == NULL)
    unix_error("config-cg.txt file does not open.");

  fscanf(fp, "%s", hostname);
  fscanf(fp, "%d", port);
  fscanf(fp, "%s", webaddr);
  fclose(fp);
}

void myShell(char *hostname, int *port, char *webaddr)
{
  char webaddr2[MAXLINE];
  char input[MAXLINE];
  char fun[MAXLINE];
  char data[MAXLINE];
  char data2[MAXLINE];

  memset(&webaddr2[0], 0, sizeof(webaddr2));
  memset(&input[0], 0, sizeof(input));
  memset(&fun[0], 0, sizeof(fun));
  memset(&data[0], 0, sizeof(data));
  memset(&data2[0], 0, sizeof(data2));

  strncpy(webaddr2, webaddr, sizeof(data));

  printf(">> ");
  scanf("%[^\n]%*c", input);
  sscanf(input, "%s %s %s", fun, data, data2);
  if (strstr(fun, "help"))
  {
    //todo : shell 명령어 추가시 추가할것
    printf("LIST : print list contain sensors\n");
    printf("INFO <sensor> : print value of <sensor>\n");
    printf("GET <sensor> : print one recent value of <sensor>\n");
    printf("GET <sensor> <n> : print <n> value of <sensor> \n");
    printf("QUIT : quit the program\n");
    printf("EXIT : quit the program\n");
    return;
  }
  else if (strstr(fun, "QUIT") || strstr(fun, "EXIT"))
  {
    exit(0);
  }
  else if (strstr(fun, "LIST"))
  {
    sprintf(webaddr2, "%s%s", webaddr2, "command=LIST");
    userTask(hostname, *port, webaddr2);
    return;
  }
  else if (strstr(fun, "INFO"))
  {
    if (data[0] == NULL)
    {
      return;
    }
    else
    {
      sprintf(webaddr2, "%s%s%s", webaddr2, "command=INFO&value=", data);
      userTask(hostname, *port, webaddr2);
      return;
    }
  }
  else if (strstr(fun, "GET"))
  {
    if (data[0] == NULL)
    {
      return;
    }
    else
    {
      if (data2[0] == NULL)
      {
        sprintf(webaddr2, "%s%s%s%s", webaddr2, "NAME=", data, "&N=1");
        userTask(hostname, *port, webaddr2);
        return;
      }
      else
      {
        int n = atoi(data2);
        sprintf(webaddr2, "%s%s%s%s%d", webaddr2, "NAME=", data, "&N=", n);
        userTask(hostname, *port, webaddr2);
        return;
      }
    }
  }
}

int main(void)
{
  char hostname[MAXLINE], webaddr[MAXLINE];
  int port;

  getargs_cg(hostname, &port, webaddr);

  printf("if you want to see commands, type 'help'\n");
  while (1)
  {
    myShell(hostname, &port, webaddr);
  }

  return (0);
}
