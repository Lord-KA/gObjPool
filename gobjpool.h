#ifndef GOBJPOOL_H
#define GOBJPOOL_H

#include <stdio.h>
#include <malloc.h>

#include "gutils.h"

// typedef int GOBJPOOL_TYPE;  //TODO remove

static const size_t GOBJPOOL_START_CAPACITY = 2;
static const size_t GOBJPOOL_CAPACITY_EXPND_FACTOR = 2;

struct gObjPool_Node 
{
    size_t next;
    GOBJPOOL_TYPE val;
    bool allocated;
} typedef gObjPool_Node;

enum gObjPool_status 
{
    gObjPool_status_OK,
    gObjPool_status_AllocErr,
    gObjPool_status_BadCapacity,
    gObjPool_status_BadStructPtr,
    gObjPool_status_BadId,
};

struct gObjPool 
{
    gObjPool_Node *data;
    size_t capacity;
    size_t last_free;
    gObjPool_status status;
    FILE *logStream;
} typedef gObjPool;


gObjPool_status gObjPool_ctor(gObjPool *pool, size_t newCapacity, FILE *newLogStream) 
{
    if (!gPtrValid(pool)) {                                          
        FILE *out;                                                   
        if (!gPtrValid(newLogStream))                                
            out = stderr;                                            
        else                                                         
            out = newLogStream;                                      
        fprintf(out, "ERROR: bad structure ptr provided to ctor!\n");
        return gObjPool_status_BadStructPtr;                         
    }                                                                

    if (newCapacity == -1 || newCapacity == 0)
        pool->capacity = GOBJPOOL_START_CAPACITY;
    else 
        pool->capacity = newCapacity;

    if (!gPtrValid(newLogStream))
        pool->logStream = stderr;
    else 
        pool->logStream = newLogStream;
 
    pool->data = (gObjPool_Node*)calloc(pool->capacity, sizeof(gObjPool_Node));
    if (pool->data == NULL) {
        fprintf(pool->logStream, "ERROR: Failed to allocate memory!\n");
        pool->capacity  = -1;
        pool->last_free = -1;
        return gObjPool_status_AllocErr;
    }
    
    for (size_t i = 0; i < pool->capacity - 1; ++i) {
        pool->data[i].next = i + 1;
        pool->data[i].allocated = false;
    }
    pool->data[pool->capacity - 1].next = -1;
    pool->data[pool->capacity - 1].allocated = false;

    pool->last_free = 0;
    return gObjPool_status_OK;
}

gObjPool_status gObjPool_dtor(gObjPool *pool)
{
    if (!gPtrValid(pool)) {                                          
        fprintf(stderr, "ERROR: bad structure ptr provided to dtor!\n");
        return gObjPool_status_BadStructPtr;                         
    }                                                                

    free(pool->data);
    pool->data       = NULL;
    pool->capacity   = -1;
    pool-> last_free = -1;
    return gObjPool_status_OK;
}

gObjPool_status gObjPool_refit(gObjPool *pool)
{
    if (!gPtrValid(pool)) {                                          
        fprintf(stderr, "ERROR: bad structure ptr provided to refit!\n");
        return gObjPool_status_BadStructPtr;                         
    }                                                              
    if (pool->last_free != -1)
        return gObjPool_status_OK;

    if (pool->capacity == -1) {
        fprintf(pool->logStream, "ERROR: bad capacity (was dtor used?)\n");
        return gObjPool_status_BadCapacity;
    }

    
    size_t newCapacity = pool->capacity * GOBJPOOL_CAPACITY_EXPND_FACTOR;
    gObjPool_Node *newData = (gObjPool_Node*)realloc(pool->data, newCapacity * sizeof(gObjPool_Node));

    if (newData == NULL) {
        fprintf(pool->logStream, "ERROR: Failed to reallocate memory!\n");
        return gObjPool_status_AllocErr;
    }
    pool->data = newData;
    pool->capacity = newCapacity;
    for (size_t i = pool->capacity / 2; i < pool->capacity - 1; ++i) {
        pool->data[i].next = i + 1;
        pool->data[i].allocated = false;
    }
    pool->data[pool->capacity - 1].next = -1;
    pool->data[pool->capacity - 1].allocated = false;
    pool->last_free = pool->capacity / 2;
   
    return gObjPool_status_OK;
}

gObjPool_status gObjPool_alloc(gObjPool *pool, size_t *result_id)
{
    if (!gPtrValid(pool)) {                                          
        fprintf(stderr, "ERROR: bad structure ptr provided to alloc!\n");
        return gObjPool_status_BadStructPtr;                         
    }                                                                

    pool->status = gObjPool_refit(pool);
    if (pool->status != gObjPool_status_OK)
        return pool->status;

    pool->data[pool->last_free].allocated = true;

    *result_id = pool->last_free;
    pool->last_free = pool->data[pool->last_free].next;


    return gObjPool_status_OK;
}

gObjPool_status gObjPool_free(gObjPool *pool, size_t id)
{
    if (!gPtrValid(pool)) {                                          
        fprintf(stderr, "ERROR: bad structure ptr provided to free!\n");
        return gObjPool_status_BadStructPtr;                         
    }                                

    if (id > pool->capacity || !(pool->data[id].allocated))
        return gObjPool_status_BadId;

    pool->data[id].next = pool->last_free;
    pool->last_free = id;

    pool->data[pool->last_free].allocated = false;
    
    return gObjPool_status_OK;
}

gObjPool_status gObjPool_get(const gObjPool *pool, const size_t id, GOBJPOOL_TYPE **returnPtr)
{
    if (!gPtrValid(pool)) {                                          
        fprintf(stderr, "ERROR: bad structure ptr provided to get!\n");
        return gObjPool_status_BadStructPtr;                         
    }                                                                
    
    if (id > pool->capacity) {
        fprintf(pool->logStream, "ERROR: bad id provided to get!\n");
        return  gObjPool_status_BadId;
    }

    *returnPtr = &pool->data[id].val;
   
    return gObjPool_status_OK;
}

gObjPool_status gObjPool_dumpFree(gObjPool *pool, FILE *newLogStream)
{
    FILE *out;                                                   
    if (gPtrValid(newLogStream))
        out = newLogStream;
    else
        out = stderr;
    if (!gPtrValid(pool)) {                                          
        fprintf(out, "ERROR: bad structure ptr provided to ctor!\n");
        return gObjPool_status_BadStructPtr;                         
    }   
    if (gPtrValid(pool->logStream) && newLogStream == NULL)
        out = pool->logStream;

    fprintf(out, "Object Pool dump:\ncapacity = %lu\nlast_free = %lu\ndata:\n", pool->capacity, pool->last_free);
    if (pool->last_free == -1)
        fprintf(out, "Empty\n");
    else {
        for (size_t id = pool->last_free; id != -1; id = pool->data[id].next)
            fprintf(out, "(%li)->", id);
        fprintf(out, "\n");
    }

    return gObjPool_status_OK;
}


#endif /* GOBJPOOL_H */
