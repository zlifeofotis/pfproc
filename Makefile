CC = cc
CFLAGS = -g -O -Wall -I/usr/local/mysql/include/mysql
pfproc: pf_main.o pf_proc.o
        $(CC) -o $@ pf_main.o pf_proc.o /usr/local/mysql/lib/mysql/libmysqlclient.so
pf_main.o: pf_main.c pf_main.h
        $(CC) $(CFLAGS) -c -o $@ $<
pf_proc.o: pf_proc.c pf_main.h
        $(CC) $(CFLAGS) -c -o $@ $<
