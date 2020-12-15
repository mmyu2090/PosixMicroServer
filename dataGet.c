#include "stems.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include "mysql/mysql.h"

//
// This program is intended to help you test your web server.
// You can use it to test that you are correctly having multiple
// threads handling http requests.
//
// htmlReturn() is used if client program is a general web client
// program like Google Chrome. textReturn() is used for a client
// program in a embedded system.
//
// Standalone test:
// # export QUERY_STRING="name=temperature&time=3003.2&value=33.0"
// # ./dataGet.cgi

MYSQL *connection;

void connectToDB(char *response)
{
  connection = mysql_init(NULL);
  if (!mysql_real_connect(connection, "localhost", "mmyu2090", "1q2w3e4r!", NULL, 0, NULL, 0))
  {
    sprintf(response, "%s", "DB connection failed !! \n");
  }
  else
  {
    if (mysql_select_db(connection, "SensorData"))
    {
      sprintf(response, "%s", "DB databases can not be used !!\n");
    }
    else
    {
      //sprintf(response, "DB connected");
    }
  }
}

int queryToDB(char *data1, char *data2, char *data3, char *data4, char *response)
{
  MYSQL_RES *result = NULL;
  int id;
  char query[MAXLINE];
  memset(&query[0], 0, sizeof(query));

  if (strstr(data1, "command"))
  {
    if (strstr(data2, "LIST"))
    {
      sprintf(query, "%s", "select name from sensorList");
      mysql_query(connection, query);
      result = mysql_store_result(connection);
      if (mysql_errno(connection) > 0)
      {
        return 1;
      }
      else
      {
        MYSQL_ROW result2;
        while ((result2 = mysql_fetch_row(result)))
        {
          sprintf(response, "%s%s\n", response, result2[0]);
        }
      }
    }
    else if (strstr(data2, "INFO"))
    {
      sprintf(query, "%s%s%s", "select * from sensorList where name = '", data4, "'");
      mysql_query(connection, query);
      result = mysql_store_result(connection);
      if (mysql_errno(connection) > 0)
      {
        return 2;
      }
      else
      {
        MYSQL_ROW result2 = mysql_fetch_row(result);
        if (result2 == NULL)
        {
          sprintf(response, "%s%s\n", "There is no sensor named ", data4);
        }
        else
        {
          sprintf(response, "%s\n%s\n", result2[2], result2[3]);
        }
      }
    }
  }
  else if (strstr(data1, "NAME"))
  {
    sprintf(query, "%s%s%s", "select * from sensorList where name = '", data2, "'");
    mysql_query(connection, query);
    result = mysql_store_result(connection);
    if (mysql_errno(connection) > 0)
    {
      return 2;
    }
    else
    {
      MYSQL_ROW result2 = mysql_fetch_row(result);
      sscanf(result2[1], "%d", &id);

      memset(&query[0], 0, sizeof(query));

      sprintf(query, "%s%d%s%s", "select * from sensor", id, " order by idx DESC limit ", data4);
      mysql_query(connection, query);
      result = mysql_store_result(connection);
      if (mysql_errno(connection) > 0)
      {
        return 3;
      }
      else
      {
        int index = atoi(data4);
        for (int i = 0; i < index; i++)
        {
          MYSQL_ROW result3 = mysql_fetch_row(result);
          time_t temp = atol(result3[0]);
          sprintf(response, "%s%s%s\n", response, ctime(&temp), result3[1]);
        }
      }
    }
  }
  return 0;
}

void htmlReturn(void)
{
  char content[MAXLINE];
  char *buf;
  char *ptr;

  /* Make the response body */
  sprintf(content, "%s<html>\r\n<head>\r\n", content);
  sprintf(content, "%s<title>CGI test result</title>\r\n", content);
  sprintf(content, "%s</head>\r\n", content);
  sprintf(content, "%s<body>\r\n", content);
  sprintf(content, "%s<h2>Welcome to the CGI program</h2>\r\n", content);
  buf = getenv("QUERY_STRING");
  ptr = strsep(&buf, "&");
  while (ptr != NULL)
  {
    sprintf(content, "%s%s\r\n", content, ptr);
    ptr = strsep(&buf, "&");
  }
  sprintf(content, "%s</body>\r\n</html>\r\n", content);

  /* Generate the HTTP response */
  printf("HTTP/1.0 200 OK\r\n");
  printf("Server: My Web Server\r\n");
  printf("Content-Length: %d\r\n", strlen(content));
  printf("Content-Type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);
}

void textReturn(void)
{
  char *header;
  char response[MAXLINE];
  char *buf;
  char *ptr;
  char *data1;
  char *data2;
  char *data3;
  char *data4;

  memset(&response[0], 0, sizeof(response));

  buf = getenv("QUERY_STRING");
  data1 = strsep(&buf, "=");
  data2 = strsep(&buf, "&");
  data3 = strsep(&buf, "=");
  data4 = strsep(&buf, "&");

  connectToDB(response);
  int stat = queryToDB(data1, data2, data3, data4, response);
  if (stat > 0)
  {
    sprintf(response, "%s %s %d", response, "error : ", stat);
  }

  /* Generate the HTTP response */
  printf("HTTP/1.0 200 OK\r\n");
  printf("Server: My Web Server\r\n");
  printf("Content-Length: %d\n", strlen(response));
  printf("Content-Type: text/plain\r\n\r\n");
  printf(response);
  fflush(stdout);
}

int main(void)
{
  //htmlReturn();
  textReturn();
  return (0);
}
