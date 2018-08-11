/*
 * Copyright (C) Chesley Diao
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <mysql.h>
#include "pf_main.h"

void pf_param_get(FILE* ,pconf_s* );
void pf_connect_db(MYSQL* ,pconf_s* ,FILE* );
void pf_close_db(MYSQL* );
void pf_proc(MYSQL* ,pconf_s* );

int main(int argc,char **argv){

    pid_t pid;
    int ch;
//    int rcode;
    FILE* fp = NULL;
//    FILE* fp1;
//    FILE* fp2;
    int fd;
    pconf_s pfconf;
//    time_t tm;
    MYSQL mysql;
//    char strtime[26];

    bzero(&pfconf,sizeof(pconf_s));
    if(argc != 3){
        puts("Usage:\n  pfproc <-f pf_proc.conf>\n");
        exit(1);
    }
    while((ch = getopt(argc,argv,"f:")) != -1){
        switch(ch){
            case 'f':
                if((fp = fopen(optarg,"r")) == NULL){
                    printf("Open %s:    %s\n",optarg,strerror(errno));
                    exit(1);
                }
                break;
            case '?':
                puts("  Invalid Parameter!\n");
                puts("Usage:\n  pfproc <-f pf_proc.conf>\n");
                exit(1);
        }
    }
/*
    if((fp1 = fopen(PF_LOG,"a")) == NULL){
        printf("Open %s:        %s\n",PF_LOG,strerror(errno));
        exit(1);
        }
*/
    if((fd = open(PF_LOG,O_APPEND|O_CREAT|O_RDWR)) == -1){
        printf("Open %s:        %s\n",PF_LOG,strerror(errno));
        exit(1);
    }
    if(dup2(fd,STDOUT_FILENO) == -1){
        perror("dup2 TO STDOUT_FILENO");
        exit(1);
    }
    close(fd);
    pid = fork();
    switch(pid){
        case -1:
            perror("fork()");
            exit(1);
        case 0:
            break;
        default:
            exit(0);
    }
    setsid();
    chdir("/");
    close(STDIN_FILENO);
    close(STDERR_FILENO);

    signal(SIGHUP,SIG_IGN);
    signal(SIGINT,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);
    signal(SIGTERM,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);

    pf_param_get(fp,&pfconf);
    fclose(fp);
    /*
    if(rcode){
        tm = time(NULL);
        strcpy(strtime,ctime(&tm));
        strtime[24] = '\0';
        fprintf(fp1,"%s ERROR CODE: %d :pf_param_app()ERROR EXIT 1\n",strtime,rcode);
        exit(1);
        }
    */
    /*
    if((fp = fopen(pfconf.pfpath,"a")) == NULL){
        tm = time(NULL);
        strcpy(strtime,ctime(&tm));
        strtime[24] = '\0';
        fprintf(stdout,"[ %s ] Open File Error: [ %s ] =>EXIT 1\n",strtime,pfconf.pfpath);
        fflush(stdout);
        exit(1);
    }
    */
    pf_connect_db(&mysql,&pfconf,stdout);
    pf_proc(&mysql,&pfconf);
    pf_close_db(&mysql);
    close(STDOUT_FILENO);
    return(0);
}

