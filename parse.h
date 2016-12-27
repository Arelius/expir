#ifndef EXPIR_PARSE_H
#define EXPIR_PARSE_H

struct expir_allocator
{
    void* (*alloc)(size_t size);
    void (*clear)();
};

enum ExpirType
{
    EXPIR_int,
    EXPIR_float,
    EXPIR_unary_op,
    EXPIR_binary_op
};

enum ExpirBinaryOp
{
    EXPIR_add,
    EXPIR_sub,
    EXPIR_mul,
    EXPIR_div,
    EXPIR_mod,
    EXPIR_pow
};

// We make sure these overlap with the corresponding binary symbols so that we can do trivial conversion by cast after tokenization.
enum ExpirUnaryOp
{
    EXPIR_pos = EXPIR_add,
    EXPIR_neg = EXPIR_sub,
    // Do we like these?
    //EXPIR_inc,
    //EXPIR_dec
};

struct expir_expression
{
    ExpirType type;
};

struct expir_int
{
    ExpirType type;
    int value;
};

struct expir_float
{
    ExpirType type;
    float value;
};

struct expir_binary_op
{
    ExpirType type;
    ExpirBinaryOp op;
    expir_expression* left;
    expir_expression* right;
};

struct expir_unary_op
{
    ExpirType type;
    ExpirUnaryOp op;
    expir_expression* expr;
};

expir_expression* expir_parse(const char* src, expir_allocator* alloc);
bool expir_cmp(expir_expression*, expir_expression*);
void print_expr(expir_expression* expr, unsigned int pre = 0);

#endif //EXPIR_PARSE_H
