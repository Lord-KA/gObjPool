typedef int GOBJPOOL_TYPE;  //TODO remove

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


    size_t id;
    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 0);

    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 1);

    int *ptr = NULL;
    // gObjPool_get(&pool, 4, &ptr);
    // printf("ptr = %p\n", ptr);
    // gObjPool_dumpFree(&pool, NULL);


    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 2);
    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 3);

    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 4);

    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 5);
    // gObjPool_dumpFree(&pool, NULL);


    gObjPool_free(&pool, 3);
    gObjPool_free(&pool, 5);
    // gObjPool_dumpFree(&pool, NULL);


    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 5);


    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 3);


    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 6);


    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 7);
    
    // gObjPool_dumpFree(&pool, NULL);

    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 8);

    // gObjPool_dumpFree(&pool, NULL);

    gObjPool_free(&pool, 1);
    gObjPool_free(&pool, 7);
    gObjPool_free(&pool, 3);
    gObjPool_free(&pool, 5);
    // gObjPool_dumpFree(&pool, NULL);

    gObjPool_dtor(&pool);
}

TEST(Auto, Stability)
{
    gObjPool poolStruct;
    gObjPool *pool = &poolStruct;
    
    gObjPool_ctor(pool, -1, NULL);
    size_t id = 0;
    
    for (size_t i = 0; i < 10000; ++i) {
        if (rnd() % 5 != 1) {
            EXPECT_FALSE(gObjPool_alloc(pool, &id));
        }
        else {
            gObjPool_status status = gObjPool_free(pool, rnd() % pool->capacity);
            EXPECT_TRUE(status == gObjPool_status_OK || status == gObjPool_status_BadId);
        }
    }

    gObjPool_dtor(pool);
}
