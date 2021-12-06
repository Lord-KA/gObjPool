typedef int GOBJPOOL_TYPE;  

#define NLOGS
#include "gobjpool.h"
#include "gtest/gtest.h"

#include <random>

std::mt19937 rnd(179);

TEST(manual, basic)
{
    gObjPool pool;
    gObjPool_ctor(&pool, -1, NULL);
    // gObjPool_dumpFree(&pool, NULL);


    int *data = NULL;
    size_t id;
    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 0);

    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 1);

    int *ptr = NULL;

    #ifndef NLOGS
        gObjPool_dumpFree(&pool, NULL);
    #endif

    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 2);
    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 3);

    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 4);

    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 5);
    #ifndef NLOGS
        gObjPool_dumpFree(&pool, NULL);
    #endif


    gObjPool_free(&pool, 3);
    gObjPool_free(&pool, 5);
    #ifndef NLOGS
        gObjPool_dumpFree(&pool, NULL);
    #endif


    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 5);
    data = NULL;
    gObjPool_get(&pool, id, &data);
    gObjPool_getId(&pool, data, &id);
    EXPECT_EQ(id, 5);


    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 3);
    data = NULL;
    gObjPool_get(&pool, id, &data);
    gObjPool_getId(&pool, data, &id);
    EXPECT_EQ(id, 3);


    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 6); 
    data = NULL;
    gObjPool_get(&pool, id, &data);
    gObjPool_getId(&pool, data, &id);
    EXPECT_EQ(id, 6);


    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 7);
    data = NULL;
    gObjPool_get(&pool, id, &data);
    gObjPool_getId(&pool, data, &id);
    EXPECT_EQ(id, 7);
    
    #ifndef NLOGS
        gObjPool_dumpFree(&pool, NULL);
    #endif

    gObjPool_alloc(&pool, &id);

    EXPECT_EQ(id, 8);

    data = NULL;
    gObjPool_get(&pool, id, &data);
    gObjPool_getId(&pool, data, &id);
    EXPECT_EQ(id, 8);

    #ifndef NLOGS
        gObjPool_dumpFree(&pool, NULL);
    #endif

    gObjPool_free(&pool, 1);
    gObjPool_free(&pool, 7);
    gObjPool_free(&pool, 3);
    gObjPool_free(&pool, 5);
    #ifndef NLOGS
        gObjPool_dumpFree(&pool, NULL);
    #endif

    gObjPool_dtor(&pool);
}

TEST(Auto, Stability)
{
    gObjPool poolStruct;
    gObjPool *pool = &poolStruct;
    
    gObjPool_status status = gObjPool_ctor(pool, -1, NULL);
    EXPECT_TRUE(status == gObjPool_status_OK || status == gObjPool_status_BadId);
    size_t id = 0;
    
    for (size_t i = 0; i < 100000; ++i) {
        if (rnd() % 5 != 1) {
            status = gObjPool_alloc(pool, &id);
            EXPECT_TRUE(status == gObjPool_status_OK || status == gObjPool_status_BadId);
        }
        else {
            status = gObjPool_free(pool, rnd() % pool->capacity);
            EXPECT_TRUE(status == gObjPool_status_OK || status == gObjPool_status_BadId);
        }
        GOBJPOOL_TYPE *data = NULL;
        status = gObjPool_get(pool, rnd() % pool->capacity, &data);
        EXPECT_TRUE(status == gObjPool_status_OK || status == gObjPool_status_BadId);

    }

    status = gObjPool_dtor(pool);
    EXPECT_TRUE(status == gObjPool_status_OK || status == gObjPool_status_BadId);
}
