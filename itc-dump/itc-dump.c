/* indirect threaded code */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum token_type { CODE, PARAM };
enum { STACK_SIZE = 5 };

typedef void xt_code(void);

struct exec_token {
    struct exec_token *next;
    char *name;
    enum token_type type;
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
new_exec_token(char *name)
{
    struct exec_token *result;

    result = malloc(sizeof(struct exec_token));
    if (!result)
	abort();
    result->next = NULL;
    result->name = name;
    return result;
}

struct exec_token *
new_code_exec_token(char *name, xt_code *code)
{
    struct exec_token *result;

    result = new_exec_token(name);
    result->type = CODE;
    result->code = code;
    return result;
}

char *
get_xt_name(char *name)
{
    char *result;

    result = malloc(strlen(name) + 2);
    if (!result)
	abort();
    result[0] = '*';
    strcpy(result + 1, name);
    return result;
}

struct exec_token *
new_param_exec_token(struct exec_token *xt)
{
    struct exec_token *result;

    result = new_exec_token(get_xt_name(xt->name));
    result->type = PARAM;
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
dump_body_name(struct exec_token *xt)
{
    if (xt->type == CODE)
	printf("()");
    else
	printf(xt->param->name);
}

void
dump_next_name(struct exec_token *xt)
{
    if (xt->next)
	printf(" ->%s", xt->next->name);
    else
	printf(" ->null");
}

void
dump_xt(struct exec_token *xt)
{
    if (!xt) {
	putchar('\n');
	return;
    }
    printf("<%s ", xt->name);
    dump_body_name(xt);
    dump_next_name(xt);
    puts(">");
}

void
dump(char *msg)
{
    printf("--- %s\n", msg);
    printf("  work: "); dump_xt(work);
    printf("  ip:   "); dump_xt(ip);
    printf("  rsp:  %d\n", rsp); 
    printf("  sp:   %d\n", sp);
}

void
do_next(void)
{
    dump("begin next");
    work = ip->param;
    ip = ip->next;
    dump("end next");
    work->code();
}

void
do_enter(void)
{
    puts("enter");
    rpush(ip);
    ip = work->next;
}

void
do_exit(void)
{
    puts("exit");
    ip = rpop();
}

void
do_dup(void)
{
    puts("dup");
    push(stack[sp]);
}

void
do_mul(void)
{
    puts("mul");
    push(pop() * pop());
}

int
main(void)
{
    struct exec_token *xt_exit, *xt_dup, *xt_mul, *square, *caller;
    struct exec_token *sqsq;

    xt_exit = new_code_exec_token("exit", do_exit);
    xt_dup = new_code_exec_token("dup", do_dup);
    xt_mul = new_code_exec_token("mul", do_mul);

    square = new_code_exec_token("square-enter", do_enter);
    append(square, xt_dup);
    append(square, xt_mul);
    append(square, xt_exit);
    caller = new_exec_token("caller");
    caller->param = square;

    sqsq = new_code_exec_token("sqsq-enter", do_enter);
    append(sqsq, square);
    append(sqsq, square);
    append(sqsq, xt_exit);
    caller = new_exec_token("caller");
    caller->param = sqsq;

    push(2);
    ip = caller;

    while (ip)
	do_next();
    dump("finish");
    printf("%d: %d\n", sp, stack[sp]);

    return 0;
}
