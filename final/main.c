#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "sbuffer.h"
#include "config.h"
#include <stdint.h>
#include <time.h>
#include "connmgr.h"
#include "datamgr.h"
#include "sensor_db.h"
#include "sbuffer.h"
#include "lib/dplist.h"
#include "lib/tcpsock.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "errmacros.h"
#include <sqlite3.h>
#include "fifo.h"

#define FIFO_NAME "logFIFO"
#define MAX 80
#define LOG_NAME "gateway.log"
#define PORT_NUM 5678


char *str_result;
pthread_barrier_t* barrier;

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
sbuffer_t* buffer;
FILE* fp_sensor_map;
FILE *fp_fifo;
FILE* fp_log;
int finished_flag=0;
#define FILE_ERROR(fp,error_msg) 	do { \
					  if ((fp)==NULL) { \
					    printf("%s\n",(error_msg)); \
					    exit(EXIT_FAILURE); \
					  }	\
					} while(0)

void writeFIFO(char* msg)
{
    if(pthread_mutex_lock(&mutex)!=0)
    {
        perror("pthread_mutex_lock\n");
        exit(EXIT_FAILURE);
    }
       printf("writing FIFO\n");
       char* send_buf;
       asprintf(&send_buf,"%s",msg);
       if(fputs(send_buf,fp_fifo)==EOF)
       {   FFLUSH_ERROR(fflush(fp_fifo));
           pthread_mutex_unlock(&mutex);
           printf("FIFO write ERROR\n");
           exit(EXIT_FAILURE);
       }
    FFLUSH_ERROR(fflush(fp_fifo));
    if(pthread_mutex_unlock(&mutex)!=0)
    {
        perror("pthread_mutex_unlock\n");
        exit(EXIT_FAILURE);
    }
    free(send_buf);


}

void* connmgr()
{

	printf("connmgr thread now\n");
    connmgr_listen(PORT_NUM,&buffer);
    connmgr_free();

    pthread_exit(NULL);
}

void* datamgr()
{

	printf("datamgr thread now\n");
    datamgr_parse_sensor_data(fp_sensor_map,&buffer);
    datamgr_free();

    pthread_exit(NULL);


}


void* storage()
{

    DBCONN* db;
	printf("storage thread now\n");
    while(buffer!=NULL)
   {

     db=init_connection(1);

     storagemgr_parse_sensor_data(db, &buffer);
     if(buffer==NULL)
     {
      disconnect(db);
      break;
     }
 }

   pthread_exit(NULL);
   
}


void child_process()
{
    printf("child process %d start\n",getpid());
    int result;
    char recv_buf[MAX];
    int sequence=1;
    result = mkfifo(FIFO_NAME, 0666);
    CHECK_MKFIFO(result);

    fp_log = fopen(LOG_NAME, "w");
    FILE_OPEN_ERROR(fp_log);
    fp_fifo = fopen(FIFO_NAME, "r");
    FILE_OPEN_ERROR(fp_fifo);

    printf("get message through FIFO\n");

    while(finished_flag!=1) {

       str_result=fgets(recv_buf, MAX, fp_fifo);

       if(str_result!=NULL) {
           printf("from fifo:%s", recv_buf);
           fprintf(fp_log, "%d %ld %s", sequence, time(NULL), recv_buf);
           fflush(fp_log);
           sequence++;
       }

   }
    result=fclose(fp_fifo);
    FILE_CLOSE_ERROR(result);
    result=fclose(fp_log);
    FILE_CLOSE_ERROR(result);
    printf("End child process\n");
    exit(3);

}

int main()
{
   pid_t my_pid,child_pid;
   my_pid=getpid();
   printf("parent pid %d started\n",my_pid);
   child_pid=fork();
   SYSCALL_ERROR(child_pid);

   if(child_pid==0)   //child process
   {
     child_process();
   }
   if(child_pid<0)
   {perror("fork error\n");}
   else           //this process
   {
    int result=mkfifo(FIFO_NAME,0666);
    CHECK_MKFIFO(result);

    fp_fifo=fopen(FIFO_NAME,"w");
    FILE_OPEN_ERROR(fp_fifo);

    fp_sensor_map=fopen("room_sensor.map","r");
    FILE_ERROR(fp_sensor_map,"can't open sensor map\n");


    sbuffer_init(&buffer);  //init one sbuffer
    pthread_t connmgr_p;  //writer: connmgr
    pthread_t datamgr_p;//reader: datamgr&storage
    pthread_t storage_p;
    int ret;
    ret=pthread_create(&connmgr_p,NULL,connmgr,NULL);     //create pthreads
    if(ret==0) printf("create connection thread success\n");
    else printf("create connection thread false\n");
   
    ret=pthread_create(&datamgr_p,NULL,datamgr,NULL);
    if(ret==0) printf("create data thread success\n");
    else printf("create data thread false\n");

    ret=pthread_create(&storage_p,NULL,storage,NULL);
    if(ret==0) printf("create storge thread success\n");
    else printf("create storage thread false\n");

   
     int temp=pthread_join(connmgr_p,NULL);               //wait till pthreads end

	 if(temp!=0) printf("cannot join write thread\n");
 	 printf("connection thread is end\n");



     temp=pthread_join(datamgr_p,NULL);
     if(temp!=0) printf("cannot join read thread\n");
     printf("data thread is end\n");


	 temp=pthread_detach(storage_p);
	 if(temp!=0) printf("cannot join read thread\n");
	 printf("storage thread is end\n");


     finished_flag=1;
     pthread_exit(NULL);
     pthread_mutex_destroy(&mutex);

     fclose(fp_fifo);
     fclose(fp_sensor_map);


     exit(3);
  }

   return 0;
}
