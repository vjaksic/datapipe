#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <winsock.h>
    #define bzero(p, l) memset(p, 0, l)
    #define bcopy(s, t, l) memmove(t, s, l)
#else
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/wait.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <strings.h>
    #define recv(x,y,z,a) read(x,y,z)
    #define send(x,y,z,a) write(x,y,z)
    #define closesocket(s) close(s)
    typedef int SOCKET;
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

int main(int argc, char *argv[])
{ 
    char buf[4096];
    struct sockaddr_in dest[2];
    SOCKET sockfd[2];    
    SOCKET maxsock = 0;
    int i;

#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32)
    /* Winsock needs additional startup activities */
    WSADATA wsadata;
    WSAStartup(MAKEWORD(1,1), &wsadata);
#endif

    

    /* check number of command line arguments */
    if (5 != argc) 
    {
        fprintf(stderr, "Usage: %s remotehost1 remoteport1 remotehost2 remoteport2\n", argv[0]);
        return -1;
    }

    /* connect to servers */
    for(i = 0; i < 2; ++i)
    {
        bzero(&dest[i], sizeof(dest[i]));
        dest[i].sin_family = AF_INET;
        dest[i].sin_port = htons((unsigned short) atol(argv[2 + i * 2]));
        if (!dest[i].sin_port)
        {
            fprintf(stderr, "invalid target port\n");
            return -1;
        }
        dest[i].sin_addr.s_addr = inet_addr(argv[1 + i * 2]);
        if (dest[i].sin_addr.s_addr == INADDR_NONE)
        {
            struct hostent *n;
            if (NULL == (n = gethostbyname(argv[1 + i * 2])))
            {
              perror("gethostbyname");
              return -1;
            }    
            bcopy(n->h_addr, (char *) &dest[i].sin_addr, n->h_length);
        }

        if ((sockfd[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        {
            perror("socket");
            return -1;
        }
        if (sockfd[i] > maxsock)
            maxsock = sockfd[i];
        if (connect(sockfd[i], (struct sockaddr *)&dest[i], sizeof(dest[i])))
        {
            perror("connect");
            closesocket(sockfd[i]);
        }
    }

    fd_set fdsr;
    int nbyt, closeneeded = 0;

    /* main polling loop. */
    while (1)
    {
        FD_ZERO(&fdsr);
        FD_SET(sockfd[0], &fdsr);
        FD_SET(sockfd[1], &fdsr);
        if (select(maxsock + 1, &fdsr, NULL, NULL, NULL) < 0) {
            return -1;
        }

        nbyt = 0;
        closeneeded = 0;

        if (FD_ISSET(sockfd[0], &fdsr))
        {
            if ((nbyt = recv(sockfd[0], buf, sizeof(buf), 0)) <= 0 || send(sockfd[1], buf, nbyt, 0) <= 0) 
                closeneeded = 1;
        }
        else if (FD_ISSET(sockfd[1], &fdsr))
        {
            if ((nbyt = recv(sockfd[1], buf, sizeof(buf), 0)) <= 0 || send(sockfd[0], buf, nbyt, 0) <= 0) 
                closeneeded = 1;
        }
        if (closeneeded) 
        {
            closesocket(sockfd[0]);
            closesocket(sockfd[1]);
        }      
    }
    
  return 0;
}
