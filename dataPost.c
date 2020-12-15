#include "stems.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include "mysql/mysql.h"

//
// This program is intended to help you test your web server.
//

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
      sprintf(response, "DB connected");
    }
  }
}

int queryToDB(char *sensorName, long time, float sensorValue, char *response)
{
  MYSQL_RES *result = NULL;
  char query[MAXLINE];
  memset(&query[0], 0, sizeof(query));
  int id;
  int index;
  float ave;

  sprintf(query, "%s", "CREATE TABLE IF NOT EXISTS sensorList(name VARCHAR(50) NOT NULL UNIQUE,id INT NOT NULL AUTO_INCREMENT,cnt INT,ave FLOAT,PRIMARY KEY(id))");
  mysql_query(connection, query);
  result = mysql_store_result(connection);
  if (mysql_errno(connection) > 0)
  {
    return 1;
  }
  memset(&query[0], 0, sizeof(query));

  sprintf(query, "%s%s%s", "INSERT INTO sensorList(name, cnt) VALUES('", sensorName, "', 1) ON DUPLICATE KEY UPDATE cnt = cnt + 1");
  mysql_query(connection, query);
  result = mysql_store_result(connection);
  if (mysql_errno(connection) > 0)
  {
    return 2;
  }
  memset(&query[0], 0, sizeof(query));

  sprintf(query, "%s", "SELECT COUNT(*) FROM sensorList");
  mysql_query(connection, query);
  result = mysql_store_result(connection);
  if (mysql_errno(connection) > 0)
  {
    return 3;
  }
  else
  {
    MYSQL_ROW result2 = mysql_fetch_row(result);
    sscanf(result2[0], "%d", &index);
  }
  memset(&query[0], 0, sizeof(query));

  sprintf(query, "%s%d", "alter table sensorList auto_increment = ", index + 1);
  mysql_query(connection, query);
  result = mysql_store_result(connection);
  if (mysql_errno(connection) > 0)
  {
    return 4;
  }
  memset(&query[0], 0, sizeof(query));

  sprintf(query, "%s%s%s", "select * from sensorList where name like'", sensorName, "'");
  mysql_query(connection, query);
  result = mysql_store_result(connection);
  if (mysql_errno(connection) > 0)
  {
    return 5;
  }
  else
  {
    MYSQL_ROW result2 = mysql_fetch_row(result);
    sscanf(result2[1], "%d", &id);
  }
  memset(&query[0], 0, sizeof(query));

  sprintf(query, "%s%d%s", "CREATE TABLE IF NOT EXISTS sensor", id, "(time LONG NOT NULL,value FLOAT NOT NULL,idx INT NOT NULL AUTO_INCREMENT, PRIMARY KEY(idx))");
  mysql_query(connection, query);
  result = mysql_store_result(connection);
  if (mysql_errno(connection) > 0)
  {
    return 8;
  }
  memset(&query[0], 0, sizeof(query));

  sprintf(query, "%s%d%s%ld%s%lf%s", "INSERT INTO sensor", id, "(time, value) VALUES(", time, ",", sensorValue, ")");
  mysql_query(connection, query);
  result = mysql_store_result(connection);
  if (mysql_errno(connection) > 0)
  {
    return 9;
  }

  sprintf(query, "%s%d", "select avg(value) from sensor", id);
  mysql_query(connection, query);
  result = mysql_store_result(connection);
  if (mysql_errno(connection) > 0)
  {
    return 6;
  }
  else
  {
    MYSQL_ROW result2 = mysql_fetch_row(result);
    sscanf(result2[0], "%f", &ave);
  }
  memset(&query[0], 0, sizeof(query));

  sprintf(query, "%s%f%s%s%s", "UPDATE sensorList SET ave = ", (float)(int)ave, "WHERE name = '", sensorName, "'");
  mysql_query(connection, query);
  result = mysql_store_result(connection);
  if (mysql_errno(connection) > 0)
  {
    return 7;
  }
  memset(&query[0], 0, sizeof(query));

  return 0;
}

int pushData(char *sensorName, float sensorValue)
{
  int fd;
  char msg[MAXLINE];

  memset(&msg[0], 0, sizeof(msg));
  sprintf(msg, "%s&%f", sensorName, sensorValue);

  if ((fd = open("fifo", O_WRONLY)) == -1)
  {
    perror("fifo open failed");
  }

  if (write(fd, msg, strlen(msg) + 1) == -1)
  {
    perror("fifo  write failed");
  }

  return 0;
}

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

int main(int argc, char *argv[])
{
  char *str;
  char astr[MAXLINE];
  char response[MAXLINE];
  char *buf;
  char *ptr;
  int len;

  char start[20];
  char end[20];

  getdoubledt(start);

  char sensorName[MAXLINE];
  long time;
  float sensorValue;

  memset(&response[0], 0, sizeof(response));
  memset(&astr[0], 0, sizeof(astr));
  memset(&sensorName[0], 0, sizeof(sensorName));

  str = getenv("REQUEST_METHOD");
  len = atoi(getenv("CONTENT_LENGTH")) + 1;
  read(STDIN_FILENO, astr, len);
  ptr = strsep(&buf, "&");
  while (ptr != NULL)
  {
    sprintf(astr, "%s%s\n", astr, ptr);
    ptr = strsep(&buf, "&");
  }

  sscanf(astr, "name=%[^&]&time=%ld&value=%f", sensorName, &time, &sensorValue);
  connectToDB(response);
  int result = queryToDB(sensorName, time, sensorValue, response);

  if (result == 0)
  {
    getdoubledt(end);
    sprintf(response, "%s %s %d start : %s, end : %s", response, "DB OK", result, start, end);
  }
  else
  {
    sprintf(response, "%s %s %d", response, "DB ERR", result);
  }

  printf("HTTP/1.0 200 OK\r\n");
  printf("Server: My Web Server\r\n");
  printf("Content-Length: %d\r\n", strlen(response));
  printf("Content-Type: text/plain\r\n\r\n");
  printf(response);

  pushData(sensorName, sensorValue);

  fflush(stdout);
  return (0);
}
