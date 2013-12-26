/* indirect threaded code */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { STACK_SIZE = 5 };

typedef void xt_code(void);

struct exec_token {
    struct exec_token *next;
    union {
	xt_code *code;
	struct exec_token *param;
    };
};

struct exec_token *ip;
struct exec_token *work;

int sp = -1;
int stack[STACK_SIZE];

int rsp = -1;
struct exec_token *rstack[STACK_SIZE];

void
push(int n)
{
    sp++;
    if (sp >= STACK_SIZE)
	abort();
    stack[sp] = n;
}

int
pop(void)
{
    if (sp < 0)
	abort();
    return stack[sp--];
}

void
rpush(struct exec_token *xt)
{
    rsp++;
    if (rsp >= STACK_SIZE)
	abort();
    rstack[rsp] = xt;
}

struct exec_token *
rpop(void)
{
    if (rsp < 0)
	abort();
    return rstack[rsp--];
}

struct exec_token *
new_exec_token(void)
{
    struct exec_token *result;

    result = malloc(sizeof(struct exec_token));
    if (!result)
	abort();
    result->next = NULL;
    return result;
}

struct exec_token *
new_code_exec_token(xt_code *code)
{
    struct exec_token *result;

    result = new_exec_token();
    result->code = code;
    return result;
}

struct exec_token *
new_param_exec_token(struct exec_token *xt)
{
    struct exec_token *result;

    result = new_exec_token();
    result->param = xt;
    return result;
}

struct exec_token *
get_tail(struct exec_token *xt)
{
    struct exec_token *result;

    for (result = xt; result->next; result = result->next)
	;
    return result;
}

void
append(struct exec_token *xt_list, struct exec_token *xt)
{
    struct exec_token *tail;

    tail = get_tail(xt_list);
    tail->next = new_param_exec_token(xt);
}

void
do_next(void)
{
    work = ip->param;
    ip = ip->next;
    work->code();
}

void
do_enter(void)
{
    rpush(ip);
    ip = work->next;
}

void
do_exit(void)
{
    ip = rpop();
}

void
do_dup(void)
{
    push(stack[sp]);
}

void
do_mul(void)
{
    push(pop() * pop());
}

int
main(void)
{
    struct exec_token *xt_exit, *xt_dup, *xt_mul, *square, *caller;
    struct exec_token *sqsq;

    xt_exit = new_code_exec_token(do_exit);
    xt_dup = new_code_exec_token(do_dup);
    xt_mul = new_code_exec_token(do_mul);

    square = new_code_exec_token(do_enter);
    append(square, xt_dup);
    append(square, xt_mul);
    append(square, xt_exit);
    // caller = new_exec_token();
    // caller->param = square;

    sqsq = new_code_exec_token(do_enter);
    append(sqsq, square);
    append(sqsq, square);
    append(sqsq, xt_exit);
    caller = new_exec_token();
    caller->param = sqsq;

    push(2);
    ip = caller;

    while (ip)
	do_next();
    printf("%d: %d\n", sp, stack[sp]);

    return 0;
}
