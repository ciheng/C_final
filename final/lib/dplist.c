#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"

/*
 * definition of error codes
 * */
#define DPLIST_NO_ERROR 0
#define DPLIST_MEMORY_ERROR 1 // error due to mem alloc failure
#define DPLIST_INVALID_ERROR 2 //error due to a list operation applied on a NULL list 

#ifdef DEBUG
	#define DEBUG_PRINTF(...) 									         \
		do {											         \
			fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	 \
			fprintf(stderr,__VA_ARGS__);								 \
			fflush(stderr);                                                                          \
                } while(0)
#else
	#define DEBUG_PRINTF(...) (void)0
#endif


#define DPLIST_ERR_HANDLER(condition,err_code)\
	do {						            \
            if ((condition)) DEBUG_PRINTF(#condition " failed\n");    \
            assert(!(condition));                                    \
        } while(0)

        
/*
 * The real definition of struct list / struct node
 */

struct dplist_node {
  dplist_node_t * prev, * next;                              
  void * element;
};

struct dplist {
  dplist_node_t * head;
  void * (*element_copy)(void * src_element);			  
  void (*element_free)(void ** element);
  int (*element_compare)(void * x, void * y);
};


dplist_t * dpl_create (// callback functions
			  void * (*element_copy)(void * src_element),
			  void (*element_free)(void ** element),
			  int (*element_compare)(void * x, void * y)
			  )
{
  dplist_t * list;
  list = malloc(sizeof(struct dplist));
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_MEMORY_ERROR);
  list->head = NULL;  
  list->element_copy = element_copy;
  list->element_free = element_free;
  list->element_compare = element_compare; 
  return list;
}

void dpl_free(dplist_t ** list, bool free_element)
{
  // add your code here
  while((*list)->head!=NULL)
 {
  dpl_remove_at_index(*list,0,free_element);
 }
 free(*list);
 *list=NULL;
}

dplist_t * dpl_insert_at_index(dplist_t * list, void * element, int index, bool insert_copy)
{
   dplist_node_t * ref_at_index, * list_node;
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  list_node = malloc(sizeof(dplist_node_t));
  DPLIST_ERR_HANDLER(list_node==NULL,DPLIST_MEMORY_ERROR);
  if(insert_copy==true)                                   //for insert_copy,if true, copy
  {list_node->element=(*(list->element_copy))(element);}
  else{list_node->element=element;}
  // pointer drawing breakpoint
  if (list->head == NULL)  
  { // covers case 1 
    list_node->prev = NULL;
    list_node->next = NULL;
    list->head = list_node;
    // pointer drawing breakpoint
  } else if (index <= 0)  
  { // covers case 2 
    list_node->prev = NULL;
    list_node->next = list->head;
    list->head->prev = list_node;
    list->head = list_node;
    // pointer drawing breakpoint
  } else 
  {
    ref_at_index = dpl_get_reference_at_index(list, index);  
    assert( ref_at_index != NULL);
    // pointer drawing breakpoint
    if (index < dpl_size(list))
    { // covers case 4
      list_node->prev = ref_at_index->prev;
      list_node->next = ref_at_index;
      ref_at_index->prev->next = list_node;
      ref_at_index->prev = list_node;
      // pointer drawing breakpoint
    } else
    { // covers case 3 
      assert(ref_at_index->next == NULL);
      list_node->next = NULL;
      list_node->prev = ref_at_index;
      ref_at_index->next = list_node;    
      // pointer drawing breakpoint
    }
  }
  return list;
}

dplist_t * dpl_remove_at_index( dplist_t * list, int index, bool free_element)
{
    // add your code here
    dplist_node_t *ref;                          //when ref is head or at middle of list or at the end of list
   if(list->head==NULL) return list;
   if(index<0) index=0;
   if(index>=dpl_size(list)) index=dpl_size(list)-1;
   ref=dpl_get_reference_at_index(list,index);
   if(ref==list->head&&list->head->next!=NULL)
   {
    list->head=list->head->next;
    list->head->prev=NULL; 
   }else if(ref==list->head&&list->head->next==NULL){list->head=NULL;}
   if(ref->prev!=NULL&&ref->next!=NULL)
   {ref->next->prev = ref->prev;
    ref->prev->next = ref->next;}
   if(ref->prev!=NULL&&ref->next==NULL){
    ref->prev->next = NULL;}
   if(free_element==true)                        //for free_element, if true,free the node and memory
  {                                             //if false,only free node without free the memory
    (*(list->element_free))(&(ref->element));
    }
  free(ref);
  return list;
}

int dpl_size( dplist_t * list )
{
  // add your code here
  dplist_node_t *ptr;
  int count=1;
  if(list->head!=NULL)
 { ptr=list->head;   }else{return 0;}
  while(ptr->next!=NULL)
 { count++;
   ptr=ptr->next;
      } 
  
  return count;
}

dplist_node_t * dpl_get_reference_at_index( dplist_t * list, int index )
{
   int count;
  dplist_node_t * dummy;
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  if (list->head == NULL ) return NULL;
  for ( dummy = list->head, count = 0; dummy->next != NULL ; dummy = dummy->next, count++) 
  { 
    if (count >= index) return dummy;
  }  
  return dummy;
}

