#include <iostream>
typedef size_t GOBJPOOL_TYPE;

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


    GOBJPOOL_TYPE *data = NULL;
    size_t id;
    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 0);

    gObjPool_alloc(&pool, &id);
    EXPECT_EQ(id, 1);

    GOBJPOOL_TYPE *ptr = NULL;

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

    std::vector<size_t> check;

    gObjPool_status status = gObjPool_ctor(pool, -1, NULL);
    EXPECT_EQ(status, gObjPool_status_OK);
    size_t id = 0;

    for (size_t i = 0; i < 300000; ++i) {
        if (rnd() % 3 != 1) {
            status = gObjPool_alloc(pool, &id);
            EXPECT_EQ(status, gObjPool_status_OK);
            check.push_back(id);

            GOBJPOOL_TYPE *data = NULL;
            status = gObjPool_get(pool, id, &data);
            EXPECT_EQ(status, gObjPool_status_OK);
            *data = id;
        }
        else {
            size_t pos = rnd() % check.size();
            if (check.size() > 0) {
                status = gObjPool_free(pool, check[pos]);
                EXPECT_EQ(status, gObjPool_status_OK);
                check.erase(check.begin() + pos);
            }
        }
        if (check.size() > 0) {
            size_t pos = rnd() % check.size();
            GOBJPOOL_TYPE *data = NULL;
            status = gObjPool_get(pool, check[pos], &data);
            EXPECT_EQ(status, gObjPool_status_OK);
            EXPECT_EQ(*data, check[pos]);
        }
    }
    for (size_t i = 0; i < check.size(); ++i)
        EXPECT_EQ(gObjPool_free(pool, check[i]), gObjPool_status_OK);

    EXPECT_EQ(pool->capacity, pool->allocated_pages * GOBJPOOL_PAGE_CAP);

    status = gObjPool_dtor(pool);
    EXPECT_EQ(status, gObjPool_status_OK);
}
