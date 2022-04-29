#ifndef GOBJPOOL_H
#define GOBJPOOL_H

/**
 * @file Header with generalized Object Pool structure
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "gutils.h"

#ifndef GOBJPOOL_BLK_ALLOC
#define GOBJPOOL_BLK_ALLOC 2
#endif

static const size_t PAGE_SIZE = 4096;                 /// == sysconf(_SC_PAGESIZE)
static const size_t GOBJPOOL_MAX_MSG_LEN = 64;        /// max error message lenght
static const size_t GOBJPOOL_MAX_BLK_CNT = 1<<13;     /// max count of memory pages

/**
 * @brief basic objPool node struct with user provided data
 */
struct gObjPool_Node
{
    GOBJPOOL_TYPE val;          /// user-provided data
    size_t next;                /// id of the next non-allocated node in the pool
    bool allocated;             /// flag if the node is allocated
} typedef gObjPool_Node;

static const size_t GOBJPOOL_PAGE_CAP = PAGE_SIZE / sizeof(gObjPool_Node);

/**
 * @brief status codes for gObjPool
 */
enum gObjPool_status
{
    gObjPool_status_OK,
    gObjPool_status_AllocErr,
    gObjPool_status_BadCapacity,
    gObjPool_status_BadStructPtr,
    gObjPool_status_BadId,
    gObjPool_status_Cnt,
};


/**
 * @brief status codes explanations
 */
const char gObjPool_statusMsg[gObjPool_status_Cnt][GOBJPOOL_MAX_MSG_LEN] = {
    "OK",
    "Allocator error",
    "Error: bad capacity detected",
    "Error: bad self ptr provided",
    "Error: bad Id provided",
};


/**
 * @brief local version of ASSERT_LOG macro
 */
#ifndef NLOGS
#define GOBJPOOL_ASSERT_LOG(expr, errCode) ASSERT_LOG((expr), (errCode), gObjPool_statusMsg[errCode], pool->logStream)
#else
#define GOBJPOOL_ASSERT_LOG(expr, errCode) ASSERT_LOG((expr), (errCode), gObjPool_statusMsg[errCode], NULL)
#endif


/**
 * @brief macro for handy id check
 */
#define GOBJPOOL_ID_VAL(id) GOBJPOOL_ASSERT_LOG(gObjPool_idValid(pool, id), gObjPool_status_BadId)

#define GET_NODE_UNSAFE(macroId) (pool->pages[(macroId) / GOBJPOOL_PAGE_CAP] + ((macroId) % GOBJPOOL_PAGE_CAP))

#define GOBJPOOL_GET_NODE_UNSAFE(macroPool, macroId) ((macroPool)->pages[(macroId) / GOBJPOOL_PAGE_CAP] + ((macroId) % GOBJPOOL_PAGE_CAP))

#define GOBJPOOL_VAL_BY_ID_UNSAFE(macroPool, macroId) &((macroPool)->pages[(macroId) / GOBJPOOL_PAGE_CAP][(macroId) % GOBJPOOL_PAGE_CAP].val)

#define GOBJPOOL_CHECK_ALLOC(expr) ({  \
    void *ptr = (void*)(expr);          \
    if (ptr == NULL) {                   \
        pool->capacity  = -1;             \
        pool->last_free = -1;              \
        for (size_t i = 0; i < pool->allocated_pages; ++i)  \
            free(pool->pages[i]);\
        free(pool->pages);\
        pool->pages = NULL;\
        pool->allocated_pages = 0;              \
        GOBJPOOL_ASSERT_LOG(false, gObjPool_status_AllocErr);   \
    }                                                            \
})


/**
 * @brief the main objPool structure
 */
struct gObjPool
{
    gObjPool_Node *data;
    size_t capacity;
    size_t last_free;
    gObjPool_status status;
    FILE *logStream;
    gObjPool_Node **pages;
    size_t allocated_pages;
} typedef gObjPool;


/**
 * @brief id check mainly for external modules
 * @param pool pointer to structure to construc onto
 * @param id id to check
 * @return true if id is valid, false otherwise
 */
static bool gObjPool_idValid(const gObjPool *pool, const size_t id)
{
    assert(gPtrValid(pool));
    if (id >= pool->capacity)
        return false;
    if (!GET_NODE_UNSAFE(id)->allocated)
        return false;
    return true;
}

static inline GOBJPOOL_TYPE* gObjPool_ValById_UNSAFE(const gObjPool *pool, size_t id)
{
    return &(pool->pages[id / GOBJPOOL_PAGE_CAP][id % GOBJPOOL_PAGE_CAP].val);
}

/**
 * @brief gObjPool constructor
 * @param pool pointer to structure to construc onto
 * @param newCapacity starting capacity, could be -1, than START_CAPACITY is used
 * @prarm newLogStream stream for logs, if NULL, than `stderr`
 * @return gObjPool status code
 */
