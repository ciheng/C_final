#define _GNU_SOURCE 
#include <stdio.h>
#include "sensor_db.h"
#include <sqlite3.h>
#include "config.h"
#include "sbuffer.h"
#include <time.h>
#include <unistd.h>
#include "fifo.h"
#include <pthread.h>
//typedef int (*callback_t)(void *, int, char **, char **);
  
  char *ErrMsg = 0;
  int rc;
  char *sql;
  char*msg;

/*
 * Reads continiously all data from the shared buffer data structure and stores this into the database
 * When *buffer becomes NULL the method finishes. This method will NOT automatically disconnect from the db
 */

typedef struct sbuffer sbuffer_t;




void storagemgr_parse_sensor_data(DBCONN * conn, sbuffer_t ** buffer)
{

   rc = sqlite3_open(TO_STRING(DB_NAME), &conn);
    sensor_data_t *data = malloc(sizeof(sensor_data_t));
    while(*buffer != NULL)
   {
    if(sbuffer_remove( *buffer, data)==SBUFFER_SUCCESS){
	rc=insert_sensor(conn,data->id,data->value,data->ts);
	printf("sensor_db read from buffer,ID  %d,value %lf,time %ld\n",data->id,data->value,data->ts);}



     if (rc != SQLITE_OK )
   {     
	fprintf(stderr, "Failed to insert sensor from file: %s\n", sqlite3_errmsg(conn));
	sqlite3_free(ErrMsg);
	sqlite3_close(conn);
        
    }
	sqlite3_free(ErrMsg);
	disconnect(conn);
}

}


/*
 * Make a connection to the database server
 * Create (open) a database with name DB_NAME having 1 table named TABLE_NAME  
 * If the table existed, clear up the existing data if clear_up_flag is set to 1
 * Return the connection for success, NULL if an error occurs
 */
DBCONN * init_connection(char clear_up_flag)
{
   int count=0;
   sqlite3 *db;
   rc = sqlite3_open(TO_STRING(DB_NAME), &db);

   while(rc!=SQLITE_OK){
       fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
       msg="Unable to connect to SQL server\n";//////////////
       writeFIFO(msg);
       count++;
       sleep(5);
       if(count==3)
       {
           sqlite3_close(db);
           return NULL;
       }

   
   }
   // printf("connected to database\n");
   msg="Connection to SQL server established\n";////////////
   writeFIFO(msg);

   sql="SELECT name FROM sqlite_master "\
        "WHERE type='table';";
   rc=sqlite3_exec(db,sql,NULL,NULL,&ErrMsg);
   if(rc==SQLITE_OK)
   {if(clear_up_flag==1)
   {
    asprintf(&sql, "DROP TABLE %s;",TO_STRING(TABLE_NAME));
	rc=sqlite3_exec(db,sql,NULL,NULL,&ErrMsg); 
	free(sql);
        sql=NULL;
	} }

	sql = "CREATE TABLE IF NOT EXISTS " TO_STRING(TABLE_NAME) "("  \
         "ID INTEGER PRIMARY KEY AUTOINCREMENT," \
         "sensor_id           INTEGER ," \
         "sensor_value          DECIMAL(4,2)," \
         "timestamp       TIMESTAMP);";

	 rc=sqlite3_exec(db,sql,NULL,NULL,&ErrMsg);
    msg="New table " TO_STRING(TABLE_NAME) " created\n";//////////
    //printf("created a new table");
    writeFIFO(msg);

   if(rc!=SQLITE_OK)
   {
      fprintf(stderr,"SQL error: %s\n",sqlite3_errmsg(db ));
      
      sqlite3_close(db);
      
    }


    sqlite3_free(ErrMsg);
    return db;
    }

/*
 * Disconnect from the database server
 */
void disconnect(DBCONN *conn)
{
	sqlite3_close(conn);
  }

/*
 * Write an INSERT query to insert a single sensor measurement
 * Return zero for success, and non-zero if an error occurs
 */
int insert_sensor(DBCONN * conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts)
{
   rc = sqlite3_open(TO_STRING(DB_NAME), &conn);
   printf("inserting data (%d,%lf,%ld) to database\n",id,value,ts);
   asprintf(&sql,"INSERT INTO %s VALUES (NULL,%d,%lf,%ld);" ,TO_STRING(TABLE_NAME),id,value,ts);
   rc=sqlite3_exec(conn,sql,NULL,NULL,&ErrMsg);	
   if (rc != SQLITE_OK )
   {    
	fprintf(stderr, "Failed to insert sensor: %s\n", sqlite3_errmsg(conn));
	sqlite3_free(ErrMsg);
    sqlite3_close(conn);
	
        return 1;
	
    }
	disconnect(conn);
	sqlite3_free(ErrMsg);
	free(sql);
	sql=NULL;
	return 0;	
}	

/*
 * Write an INSERT query to insert all sensor measurements available in the file 'sensor_data'
 * Return zero for success, and non-zero if an error occurs
 */
