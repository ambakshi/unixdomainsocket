#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <getopt.h>


/*
 * Unix Domain Server
 */
int uds(char* path) {
    int s, len;
    struct sockaddr_un local;

    if ((s=socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, path);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(s, (struct sockaddr *)&local, len) == -1) {
        perror("bind");
        close(s);
        exit(1);
    }
    if (listen(s, 5) == -1) {
        perror("listen");
        close(s);
        exit(1);
    }

    char str[200];
    for(;;) {
        int done, n, t, s2;
        struct sockaddr_un remote;

        printf("Waiting...\n");
        t = sizeof(remote);

        if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
            perror("accept");
            close(s);
            exit(1);
        }

        printf("Connected\n");

        done = 0;
        do {
            n = recv(s2, str, 100, 0);
            if (n <= 0) {
                if (n < 0) perror("recv");
                done = 1;
            }
            if (!done) {
                if (send(s2, str, n, 0) < 0) {
                    perror("send");
                    done = 1;
                }
            }
        } while(!done);
        close(s2);
    }
    return 0;
}

/*
 * Unix Domain Client
 */
int udc(char* path) {
    int s, len;
    struct sockaddr_un remote;

    if ((s=socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, path);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        exit(1);
    }
    printf("Connected\n");
    char str[200];
    int t;
    while(printf("> "), fgets(str,100, stdin), !feof(stdin)) {
        if (send(s,str, strlen(str), 0) == -1) {
            perror("send");
            exit(1);
        }
        if ((t=recv(s,str,100,0)) > 0) {
            str[t]=0;
            printf("echo > %s", str);
        } else {
            if (t < 0) perror("recv");
            else printf("Server closed connection\n");
            exit(1);
        }
    }
    close(s);
    return 0;
}

int main(int argc, char** argv) {
    int c;
    int aflag = 0, bflag = 0;
    char* cvalue = NULL;
    char* dvalue = "";
    char* session = NULL;

    // optopt, opterr, optarg
    opterr = 0;
    while ((c = getopt (argc, argv, "habc:d:S:x:")) != -1) {
        switch (c) {
            case 'a':
                aflag = 1;
                break;
            case 'b':
                bflag = 1;
                break;
            case 'c':
                cvalue = optarg;
                break;
            case 'd':
                dvalue = optarg;
                break;
            case 'S':
                session = optarg;
                break;
            case '?':
                fprintf(stderr, "-%c is missing argument\n", optopt);
                exit(1);
            case 'h':
            default:
                fprintf(stderr, "%s -S session -a -b -c <arg> -d <arg>\n", argv[0]);
                exit(1);
        }
    }
    if (session) {
        int pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            /* child becomes daemon */
            close(0);
            close(1);
            close(2);
            setsid();
            uds(session);
            exit(0);
        }
        printf("Forked daemon %d\n", pid);
        exit(0);
    } else if (cvalue) {
        udc(cvalue);
    }
    printf("%d %d %s %s %s\n", aflag, bflag, cvalue, dvalue, session);
    return 0;
}


