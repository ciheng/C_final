#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include "lib/dplist.h"
#include <inttypes.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "datamgr.h"
#include "config.h"
#include "sbuffer.h"
#include "fifo.h"
dplist_t * List=NULL;

typedef uint16_t room_id_t;

typedef struct {
  sensor_id_t sensor_id;
  room_id_t room_id;
  sensor_value_t average;
  int NrofReadings;
  sensor_value_t last_readings[RUN_AVG_LENGTH]; 
  time_t time;
}sensor_element_t;


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


static sensor_element_t* get_element_at_id(sensor_id_t sensor_id);

  void *element_copy(void * src_element);			  
  void element_free(void ** element);
  int element_compare(void * x, void * y);

double SET_MAX_TEMP=25.0; ////////////
double SET_MIN_TEMP=10.0;

char*msg;
sensor_element_t* get_element_at_id(sensor_id_t sensorId)
{
   int index=0;

   for(int i=0;i<sizeof(List);i++)
   {
   if((((sensor_element_t *)dpl_get_element_at_index(List,i))->sensor_id)==sensorId)
      {index=i;}
             }
   if(index<0) return NULL;
   return (sensor_element_t *)dpl_get_element_at_index(List,index);
}


/*
 *  This method holds the core functionality of your datamgr. It takes in 2 file pointers to the sensor files and parses them. 
 *  When the method finishes all data should be in the internal pointer list and all log messages should be printed to stderr.
 */

void datamgr_parse_sensor_files(FILE * fp_sensor_map, FILE * fp_sensor_data)
{
   sensor_element_t* element;
   sensor_element_t* dummy;
   sensor_element_t* element2;
   element=malloc(sizeof(sensor_element_t));
   element2=malloc(sizeof(sensor_element_t));
   element->NrofReadings=0;
   List=dpl_create(&element_copy,&element_free,&element_compare);
   uint16_t roomID;
   uint16_t sensorID;
   while(fscanf(fp_sensor_map,"%"SCNd16 "%"SCNd16 ,&roomID,&sensorID) != EOF) //decimal scanf format for unit16_t
	{
		element->room_id=roomID;                                  //fscanf for format,fread for length
		element->sensor_id=sensorID;	
        dpl_insert_sorted(List,element,true);
	 	
		
	}
   
  
   while(feof(fp_sensor_data)!=1)
   {
      fread(&(element2->sensor_id),sizeof(sensor_id_t),1,fp_sensor_data); //store address,unit size,Nr of units,file 
      fread(&(element2->average),sizeof(sensor_value_t),1,fp_sensor_data);
      fread(&(element2->time),sizeof(sensor_ts_t),1,fp_sensor_data);
      dummy=get_element_at_id(element2->sensor_id);
      if(dummy->sensor_id==element2->sensor_id)
         {
		dummy->time=element2->time;
		dummy->last_readings[(dummy->NrofReadings)%RUN_AVG_LENGTH]=element2->average;
		dummy->NrofReadings ++;
		
		
                 
		if(dummy->NrofReadings >= RUN_AVG_LENGTH)
	       {
		dummy->average=0;
		double total=0.0;
		for(int j=0;j<RUN_AVG_LENGTH;j++)
   		{ 
		    total=total+dummy->last_readings[j];
			}
		dummy->average=total/RUN_AVG_LENGTH;


		if(dummy->NrofReadings >= RUN_AVG_LENGTH)
	       {
		dummy->average=0;
		for(int j=0;j<RUN_AVG_LENGTH;j++)
   		{ 
		  dummy->average+=dummy->last_readings[j];
			}
		dummy->average/=RUN_AVG_LENGTH;


		
		
		
         if((dummy->average)>SET_MAX_TEMP)
		{fprintf(stderr,"room:""%"PRIu16" too hot\n",dummy->room_id);}//decimal printf format for unit16_t
		if((dummy->average)<SET_MIN_TEMP)
		{fprintf(stderr,"room:""%"PRIu16" too cold\n",dummy->room_id);}
		
               }
				
		
 	      }
       
 	}
  
   }
free(element);
element=NULL;
free(element2);
element2=NULL;
}