int insert_sensor_from_file(DBCONN * conn, FILE * sensor_data)
{
   
   rc = sqlite3_open(TO_STRING(DB_NAME), &conn);
   sensor_id_t id;
   sensor_value_t value;
   sensor_ts_t ts;
   while(fread(&id,sizeof(sensor_id_t),1,sensor_data)!=0)
   {
	fread(&value,sizeof(sensor_value_t),1,sensor_data);
	fread(&ts,sizeof(sensor_ts_t),1,sensor_data);
	rc=insert_sensor(conn,id,value,ts);
        }
     if (rc != SQLITE_OK )
   {     
	fprintf(stderr, "Failed to insert sensor from file: %s\n", sqlite3_errmsg(conn));
	sqlite3_free(ErrMsg);
        sqlite3_close(conn);
        return 1;
    } 
	sqlite3_free(ErrMsg);
	disconnect(conn);
	return 0;	
   }


/*
  * Write a SELECT query to select all sensor measurements in the table 
  * The callback function is applied to every row in the result
  * Return zero for success, and non-zero if an error occurs
  */
int find_sensor_all(DBCONN * conn, callback_t f)
{
  rc = sqlite3_open(TO_STRING(DB_NAME), &conn);
   sql="SELECT * FROM " TO_STRING(TABLE_NAME) ";";
   rc = sqlite3_exec(conn, sql, f, NULL, &ErrMsg);
   if (rc != SQLITE_OK )
   {      
      fprintf(stderr, "Failed to find sensor: %s\n", sqlite3_errmsg(conn));
      sqlite3_free(ErrMsg);
      sqlite3_close(conn);
        return 1;
    } 
	sqlite3_free(ErrMsg);
	disconnect(conn);
	return 0;	
   }

/*
 * Write a SELECT query to return all sensor measurements having a temperature of 'value'
 * The callback function is applied to every row in the result
 * Return zero for success, and non-zero if an error occurse
 */
int find_sensor_by_value(DBCONN * conn, sensor_value_t value, callback_t f)
{
   rc = sqlite3_open(TO_STRING(DB_NAME), &conn);
   asprintf(&sql, "SELECT * FROM %s WHERE sensor_value = %lf;" ,TO_STRING(TABLE_NAME),value);
    
   rc = sqlite3_exec(conn, sql, f, NULL, &ErrMsg);
   if (rc != SQLITE_OK )
   {        
	fprintf(stderr, "Failed to find sensor by value: %s\n", sqlite3_errmsg(conn));
	sqlite3_free(ErrMsg);
	sqlite3_close(conn);
	free(sql);
	sql=NULL;
        return 1;
    } 
	sqlite3_free(ErrMsg);
	free(sql);
	sql=NULL;
	return 0;	
   }


/*
 * Write a SELECT query to return all sensor measurements of which the temperature exceeds 'value'
 * The callback function is applied to every row in the result
 * Return zero for success, and non-zero if an error occurs
 */
int find_sensor_exceed_value(DBCONN * conn, sensor_value_t value, callback_t f)
{
  rc = sqlite3_open(TO_STRING(DB_NAME), &conn);
   asprintf(&sql, "SELECT * FROM %s WHERE sensor_value > %lf;" ,TO_STRING(TABLE_NAME),value);
   rc = sqlite3_exec(conn, sql, f, NULL, &ErrMsg);
   if (rc != SQLITE_OK )
   {    
	 fprintf(stderr, "Failed to find sensor exceed value: %s\n", sqlite3_errmsg(conn));
	 sqlite3_free(ErrMsg);
         sqlite3_close(conn);
	 free(sql); 
	 sql=NULL;
         return 1;
    } 
	disconnect(conn);
	sqlite3_free(ErrMsg);
	free(sql);
	sql=NULL;
	return 0;	
   }


/*
 * Write a SELECT query to return all sensor measurements having a timestamp 'ts'
 * The callback function is applied to every row in the result
 * Return zero for success, and non-zero if an error occurs
 */
int find_sensor_by_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f)
{
   rc = sqlite3_open(TO_STRING(DB_NAME), &conn);
   asprintf(&sql,"SELECT * FROM %s WHERE timestamp = %ld;",TO_STRING(TABLE_NAME),ts);
   rc = sqlite3_exec(conn, sql, f, NULL, &ErrMsg);
   if (rc != SQLITE_OK )
   {    
	fprintf(stderr, "Failed to find sensor by timestamp: %s\n", sqlite3_errmsg(conn));
	sqlite3_free(ErrMsg);
        sqlite3_close(conn);  
	free(sql);
	sql=NULL;  
        return 1;
    } 
	sqlite3_free(ErrMsg);
	disconnect(conn);
	free(sql);
	sql=NULL;
	return 0;	
   }


/*
 * Write a SELECT query to return all sensor measurements recorded after timestamp 'ts'
 * The callback function is applied to every row in the result
 * return zero for success, and non-zero if an error occurs
 */
int find_sensor_after_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f)
{
   rc = sqlite3_open(TO_STRING(DB_NAME), &conn);
   asprintf(&sql,"SELECT * FROM %s WHERE timestamp > %ld;",TO_STRING(TABLE_NAME),ts);
   rc = sqlite3_exec(conn, sql, f, NULL, &ErrMsg);
   if (rc != SQLITE_OK )
   {    
	fprintf(stderr, "Failed to find sensor after timestamp: %s\n", sqlite3_errmsg(conn));
	sqlite3_free(ErrMsg);
        sqlite3_close(conn);  
	free(sql); 
	sql=NULL;
        return 1;
    } 
	disconnect(conn);
	sqlite3_free(ErrMsg);
	free(sql);
	sql=NULL;
	return 0;	
   }