static gObjPool_status gObjPool_ctor(gObjPool *pool, size_t newCapacity, FILE *newLogStream)
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
    assert(PAGE_SIZE == sysconf(_SC_PAGESIZE));

    if (newCapacity == -1 || newCapacity == 0)
        pool->capacity = GOBJPOOL_PAGE_CAP;
    else
        pool->capacity = ((newCapacity + GOBJPOOL_PAGE_CAP - 1) / GOBJPOOL_PAGE_CAP) * GOBJPOOL_PAGE_CAP;
    assert(pool->capacity % GOBJPOOL_PAGE_CAP == 0);

    if (!gPtrValid(newLogStream))
        pool->logStream = stderr;
    else
        pool->logStream = newLogStream;

    pool->pages = (gObjPool_Node**)calloc(GOBJPOOL_MAX_BLK_CNT, sizeof(gObjPool_Node*));
    GOBJPOOL_CHECK_ALLOC(pool->pages);
    pool->allocated_pages = pool->capacity / GOBJPOOL_PAGE_CAP;
    for (size_t i = 0; i < pool->allocated_pages; ++i)
        GOBJPOOL_CHECK_ALLOC(pool->pages[i] = (gObjPool_Node*)aligned_alloc(PAGE_SIZE, PAGE_SIZE));

    gObjPool_Node *node = ({ size_t tmp = 0; (pool->pages[tmp / GOBJPOOL_PAGE_CAP] + (tmp % GOBJPOOL_PAGE_CAP)); });
    for (size_t i = 0; i < pool->capacity - 1; ++i) {
        node->next = i + 1;
        node->allocated = false;
        node = ({ size_t tmp = i + 1; (pool->pages[tmp / GOBJPOOL_PAGE_CAP] + (tmp % GOBJPOOL_PAGE_CAP)); });
    }
    node->next = -1;
    node->allocated = false;

    pool->last_free = 0;
    return gObjPool_status_OK;
}

/**
 * @brief gObjPool destructor
 * @param pool pointer to structure to destruct
 * @return gObjPool status code
 */
static gObjPool_status gObjPool_dtor(gObjPool *pool)
{
    ASSERT_LOG(gPtrValid(pool), gObjPool_status_BadStructPtr, "ERROR: bad structure ptr provided to dtor!\n", stderr);

    for (size_t i = 0; i < pool->allocated_pages; ++i)
        free(pool->pages[i]);

    free(pool->pages);
    pool->pages = NULL;
    pool->capacity = -1;
    pool-> last_free = -1;
    return gObjPool_status_OK;
}


/**
 * @brief expands pool if no free elements
 * @param pool pointer to gObjPool structure
 * @return gObjPool status code
 */
static gObjPool_status gObjPool_refit(gObjPool *pool)
{
    ASSERT_LOG(gPtrValid(pool), gObjPool_status_BadStructPtr, "ERROR: bad structure ptr provided to refit!\n", stderr);
    if (pool->last_free != -1)
        return gObjPool_status_OK;

    ASSERT_LOG((pool->capacity != -1), (gObjPool_status_BadCapacity), gObjPool_statusMsg[gObjPool_status_BadCapacity], pool->logStream);

    for (size_t i = 0; i < GOBJPOOL_BLK_ALLOC; ++i)
        GOBJPOOL_CHECK_ALLOC(pool->pages[pool->allocated_pages + i] = (gObjPool_Node*)aligned_alloc(PAGE_SIZE, PAGE_SIZE));

    pool->allocated_pages += GOBJPOOL_BLK_ALLOC;

    gObjPool_Node *node = GET_NODE_UNSAFE(pool->capacity);
    for (size_t i = pool->capacity; i < pool->capacity + GOBJPOOL_BLK_ALLOC * GOBJPOOL_PAGE_CAP - 1; ++i) {
        node->next = i + 1;
        node->allocated = false;
        node = GET_NODE_UNSAFE(i + 1);
    }
    node->next = -1;
    node->allocated = false;
    pool->last_free = pool->capacity;
    pool->capacity += GOBJPOOL_BLK_ALLOC * GOBJPOOL_PAGE_CAP;

    return gObjPool_status_OK;
}


/**
 * @brief allocate element from the pool
 * @param pool pointer to gObjPool structure
 * @param result_id pointer to write id of the allocated node to
 * @return gObjPool status code
 */
