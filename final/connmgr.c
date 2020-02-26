#define _GNU_SOURCE
#include "lib/dplist.h"
#include "lib/tcpsock.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include "config.h"
#include <limits.h>
#include "sbuffer.h"
#include "fifo.h"
#define FILE_ERROR(fp,error_msg) 	do { \
					  if ((fp)==NULL) { \
					    printf("%s\n",(error_msg)); \
					    exit(EXIT_FAILURE); \
					  }	\
					} while(0)


dplist_t * list=NULL;

struct sockaddr_in addrport;

struct sbuffer_data {
	sensor_data_t data;
};

typedef struct sbuffer_data sbuffer_data_t;

typedef struct sbuffer_node {
	struct sbuffer_node * next;
	sbuffer_data_t element;
} sbuffer_node_t;



struct sbuffer {
	sbuffer_node_t * head;
	sbuffer_node_t * tail;
};
typedef struct sbuffer sbuffer_t;


int max=0;
time_t now;

char* msg;

typedef struct 
{
  struct pollfd pd;
  time_t ts;
    }poll_element_t;


#define TIMEOUT 5
#define OPEN_MAX 1024


  void *element_copy(void * element);			  
  void element_free(void ** element);
  int element_compare(void * x, void * y);

int element_compare(void *x,void *y)
{  
   if((int*)x>(int*)y)
   return 1;
   if((int*)x<(int*)y)
   return -1;
   else return 0;


	}

void* element_copy(void *element)
{
    poll_element_t *copy_element=malloc(sizeof(poll_element_t));
    copy_element->pd=((poll_element_t*)element)->pd;
    copy_element->ts=((poll_element_t*)element)->ts;
    return (void*) copy_element;
	}

void element_free(void **element) 
{
    free(*element);
    *element= NULL;
	}


void connmgr_listen(int port_number,sbuffer_t ** buffer)
{
  
   
   list=dpl_create(&element_copy,&element_free,&element_compare);
   
   tcpsock_t * server;
   struct pollfd pollfd[OPEN_MAX];
 
   sensor_data_t* data=malloc(sizeof(sensor_data_t)); 
   poll_element_t* element1=malloc(sizeof(poll_element_t));

  
   int sockfd=socket(AF_INET,SOCK_STREAM,0);

   int update=1;

   memset(&addrport,0,sizeof(addrport));
   addrport.sin_family=AF_INET;
   addrport.sin_port=htons(port_number);
   addrport.sin_addr.s_addr=htonl(INADDR_ANY);
   bind(sockfd,(struct sockaddr*)&addrport,sizeof(addrport));
   
   listen(sockfd,10);
 
   if(tcp_passive_open(&server,port_number)==TCP_NO_ERROR)
   {printf("ERROR\n");}
   
/////////start poll//////////////

   for(int i=0;i<OPEN_MAX;i++)
   {
     pollfd[i].fd=-1;            //init poll
	}
    
    pollfd[0].fd=sockfd;
    pollfd[0].events=POLLIN;


    while(1)
    {

       int ret=poll(pollfd,max+1,TIMEOUT*1000);   //call poll, Number of the reaction
       if(ret==0)
       {
 	 perror("timeout\n");
	 break;

 	}

 	if((pollfd[0].revents&POLLIN)==POLLIN)   //if sockfd exist
	{
	   struct sockaddr_in clientaddr;
	   unsigned int clilen=sizeof(struct sockaddr_in);
	   int new_sock;
	   printf("new client");
	   new_sock=accept(sockfd,(struct sockaddr*)&clientaddr,&clilen); //accept new link
         
	   if(new_sock==-1)
		{
		   
		   if(errno==EINTR) continue;
		   else
			{
			   perror("accept error:");
			   exit(1);
			}

		 }
	   fprintf(stdout,"accept new client\n");

	    asprintf(&msg,"new socket %d accept\n",new_sock);

		writeFIFO(msg);
	    free(msg);

	   int count;
           
	   for(count=0;count<OPEN_MAX;count++) 
           {

		if(pollfd[count].fd<0) 
		 {
		  pollfd[count].fd= new_sock;
		  pollfd[count].events=POLLIN;
		  
            
		  element1->pd=pollfd[count];
		  element1->ts=time(&now);
		  dpl_insert_at_index(list,element1,count,false); 
                  update++;
		  break;
	         }	
		  
							
	     }
	
	    

		if(update>max) max=update;
			
		if(--ret<=0) continue;   //if no reaction, continue poll

			
                   }

           
	   for(int i=1;i<=max;i++)
	   {

		
		for(int j=0;j<dpl_size(list);j++)
		{

		element1=( poll_element_t*)dpl_get_element_at_index(list,j);
		
		time_t t=time(&now);
                
		  if ((element1->pd.fd)==pollfd[i].fd)
		  {
			

			if((t-element1->ts)>TIMEOUT)
		        {
		  	   printf("socket%d closed\n",element1->pd.fd);

		  	   close(pollfd[i].fd);
		
		  	   pollfd[i].fd=-1;
                           
		  	   dpl_remove_at_index(list,j,false);

		  	   break;
			} 	
			element1->ts=time(&now);
			element1->pd=pollfd[i];
			
			break;
		 }
		}		
		

		
		if(pollfd[i].fd<0) continue;

 		if(pollfd[i].revents&(POLLIN|POLLERR))
		{		
			int rec1 = recv(pollfd[i].fd,&data->id,sizeof(data->id),0);
			int rec2 = recv(pollfd[i].fd,&data->value,sizeof(data->value),0);
			int rec3 = recv(pollfd[i].fd,&data->ts,sizeof(data->ts),0);

			if((rec1 == 0)||(rec2 == 0)||(rec3 == 0))
			{
		   	  printf("client close\n");

		   	  asprintf(&msg,"sensor ID %d closed\n",data->id);
		   	  writeFIFO(msg);
		   	  free(msg);
		   	  close(pollfd[i].fd);
		   	  pollfd[i].fd=-1;

			}
			else
			{

			    sbuffer_insert(*buffer,data);

                printf("sensor id = %d, temperature = %lf, timestamp = %ld inserted to shared buffer \n", data->id, data->value, (long int)data->ts);

			 }
	          if(--ret<=0) break;	
                 
		 }
			
		 
		}

         
	
	   
   }	

    free(element1);
	element1=NULL;
	free(data);
	data=NULL;
	while((*buffer)->head!=NULL)
	{
       usleep(1000);
	}


	sbuffer_free(buffer);
	
	
  }     

void connmgr_free()
{
   dpl_free(&list,false);
   printf("data freed\n");
	}