/*
 * Reads continiously all data from the shared buffer data structure, parse the room_id's
 * and calculate the running avarage for all sensor ids
 * When *buffer becomes NULL the method finishes. This method will NOT automatically free all used memory
 */
void datamgr_parse_sensor_data(FILE * fp_sensor_map, sbuffer_t ** buffer)
{

   sensor_element_t* element;
   sensor_element_t* dummy;
   sensor_data_t* element2;
   element=malloc(sizeof(sensor_element_t));
   element2=malloc(sizeof(sensor_data_t));
   element->NrofReadings=0;
   List=dpl_create(&element_copy,&element_free,&element_compare);
   uint16_t roomID;
   uint16_t sensorID;
   while(fscanf(fp_sensor_map,"%"SCNd16 "%"SCNd16 ,&roomID,&sensorID) != EOF) //decimal scanf format for unit16_t
	{
		element->room_id=roomID;                                  //fscanf for format,fread for length
		element->sensor_id=sensorID;	
		dpl_insert_sorted(List,element,true);
	 	
		
	}



   while(*buffer != NULL)
   {
       if( sbuffer_remove(*buffer,element2)==SBUFFER_SUCCESS){
       printf("datamgr read from buffer,id %d,value %lf,time %ld\n",element2->id,element2->value,element2->ts);}


       dummy=get_element_at_id(element2->id);
       if(dummy->sensor_id==element2->id)
        {
		dummy->time=element2->ts;
		dummy->last_readings[(dummy->NrofReadings)%RUN_AVG_LENGTH]=element2->value;
		dummy->NrofReadings ++;
		
		
                 
		if(dummy->NrofReadings >= RUN_AVG_LENGTH)
	    {
		dummy->average=0;
		double total=0.0;
		for(int j=0;j<RUN_AVG_LENGTH;j++)
   		{ 
		    total=total+dummy->last_readings[j];
			}
		dummy->average=total/RUN_AVG_LENGTH;


		if(dummy->NrofReadings >= RUN_AVG_LENGTH)
	       {
		dummy->average=0;
		for(int j=0;j<RUN_AVG_LENGTH;j++)
   		{ 
		  dummy->average+=dummy->last_readings[j];
			}
		dummy->average/=RUN_AVG_LENGTH;


		
		
		
        if((dummy->average)>SET_MAX_TEMP)
		{
		fprintf(stderr,"room:""%"PRIu16" too hot\n",dummy->room_id);//decimal printf format for unit16_t
		asprintf(&msg,"room%d is too hot\n",dummy->room_id);
		writeFIFO(msg);
        free(msg);
		}
		if((dummy->average)<SET_MIN_TEMP)
		{fprintf(stderr,"room:""%"PRIu16" too cold\n",dummy->room_id);}
		 asprintf(&msg,"room%d is too cold\n",dummy->room_id);/////////////
         writeFIFO(msg);
         free(msg);
               }
				
		
 	      }
            free(element);
            element=NULL;
            free(element2);
            element2=NULL;

        }

   }







}





void datamgr_free()
{
   dpl_free(&List,true);

   }
    

uint16_t datamgr_get_room_id(sensor_id_t sensor_id)
{
    sensor_element_t* element;
    element=get_element_at_id(sensor_id);
    if(element==NULL) return -1;
    return element->room_id;
  }


sensor_value_t datamgr_get_avg(sensor_id_t sensor_id)
{
    sensor_element_t* element;
    element=get_element_at_id(sensor_id);
    if(element==NULL) return 0;
    return (element->average);
	}


time_t datamgr_get_last_modified(sensor_id_t sensor_id)
{
    sensor_element_t* element;
    element=get_element_at_id(sensor_id);
    if(element==NULL) return 0;
    return (element->time);
	}


int datamgr_get_total_sensors()
{
  if(List==NULL) return 0;
  return dpl_size(List);
  }



