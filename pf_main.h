/*
 * Copyright (C) Chesley Diao
 */

#define PF_LOG "/var/log/pf_proc.log"

typedef struct{
    char mysrvip[24];
    char myuser[24];
    char mypw[24];
    char db[24];
    char pfpath[128];
    int  mysrvport;
    int  itime;
}pconf_s;

typedef struct{
    char tcpnum[24];
    char upload[24];
    char download[24];
    char ip[48];
}vuser_s;


