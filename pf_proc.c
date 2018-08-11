/*
 * Copyright (C) Chesley Diao
 */


#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <errmsg.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "pf_main.h"

char* pf_ltrim(char* str){
        while(isspace(*str)){
                str++;
    }
        return(str);
}

char* pf_rtrim(char* str){
        int strl;
        strl = strlen(str);
        while(isspace(*(str+(strl-1))) && strl > 0){
                *(str+(strl-1)) = 0;
                strl--;
    }
        return(str);
}

/*
 *take the equals sign on both sides of string
 */
void pf_strtok(char* str,char* left,char *right){
        char* strptr[2];
        char* saveptr;
        int i;
        for(i = 0;i < 2;i++){
                if((strptr[i] = strtok_r(str,"=",&saveptr)) == NULL)
                        break;
                str = NULL;
    }
        if(strptr[0])
                strcpy(left,strptr[0]);
        if(strptr[1])
                strcpy(right,strptr[1]);

}
/*
 *after reading parameter file,init parameter
 */
void pf_param_get(FILE* fp,pconf_s* pconf){

    char str[1024];
    char* str1;
    char left[512];
    char* left1;
    char right[512];
    char* right1;

    while(fgets(str,1024,fp) != NULL){
        memset(left,0,512);
        memset(right,0,512);
        str1 = pf_ltrim(str);
        if(*str1 == '#' || *str1 == 0)
            continue;
        pf_strtok(str1,left,right);
        left1 = pf_rtrim(left);
        right1 = pf_ltrim(right);
        right1 = pf_rtrim(right1);
        if(strcmp(left1,"mysrvip") == 0){
            strcpy(pconf->mysrvip,right1);
            pconf->mysrvip[23] = '\0';
        }
        else if(strcmp(left1,"mysrvport") == 0)
            pconf->mysrvport = atoi(right1);
        else if(strcmp(left1,"myuser") == 0){
            strcpy(pconf->myuser,right1);
            pconf->myuser[23] = '\0';
        }
        else if(strcmp(left1,"mypw") == 0){
            strcpy(pconf->mypw,right1);
            pconf->mypw[23] = '\0';
        }
        else if(strcmp(left1,"db") == 0){
            strcpy(pconf->db,right1);
            pconf->db[23] = '\0';
        }
        else if(strcmp(left1,"itime") == 0)
            pconf->itime = atoi(right1);
        else if(strcmp(left1,"pfpath") == 0){
            strcpy(pconf->pfpath,right1);
            pconf->pfpath[127] = '\0';
        }
        else
            continue;
    }
}
void pf_connect_db(MYSQL* mysql,pconf_s* pfconf,FILE* fp){
    time_t tm;
    char strtime[26];
    mysql_init(mysql);
    while(!mysql_real_connect(mysql,pfconf->mysrvip,pfconf->myuser, \
                       pfconf->mypw,pfconf->db,pfconf->mysrvport,NULL,0)){
        tm = time(NULL);
        strcpy(strtime,ctime(&tm));
        strtime[24] = '\0';
        fprintf(fp,"[ %s ] %s mysql_real_connect():%s %d %s %s %s\n", \
                strtime,mysql_error(mysql),pfconf->mysrvip, \
                pfconf->mysrvport,pfconf->myuser,pfconf->mypw,pfconf->db);
        sleep(60);
    }
        tm = time(NULL);
        strcpy(strtime,ctime(&tm));
        strtime[24] = '\0';
        fprintf(fp,"[ %s ] mysql connection success\n",strtime);
        fflush(fp);
}

void pf_close_db(MYSQL* mysql_p){
    mysql_close(mysql_p);
}

void write_pf(vuser_s* vuser_p,FILE* fp){
    fprintf(fp,"altq on $exp %s, %s, %s, %s\n",vuser_p->tcpnum, \
            vuser_p->upload,vuser_p->download,vuser_p->ip);
    fflush(fp);
}

void pf_wr_myerror(MYSQL* mysql_p,FILE* fp){
    time_t tm;
    char strtime[26];
    tm = time(NULL);
    strcpy(strtime,ctime(&tm));
    strtime[24] = '\0';
    fprintf(fp,"[ %s ] %s\n",strtime,mysql_error(mysql_p));
    fflush(fp);
}