void * dpl_get_element_at_index( dplist_t * list, int index )
{
   // add your code here
   DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
   if(index<0) index=0;
   if(index>=dpl_size(list)) index=dpl_size(list)-1;
   //dplist_node_t *ptr;
   //ptr=dpl_get_reference_at_index( list,index );
   return(dpl_get_reference_at_index( list,index )->element);
}

int dpl_get_index_of_element( dplist_t * list, void * element )
{
     // add your code here
     if(list->head==NULL) return -1;
     int index=0;
     dplist_node_t * dummy=list->head;
    while(dummy!=NULL)
     {
	if((*(list->element_compare))(element,dummy->element)==0)
        {return index;} 
 	dummy=dummy->next;
	index++;       
     }return -1;
}


// HERE STARTS THE EXTRA SET OF OPERATORS //

// ---- list navigation operators ----//
  
dplist_node_t * dpl_get_first_reference( dplist_t * list )
{
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    return list->head;
}

dplist_node_t * dpl_get_last_reference( dplist_t * list )
{
    if(list->head==NULL) return NULL;
    int size=dpl_size(list);
    return dpl_get_reference_at_index(list,size-1);
}

dplist_node_t * dpl_get_next_reference( dplist_t * list, dplist_node_t * reference )
{
    if(list->head==NULL) return NULL;
    if(reference==NULL) return NULL;
    if(reference->next==NULL) return NULL;
    dplist_node_t * dummy=list->head;
    while(dummy!=NULL)
    {
	if(dummy==reference)
         {
		return dummy->next;
                  }
        dummy=dummy->next;

      }return NULL;
}

dplist_node_t * dpl_get_previous_reference( dplist_t * list, dplist_node_t * reference )
{
    if(list->head==NULL) return NULL;
    if(reference==NULL) return dpl_get_last_reference( list );
    if(reference->prev==NULL) return NULL;
    dplist_node_t * dummy=list->head;
    while(dummy!=NULL)
    {
	if(dummy==reference)
         {
		return dummy->prev;
                  }
        dummy=dummy->next;

      }return NULL;
}

// ---- search & find operators ----//  
  
void * dpl_get_element_at_reference( dplist_t * list, dplist_node_t * reference )
{
    if(list->head==NULL) return NULL;
    if(reference==NULL) return (dpl_get_last_reference( list )->element);
    dplist_node_t * dummy=list->head;
    while(dummy!=NULL)
    {
	if(dummy==reference)
         {
		return (dummy->element);
                  }
        dummy=dummy->next;
      }return NULL;

}

dplist_node_t * dpl_get_reference_of_element( dplist_t * list, void * element )
{
    if(list->head==NULL) return NULL;
    int index=dpl_get_index_of_element(list,element);
    if(index!=-1)
    return  dpl_get_reference_at_index(list,index);
    return NULL;
}

int dpl_get_index_of_reference( dplist_t * list, dplist_node_t * reference )
{
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
   void * element=dpl_get_element_at_reference(list,reference);
   if(element!=NULL) return dpl_get_index_of_element(list,element);
   return -1;
    
}
  
// ---- extra insert & remove operators ----//

dplist_t * dpl_insert_at_reference( dplist_t * list, void * element, dplist_node_t * reference, bool insert_copy )
{
  if(list->head==NULL) return NULL;
  if(reference==NULL) 
  return dpl_insert_at_index(list, element,dpl_size(list ) ,insert_copy);
  if(element==NULL) return list;
  int index = dpl_get_index_of_reference(list, reference);
  if(index==-1) return list;
  return  dpl_insert_at_index(list, element,index , insert_copy);
}

dplist_t * dpl_insert_sorted( dplist_t * list, void * element, bool insert_copy )
{
    if(list->head==NULL) return dpl_insert_at_index(list,element,0,insert_copy);
    dplist_node_t * dummy=list->head;
    int i=0;
    while(dummy!=NULL)
    {
	int compare=((*(list->element_compare)) (dummy->element, element ));
 	if(compare!=1)
	{dummy=dummy->next;
	  i++;	}
        else break;
        }return dpl_insert_at_index(list,element,i,insert_copy);
    
}

dplist_t * dpl_remove_at_reference( dplist_t * list, dplist_node_t * reference, bool free_element )
{
     if(list->head==NULL) return list;
     if(reference==NULL) return dpl_remove_at_index(list,  dpl_size(list)-1, free_element);
     int index=dpl_get_index_of_reference(list,reference);
     if(index!=-1) return dpl_remove_at_index(list,index,free_element);
     return list;
}

dplist_t * dpl_remove_element( dplist_t * list, void * element, bool free_element )
{
    if(list->head==NULL) return NULL; 
    int index=dpl_get_index_of_element(list,element);
    if(index!=-1) return dpl_remove_at_index(list,index,free_element);
    return list;
}
  
// ---- you can add your extra operators here ----//



