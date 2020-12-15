#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include "stems.h"

void getargs_pc(char *hostname, char *target, char *authKey, int *threshold)
{
    FILE *fp;

    fp = fopen("config-pc.txt", "r");
    if (fp == NULL)
        unix_error("config-pc.txt file does not open.");

    fscanf(fp, "%s", hostname);
    fscanf(fp, "%s", target);
    fscanf(fp, "%s", authKey);
    fscanf(fp, "%d", threshold);
    fclose(fp);
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    return size * nmemb;
}

void postMSG(char *msg, char *hostname, char *target, char *authKey, int threshold)
{
    char requestBody[MAXLINE];
    char sensorName[MAXLINE];
    char authHeader[MAXLINE];
    float sensorValue = 0;

    memset(&requestBody[0], 0, sizeof(requestBody));
    memset(&sensorName[0], 0, sizeof(sensorName));
    memset(&authHeader[0], 0, sizeof(authHeader));

    //TODO: 센서 값에 따른 push 유무 변경
    sscanf(msg, "%[^&]&%f", sensorName, &sensorValue);
    sprintf(requestBody, "{\"to\": %s,\"priority\": \"high\",\"notification\": {\"title\": \"%s\",\"body\": \"%f\"}}", target, sensorValue, sensorName);
    sprintf(authHeader, "Authorization: key=%s", authKey);
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();

    struct curl_slist *list = NULL;

    if (curl && (strstr(sensorName, "humidity") && (sensorValue > threshold)))
    {
        curl_easy_setopt(curl, CURLOPT_URL, hostname);

        list = curl_slist_append(list, "Content-Type: application/json");
        list = curl_slist_append(list, authHeader);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L);

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody);

        /* Perform the request, res will get the return code */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        res = curl_easy_perform(curl); // curl 실행 res는 curl 실행후 응답내용이
        curl_slist_free_all(list);     // CURLOPT_HTTPHEADER 와 세트

        /* Check for errors */
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl); // curl_easy_init 과 세트
    }
    curl_global_cleanup(); // curl_global_init 과 세트
    printf("\n");
}

void fifoInit(int *fd)
{
    if (mkfifo("fifo", 0666) == -1)
    {
        perror("mkfifo failed");
        exit(1);
    }
    if ((*fd = open("fifo", O_RDWR)) == -1)
    {
        perror("fifo open failed");
        exit(1);
    }
}

void my_handler(int s)
{
    printf("\nBYE\n");
    if (unlink("fifo") == -1)
    {
        printf("unlink failed plz remove \"fifo\" file manually\n");
    }
    exit(1);
}

int main(void)
{
    signal(SIGINT, my_handler);

    int fd, threshold;
    char msg[MAXLINE], hostname[MAXLINE], target[MAXLINE], authKey[MAXLINE];
    memset(&msg[0], 0, sizeof(msg));
    memset(&hostname[0], 0, sizeof(msg));
    memset(&target[0], 0, sizeof(msg));
    memset(&authKey[0], 0, sizeof(msg));
    fifoInit(&fd);

    getargs_pc(hostname, target, authKey, &threshold);

    while (1)
    {
        if (read(fd, msg, sizeof(msg)) == -1)
        {
            perror("read failed");
        }
        postMSG(msg, hostname, target, authKey, threshold);
    }
    return 0;
}