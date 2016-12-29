#include "parse.h"
#include <stdio.h>

static unsigned int op_precedence[] =
{
    10,
    10,
    20,
    20,
    20,
    40
};

static const unsigned int bind_left = 1;
static const unsigned int bind_right = 0;

static unsigned int op_binding[] =
{
    bind_left,
    bind_left,
    bind_left,
    bind_left,
    bind_left,
    bind_right
};

struct parse_state
{
    expir_expression* token;
    const char* start;
    bool error;
};

static void parse_error(parse_state* parse, const char* errstr)
{
    //TODO
    printf(errstr);
    putc('\n', stdout);
    parse->error = true;
}

static bool is_whitespace(char c)
{
    return
        c == ' ' ||
        c == '\t' ||
        c == '\r' ||
        c == '\n';
}

static bool is_number(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_operator(char c)
{
    return
        c == '+' ||
        c == '-' ||
        c == '/' ||
        c == '*' ||
        c == '%' ||
        c == '^';
}

static void consume_whitespace(const char** src)
{
    while(is_whitespace(**src))
        (*src)++;  
}

static expir_expression* tokenize_number(const char** src, expir_allocator* alloc, parse_state* parse)
{
    int ival = 0;
    while(is_number(**src)) {
        int i = (**src) - '0';
        ival = ival * 10 + i;
        (*src)++;
    }
    expir_int* expr = (expir_int*)alloc->alloc(sizeof(expir_int));
    *expr = {EXPIR_int, ival};
    return (expir_expression*)expr;
}

static expir_expression* tokenize_operator(const char** src, expir_allocator* alloc, parse_state* parse)
{
    char c = **src;
    (*src)++;
    ExpirBinaryOp op;
    switch(c) {
    case '+':
        op = EXPIR_add;
        //if(*src == '+') ++
        break;
    case '-':
        op = EXPIR_sub;
        //if(*src == '-') --
        break;
    case '*':
        op = EXPIR_mul;
        break;
    case '/':
        op = EXPIR_div;
        break;
    case '%':
        op = EXPIR_mod;
        break;
    case '^':
        op = EXPIR_pow;
        break;
    default:
        parse_error(parse, "Expected operator");
    }
    expir_binary_op* expr = (expir_binary_op*)alloc->alloc(sizeof(expir_binary_op));
    *expr = {EXPIR_binary_op, op, nullptr, nullptr};
    return (expir_expression*)expr;
}

static expir_expression* tokenize(const char** src, expir_allocator* alloc, parse_state* parse)
{
    const char* c = *src;
    consume_whitespace(src);

    if(is_number(**src)) {
        return tokenize_number(src, alloc, parse);
    }
    else if(is_operator(**src)) {
        return tokenize_operator(src, alloc, parse);
    }
    else if(**src == '\0') {
        return nullptr;
    }
    else {
        parse_error(parse, "Unexpected character");
        return nullptr;
    }
}

static expir_expression* consume_token(const char** src, expir_allocator* alloc, parse_state* state)
{
    expir_expression* ret = state->token;
    state->token = tokenize(src, alloc, state);
    return ret;
}



expir_expression* expir_parse_expr(const char** src, expir_allocator* alloc, parse_state* parse, unsigned int prec)
{
    expir_expression* expr = nullptr;
    while(parse->token) {
        switch(parse->token->type) {
        case EXPIR_int:
        case EXPIR_float:
        {
            expr = consume_token(src, alloc, parse);
            break;
        }
        case EXPIR_binary_op:
        {
            expir_binary_op* op = (expir_binary_op*)parse->token;
            if(prec > op_precedence[op->op])
                return expr;
            op->left = expr;
            expr = consume_token(src, alloc, parse);
            op->right = expir_parse_expr(src, alloc, parse, op_precedence[op->op] + op_binding[op->op]);
            break;
        }
        default:
        {
            parse_error(parse, "Unexpected token");
            return nullptr;
        }
        }
    }
    return expr;
}

expir_expression* expir_parse(const char* src, expir_allocator* alloc)
{
    parse_state parse = {nullptr, src, false};
    consume_token(&src, alloc, &parse);

    expir_expression* root = nullptr;

    root = expir_parse_expr(&src, alloc, &parse, 0);

    if(parse.error) {
        return nullptr;
    }
    return root;
}

bool expir_cmp(expir_expression* a, expir_expression* b)
{
    if(a == b)
        return true;
    if(a == nullptr || b == nullptr)
        return false;
    if(a->type != b->type)
        return false;

    switch(a->type) {
    case EXPIR_int:
        return ((expir_int*)a)->value == ((expir_int*)b)->value;
    case EXPIR_binary_op:
        if(((expir_binary_op*)a)->op != ((expir_binary_op*)b)->op)
            return false;
        return expir_cmp(((expir_binary_op*)a)->left, ((expir_binary_op*)b)->left) &&
            expir_cmp(((expir_binary_op*)a)->right, ((expir_binary_op*)b)->right);
        break;
    default:
        return false;
    }
}

static const char* unary_op_name(ExpirUnaryOp op)
{
    switch(op) {
    case EXPIR_pos:
        return "positive";
    case EXPIR_neg:
        return "negate";
    }
    return "UNKNOWN";
}

static const char* binary_op_name(ExpirBinaryOp op)
{
    switch(op) {
    case EXPIR_add:
        return "add";
    case EXPIR_sub:
        return "sub";
    case EXPIR_mul:
        return "mul";
    case EXPIR_div:
        return "div";
    case EXPIR_mod:
        return "mod";
    case EXPIR_pow:
        return "pow";
    }
    return "UNKNOWN";
}

void print_expr(expir_expression* expr, unsigned int pre)
{
    if(expr == nullptr) {
        printf("%*sNULL\n", pre, "");
        return;
    }

    switch(expr->type) {
    case EXPIR_int: {
        expir_int* iexpr = (expir_int*)expr;
        printf("%*sint: %d\n", pre, "", iexpr->value);
        break;
    }
    case EXPIR_float: {
        expir_float* fexpr = (expir_float*)expr;
        printf("%*sfloat: %f\n", pre, "", fexpr->value);
        break;
    }
    case EXPIR_unary_op: {
        expir_unary_op* uexpr = (expir_unary_op*)expr;
        const char* opname = unary_op_name(uexpr->op);
        printf("%*s%s\n", pre, "", opname);
        print_expr(uexpr->expr, pre + 2);
        break;
    }
    case EXPIR_binary_op: {
        expir_binary_op* bexpr = (expir_binary_op*)expr;
        const char* opname = binary_op_name(bexpr->op);
        printf("%*s%s\n", pre, "", opname);
        print_expr(bexpr->left, pre + 2);
        print_expr(bexpr->right, pre + 2);
        break;
    }
    default:
        printf("%*sUNKNOWN\n", pre, "");
        break;
    }
}
