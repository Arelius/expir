#include "parse.h"
#include <stdio.h>
#include <stdarg.h>

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

enum ExpirToken
{
    TOK_fail = 0,
    TOK_int,
    TOK_float,
    TOK_openparen,
    TOK_closeparen,
    // corresponds to ExpirBinaryOp
    TOK_plus,
    TOK_minus,
    TOK_star,
    TOK_slash,
    TOK_percent,
    TOK_caret,
};

const char* TokToDiagStr(ExpirToken tok)
{
    switch(tok) {
    case TOK_int:
        return "int";
    case TOK_float:
        return "float";
    case TOK_openparen:
        return "'('";
    case TOK_closeparen:
        return "')'";
    case TOK_plus:
        return "'+'";
    case TOK_minus:
        return "'-'";
    case TOK_star:
        return "'*'";
    case TOK_slash:
        return "'/'";
    case TOK_percent:
        return "'*'";
    case TOK_caret:
        return "'^'";
    default:
        return "Unknown Token";
    }
}

static ExpirBinaryOp TokToBinaryOp(ExpirToken tok)
{
    return (ExpirBinaryOp)(EXPIR_add + (tok - TOK_plus));
}

struct token_value
{
    union
    {
        int intValue;
        float floatValue;
    };
};

struct parse_state
{
    ExpirToken token;
    token_value value;
    const char* start;
    bool error;
};

static void parse_error(parse_state* parse, const char* errstr, ...)
{
    //TODO
    va_list args;
    va_start(args, errstr);
    vprintf(errstr, args);
    va_end(args);
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

static ExpirToken tokenize_number(const char** src, token_value* outValue, expir_allocator* alloc, parse_state* parse)
{
    int ival = 0;
    while(is_number(**src)) {
        int i = (**src) - '0';
        ival = ival * 10 + i;
        (*src)++;
    }
    if(**src == '.') {
        (*src)++;
        //TODO: I doubt the percision of this.
        int num = 0;
        int denom = 1;
        while(is_number(**src)) {
            int i = (**src) - '0';
            num = num * 10 + i;
            denom = denom * 10;
            (*src)++;
        }
        outValue->floatValue = ival + (float)num / (float)denom;
        return TOK_float;
    }
    else {
        outValue->intValue = ival;
        return TOK_int;
    }
}

static ExpirToken tokenize_operator(const char** src, token_value* outValue, expir_allocator* alloc, parse_state* parse)
{
    char c = **src;
    (*src)++;
    ExpirToken tok;
    switch(c) {
    case '+':
        tok = TOK_plus;
        //if(*src == '+') ++
        break;
    case '-':
        tok = TOK_minus;
        //if(*src == '-') --
        break;
    case '*':
        tok = TOK_star;
        break;
    case '/':
        tok = TOK_slash;
        break;
    case '%':
        tok = TOK_percent;
        break;
    case '^':
        tok = TOK_caret;
        break;
    default:
        parse_error(parse, "Expected operator");
    }
    return tok;
}

static ExpirToken tokenize(const char** src, token_value* outValue, expir_allocator* alloc, parse_state* parse)
{
    const char* c = *src;
    consume_whitespace(src);

    if(is_number(**src)) {
        return tokenize_number(src, outValue, alloc, parse);
    }
    else if(**src == '(') {
        (*src)++;
        return TOK_openparen;
    }
    else if(**src == ')') {
        (*src)++;
        return TOK_closeparen;
    }
    else if(is_operator(**src)) {
        return tokenize_operator(src, outValue, alloc, parse);
    }
    else if(**src == '\0') {
        return TOK_fail;
    }
    else {
        parse_error(parse, "Unexpected character");
        return TOK_fail;
    }
}

static ExpirToken consume_token(const char** src, expir_allocator* alloc, parse_state* state)
{
    ExpirToken ret = state->token;
    token_value value = {};
    state->token = tokenize(src, &value, alloc, state);
    state->value = value;
    return ret;
}

expir_expression* expir_parse_expr(const char** src, expir_allocator* alloc, parse_state* parse, unsigned int prec)
{
    expir_expression* expr = nullptr;
    while(parse->token) {
        switch(parse->token) {
        case TOK_int:
        {
            expir_int* iexpr = (expir_int*)alloc->alloc(sizeof(expir_int));
            *iexpr = {EXPIR_int, parse->value.intValue};
            if(expr != nullptr) parse_error(parse, "Unexpected %s", TokToDiagStr(parse->token));
            expr = (expir_expression*)iexpr;
            consume_token(src, alloc, parse);
            break;
        }
        case TOK_float:
        {
            expir_float* fexpr = (expir_float*)alloc->alloc(sizeof(expir_float));
            *fexpr = {EXPIR_float, parse->value.floatValue};
            if(expr != nullptr) parse_error(parse, "Unexpected %s", TokToDiagStr(parse->token));
            expr = (expir_expression*)fexpr;
            consume_token(src, alloc, parse);
            break;
        }
        case TOK_openparen:
        {
            if(expr != nullptr) parse_error(parse, "Unexpected %s", TokToDiagStr(parse->token));
            consume_token(src, alloc, parse);
            expr = expir_parse_expr(src, alloc, parse, 0);
            if(expr == nullptr)
                 parse_error(parse, "Empty block '()' unexpected", TokToDiagStr(parse->token));
            if(parse->token == TOK_closeparen) {
                consume_token(src, alloc, parse);
            }
            else {
                parse_error(parse, "Expected ')' got %s", TokToDiagStr(parse->token));
            }
            break;
        }
        case TOK_closeparen:
            return expr;
        case TOK_plus:
        case TOK_minus:
        case TOK_star:
        case TOK_slash:
        case TOK_percent:
        case TOK_caret:
        {
            expir_binary_op* op = (expir_binary_op*)alloc->alloc(sizeof(expir_binary_op));
            *op = {EXPIR_binary_op, TokToBinaryOp(parse->token), nullptr, nullptr};
            if(prec > op_precedence[op->op])
                return expr;
            op->left = expr;
            expr = (expir_expression*)op;
            consume_token(src, alloc, parse);
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
    parse_state parse = {TOK_fail, 0, src, false};
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
    case EXPIR_float:
        return ((expir_float*)a)->value == ((expir_float*)b)->value;
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
