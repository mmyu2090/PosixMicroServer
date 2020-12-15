#ifndef __REQUEST_H__

enum {STATIC, DYNAMIC};

void initWatch(void);
double getWatch(void);
void requestHandle(int connfd, double arrivalTime);
int requestType(int fd);

#endif
