#include "parse.h"

#include <stdio.h>
#include <stdlib.h>

void* expir_allocate(size_t size)
{
    return malloc(size);
}

void expir_clear_allocs()
{
    // Clear all allocations.
}

int testNum;
int passedTests;
int failedTests;

void test(
    const char* name,
    bool pass)
{
    testNum++;
    if(pass) {
        passedTests++;
    }
    else {
        printf("FAILED: Test '%s' failed.\n", name);
        failedTests++;
    }
}

int main(int argc, const char** argv)
{
    testNum = 0;
    passedTests = 0;
    failedTests = 0;

    expir_allocator alloc = {expir_allocate, expir_clear_allocs};

    {
        expir_int int1 = {EXPIR_int, 1};
        test(
            "1",
            expir_cmp(
                expir_parse("1", &alloc),
                (expir_expression*)&int1));
    }

    {
        expir_int int2 = {EXPIR_int, 2};
        test(
            "2 not 1",
            !expir_cmp(
                expir_parse("1", &alloc),
                (expir_expression*)&int2));
    }

    {
        expir_int int2 = {EXPIR_int, 2};
        test(
            "2",
            expir_cmp(
                expir_parse("2", &alloc),
                (expir_expression*)&int2));
    }

    {
        expir_int int1 = {EXPIR_int, 1};
        test(
            "1 not 2",
            !expir_cmp(
                expir_parse("2", &alloc),
                (expir_expression*)&int1));
    }

    {
        expir_int int1 = {EXPIR_int, 1};
        expir_unary_op neg = {EXPIR_unary_op, EXPIR_neg, (expir_expression*)&int1};
        test(
            "-1",
            expir_cmp(
                expir_parse("-1", &alloc),
                (expir_expression*)&neg));
    }

    {
        expir_int int1 = {EXPIR_int, 1};
        expir_binary_op add = {EXPIR_binary_op, EXPIR_add, (expir_expression*)&int1, (expir_expression*)&int1};
        test(
            "1 + 1",
            expir_cmp(
                expir_parse("1 + 1", &alloc),
                (expir_expression*)&add));
    }

    {
        expir_int int1 = {EXPIR_int, 1};
        expir_binary_op add = {EXPIR_binary_op, EXPIR_add, (expir_expression*)&int1, (expir_expression*)&int1};
        expir_binary_op add2 = {EXPIR_binary_op, EXPIR_add, (expir_expression*)&add, (expir_expression*)&int1};
        test(
            "left associative +",
            expir_cmp(
                expir_parse("1 + 1 + 1", &alloc),
                (expir_expression*)&add2));
    }

    {
        expir_int int2 = {EXPIR_int, 2};
        expir_int int3 = {EXPIR_int, 3};
        expir_int int4 = {EXPIR_int, 4};
        expir_binary_op pow = {EXPIR_binary_op, EXPIR_pow, (expir_expression*)&int3, (expir_expression*)&int4};
        expir_binary_op pow2 = {EXPIR_binary_op, EXPIR_pow, (expir_expression*)&int2, (expir_expression*)&pow};
        test(
            "right associative ^",
            expir_cmp(
                expir_parse("2 ^ 3 ^ 4", &alloc),
                (expir_expression*)&pow2));
    }

    {
        expir_int int1 = {EXPIR_int, 1};
        expir_binary_op mul = {EXPIR_binary_op, EXPIR_mul, (expir_expression*)&int1, (expir_expression*)&int1};
        expir_binary_op add = {EXPIR_binary_op, EXPIR_add, (expir_expression*)&int1, (expir_expression*)&mul};
        test(
            "1 + 1 * 1",
            expir_cmp(
                expir_parse("1 + 1 * 1", &alloc),
                (expir_expression*)&add));
    }

    {
        expir_int int1 = {EXPIR_int, 1};
        expir_binary_op mul = {EXPIR_binary_op, EXPIR_mul, (expir_expression*)&int1, (expir_expression*)&int1};
        expir_binary_op add = {EXPIR_binary_op, EXPIR_add, (expir_expression*)&mul, (expir_expression*)&int1};
        test(
            "1 * 1 + 1",
            expir_cmp(
                expir_parse("1 * 1 + 1", &alloc),
                (expir_expression*)&add));
    }

    printf("TEST %s. %d tests, %d failures.\n", (failedTests == 0) ? "SUCCESS" : "FAILURE", testNum, failedTests);

    return (failedTests == 0) ? 0 : -1;
}