static gObjPool_status gObjPool_alloc(gObjPool *pool, size_t *result_id)
{
    ASSERT_LOG(gPtrValid(pool), gObjPool_status_BadStructPtr, "ERROR: bad structure ptr provided to alloc!\n", stderr);
    pool->status = gObjPool_refit(pool);
    if (pool->status != gObjPool_status_OK)
        return pool->status;

    gObjPool_Node *node = GET_NODE_UNSAFE(pool->last_free);
    node->allocated = true;

    *result_id = pool->last_free;
    pool->last_free = node->next;

    node->val = {};

    return gObjPool_status_OK;
}


/**
 * @brief free element to the pool
 * @param pool pointer to gObjPool structure
 * @param id id of a node to free
 * @return gObjPool status code
 */
static gObjPool_status gObjPool_free(gObjPool *pool, size_t id)
{
    ASSERT_LOG(gPtrValid(pool), gObjPool_status_BadStructPtr, "ERROR: bad structure ptr provided to free!\n", stderr);
    ASSERT_LOG((gObjPool_idValid(pool, id)), (gObjPool_status_BadId), gObjPool_statusMsg[gObjPool_status_BadId], pool->logStream);

    gObjPool_Node *node = GET_NODE_UNSAFE(id);
    node->next = pool->last_free;
    node->allocated = false;
    pool->last_free = id;

    return gObjPool_status_OK;
}

/**
 * @brief get element from the pool
 * @param pool pointer to gObjPool structure
 * @param id id of the desired node
 * @param returnPtr pointer to write write pointer to the data
 * @return gObjPool status code
 */
static gObjPool_status gObjPool_get(const gObjPool *pool, const size_t id, GOBJPOOL_TYPE **returnPtr)
{
    ASSERT_LOG(gPtrValid(pool), gObjPool_status_BadStructPtr, "ERROR: bad structure ptr provided to get!\n", stderr);
    ASSERT_LOG((gObjPool_idValid(pool, id)), (gObjPool_status_BadId), gObjPool_statusMsg[gObjPool_status_BadId], pool->logStream);

    *returnPtr = gObjPool_ValById_UNSAFE(pool, id);

    return gObjPool_status_OK;
}

/**
 * @brief gets id of a node by pointer to data
 * @param pool pointer to gObjPool structure
 * @param ptr pointer to data of the desired node
 * @param id pointer to write the id to
 * @return gObjPool status code
 */
static gObjPool_status gObjPool_getId(const gObjPool *pool, GOBJPOOL_TYPE *ptr, size_t *id)
{
    ASSERT_LOG(gPtrValid(pool), gObjPool_status_BadStructPtr, "ERROR: bad structure ptr provided to getId!\n", stderr);
    ASSERT_LOG(gPtrValid(ptr), gObjPool_status_BadStructPtr, "ERROR: bad data ptr provided to getId!\n", stderr);
    ASSERT_LOG(gPtrValid(id), gObjPool_status_BadStructPtr, "ERROR: bad id ptr provided to getId!\n", stderr);

    gObjPool_Node *node = (gObjPool_Node*)ptr;
    size_t page = -1;
    for (size_t i = 0; i < pool->allocated_pages; ++i) {
        if (((node - pool->pages[i]) >= 0) && ((page == -1) || (pool->pages[i] - pool->pages[page] >= 0)))
                page = i;
    }
    if (page != -1)         //TODO for whatever reason while node >= pool->pages[page], node - pool->pages[page] is too big
        *id = page * GOBJPOOL_PAGE_CAP + ((size_t)node - (size_t)pool->pages[page]) / sizeof(gObjPool_Node);
    else
        *id = -1;

    return gObjPool_status_OK;
}

/**
 * @param dumps in line all free nodes
 * @param pool pointer to gObjPool structure
 * @param newLogStream stream to dump to, if `newLogStream == NULL` writes to `pool.logStream`
 * @return gObjPool status code
 */
static gObjPool_status gObjPool_dumpFree(gObjPool *pool, FILE *newLogStream)
{
    ASSERT_LOG(gPtrValid(pool), gObjPool_status_BadStructPtr, "ERROR: bad structure ptr provided to dump!\n", stderr);
    FILE *out = stderr;
    if (gPtrValid(newLogStream))
        out = newLogStream;
    if (gPtrValid(pool->logStream) && newLogStream == NULL)
        out = pool->logStream;

    fprintf(out, "Object Pool dump:\ncapacity = %lu\nlast_free = %lu\ndata:\n", pool->capacity, pool->last_free);
    if (pool->last_free == -1)
        fprintf(out, "Empty\n");
    else {
        for (size_t id = pool->last_free; id != -1; id = ({ size_t tmp = id; (pool->pages[tmp / GOBJPOOL_PAGE_CAP] + (tmp % GOBJPOOL_PAGE_CAP)); })->next)
            fprintf(out, "(%li)->", id);
        fprintf(out, "\n");
    }

    return gObjPool_status_OK;
}

#undef GET_NODE_UNSAFE

#endif /* GOBJPOOL_H */