void pf_proc(MYSQL* mysql,pconf_s* pfconf){
    char sqlstr[512];
    char sqlstr1[512];
    char str[128];
    int  sqlstr_l;
    int  sqlstr1_l;
    time_t tm;
    MYSQL_RES *res;
    MYSQL_ROW row;
    FILE* fp;
    vuser_s vuser;
    char strtime[26];

    bzero(&vuser,sizeof(vuser_s));
    strcpy(sqlstr,"select zt from pfzt where id=1");
    strcpy(sqlstr1,"select tcpnum,upload,download,homeip from vpnuser where cash > 0");
    sqlstr_l  = strlen(sqlstr);
    sqlstr1_l = strlen(sqlstr1);
    while(1){
        if(mysql_real_query(mysql,sqlstr,sqlstr_l) == 0){
            res = mysql_store_result(mysql);
            if(!res){
                pf_wr_myerror(mysql,stdout);
                if(mysql_errno(mysql) == CR_SERVER_LOST || mysql_errno(mysql) == CR_SERVER_GONE_ERROR || \
                        mysql_errno(mysql) == CR_IPSOCK_ERROR ||  mysql_errno(mysql) == CR_TCP_CONNECTION)
                    pf_connect_db(mysql,pfconf,stdout);
                //goto procpoint;
                continue;
            }
        }
        else{
            pf_wr_myerror(mysql,stdout);
            if(mysql_errno(mysql) == CR_SERVER_LOST || mysql_errno(mysql) == CR_SERVER_GONE_ERROR || \
                mysql_errno(mysql) == CR_IPSOCK_ERROR ||  mysql_errno(mysql) == CR_TCP_CONNECTION)
                pf_connect_db(mysql,pfconf,stdout);
            //goto procpoint;
            continue;
        }
        row = mysql_fetch_row(res);
        if(!row){
            pf_wr_myerror(mysql,stdout);
            if(mysql_errno(mysql) == CR_SERVER_LOST || mysql_errno(mysql) == CR_SERVER_GONE_ERROR || \
                mysql_errno(mysql) == CR_IPSOCK_ERROR ||  mysql_errno(mysql) == CR_TCP_CONNECTION)
                pf_connect_db(mysql,pfconf,stdout);
            mysql_free_result(res);
            //goto procpoint;
            continue;
        }
        mysql_free_result(res);
        if(atoi(row[0]) != 1){
            goto procpoint;
        }

        if(mysql_real_query(mysql,sqlstr1,sqlstr1_l) == 0){
            res = mysql_use_result(mysql);
            if(!res){
                pf_wr_myerror(mysql,stdout);
                if(mysql_errno(mysql) == CR_SERVER_LOST || mysql_errno(mysql) == CR_SERVER_GONE_ERROR || \
                    mysql_errno(mysql) == CR_IPSOCK_ERROR ||  mysql_errno(mysql) == CR_TCP_CONNECTION)
                    pf_connect_db(mysql,pfconf,stdout);
                //goto procpoint;
                continue;
            }
        }
        else{
            pf_wr_myerror(mysql,stdout);
            if(mysql_errno(mysql) == CR_SERVER_LOST || mysql_errno(mysql) == CR_SERVER_GONE_ERROR || \
                mysql_errno(mysql) == CR_IPSOCK_ERROR ||  mysql_errno(mysql) == CR_TCP_CONNECTION)
                pf_connect_db(mysql,pfconf,stdout);
            //goto procpoint;
            continue;
        }
        sprintf(str,"%s.new",pfconf->pfpath);
        if((fp = fopen(str,"w+")) == NULL){
            mysql_free_result(res);
            tm = time(NULL);
            strcpy(strtime,ctime(&tm));
            strtime[24] = '\0';
            fprintf(stdout,"[ %s ][ %s ] Open File Error: %s =>EXIT 1\n",strtime,str,strerror(errno));
            fflush(stdout);
            continue;
        }
        while((row = mysql_fetch_row(res)) != NULL){
            strncpy(vuser.tcpnum,row[0],23);
            vuser.tcpnum[23] = '\0';
            strncpy(vuser.upload,row[1],23);
            vuser.upload[23] = '\0';
            strncpy(vuser.download,row[2],23);
            vuser.download[23] = '\0';
            strncpy(vuser.ip,row[3],47);
            vuser.ip[47] = '\0';
            write_pf(&vuser,fp);
        }
        fclose(fp);
        if(mysql_errno(mysql)){
            pf_wr_myerror(mysql,stdout);
            if(mysql_errno(mysql) == CR_SERVER_LOST || mysql_errno(mysql) == CR_SERVER_GONE_ERROR || \
                mysql_errno(mysql) == CR_IPSOCK_ERROR ||  mysql_errno(mysql) == CR_TCP_CONNECTION)
                pf_connect_db(mysql,pfconf,stdout);
            mysql_free_result(res);
            //goto procpoint;
            continue;
        }
        mysql_free_result(res);
        if(rename(str,pfconf->pfpath)){
            tm = time(NULL);
            strcpy(strtime,ctime(&tm));
            strtime[24] = '\0';
            fprintf(stdout,"[ %s ][ %s ] => [ %s ] ERROR: %s\n",strtime,str,pfconf->pfpath,strerror(errno));
            fflush(stdout);
        }
        procpoint:
            sleep(pfconf->itime);
    }

}




