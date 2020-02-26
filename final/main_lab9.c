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
#define NUM_WRITERS 1
#define NUM_READERS 2


#define FILE_ERROR(fp,error_msg) 	do { \
					  if ((fp)==NULL) { \
					    printf("%s\n",(error_msg)); \
					    exit(EXIT_FAILURE); \
					  }	\
					} while(0)

FILE* fp_in,*fp_out;
sbuffer_t* buffer;

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

void* writer()
{
   sensor_data_t *data=malloc(sizeof(sensor_data_t));

	if(pthread_mutex_lock(&mutex)!=0)
	{
	   perror("pthread_mutex_lock\n");
	   exit(EXIT_FAILURE);
		}


   
   while(feof(fp_in)!=1)
   {
	fread(&(data->id),sizeof(sensor_id_t),1,fp_in);
	fread(&(data->value),sizeof(sensor_value_t),1,fp_in);
	fread(&(data->ts),sizeof(sensor_ts_t),1,fp_in);
	int result=sbuffer_insert(buffer,data);
	if(result==SBUFFER_SUCCESS) 
	{	
	   //printf("writing\n");
	   
	
		}else
		{pthread_exit(NULL);}
		}
		
	
	if(pthread_mutex_unlock(&mutex)!=0)
	{
	  perror("pthread_mutex_unlock\n");
	  exit(EXIT_FAILURE);
		}
	
	printf("write over\n");
	//sleep(1);
	free(data);
	data=NULL;
	pthread_exit(NULL);	
			}




void* reader()
{        
	 sensor_data_t *data=malloc(sizeof(sensor_data_t));
	 int result=sbuffer_remove(buffer,data);

	if(pthread_mutex_lock(&mutex)!=0)
	{
	   perror("pthread_mutex_lock\n");
	   exit(EXIT_FAILURE);
		}


     while(result==SBUFFER_SUCCESS)
     { 
	
	 
	  fwrite(&(data->id),sizeof(sensor_id_t),1,fp_out);
	  fwrite(&(data->value),sizeof(sensor_value_t),1,fp_out);
	  fwrite(&(data->ts),sizeof(sensor_ts_t),1,fp_out);
	  printf("Sensor_id:%d,value:%lf,time:%ld\n",data->id,data->value,data->ts);
	  
	 
	 if(result==SBUFFER_NO_DATA)
		{printf("No data\n");
		  	exit(0);	}
		
	//if(result==SBUFFER_FAILURE){pthread_exit(NULL);}  
	result=sbuffer_remove(buffer,data);
      }
		
	
	if(pthread_mutex_unlock(&mutex)!=0)
	{
	  perror("pthread_mutex_unlock\n");
	  exit(EXIT_FAILURE);
		}
	
	//sleep(2);
	free(data);
	data=NULL;
	pthread_exit(NULL);
	 	  
       	  	
}


int main(){
  sbuffer_init(&buffer);
  pthread_t writer_threads[NUM_WRITERS];
  pthread_t reader_threads[NUM_READERS];
  fp_in=fopen("sensor_data","r");
  FILE_ERROR(fp_in,"can't open sensor_data\n");
  fp_out=fopen("sensor_data_out","w");
  FILE_ERROR(fp_out,"can't open sensor_data_out\n");
 
  for(int i=0;i<NUM_WRITERS;i++)
  {
  	int ret=pthread_create(&writer_threads[i],NULL,writer,NULL);
	if(ret==0) printf("create write thread%d success\n",i+1);
	else printf("create thread%d false\n",i+1);
	
		}
  for(int i=0;i<NUM_READERS;i++)
  {
	
	
  	int ret=pthread_create(&writer_threads[i],NULL,reader,NULL);
	if(ret==0) printf("create read thread%d success\n",i+1);
	else printf("create thread%d false\n",i+1);
	
		}
  for(int i=0;i<NUM_WRITERS;i++)
  {
	int temp=pthread_join(writer_threads[i],NULL);
	sleep(1);
	if(temp!=0) printf("cannot join write thread%d\n",i+1);
 	 
	 printf("write thread%d is end\n",i+1);
	 pthread_exit(NULL);
	
	//pthread_cancel(writer_threads[i]);
		}
    for(int i=0;i<NUM_READERS;i++)
  {
	int temp=pthread_join(reader_threads[i],NULL);
	sleep(2);
	if(temp!=0) printf("cannot join read thread%d\n",i+1);
	
	printf("read thread%d is end\n",i+1);
	
	  pthread_exit(NULL);

	//pthread_cancel(reader_threads[i]);
		}
  
  pthread_mutex_destroy(&mutex); 

  fclose(fp_in);
  fclose(fp_out);
  sbuffer_free(&buffer);
  pthread_exit(NULL);
  return 0;

		}
