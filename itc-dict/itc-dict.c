/* add dictionary to itc.c */

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

struct dict_entry {
    struct dict_entry *next;
    char *name;
    struct exec_token *xt;
};

struct exec_token *ip;
struct exec_token *work;

int sp = -1;
int stack[STACK_SIZE];

int rsp = -1;
struct exec_token *rstack[STACK_SIZE];

struct dict_entry *dict;

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
append_xt(struct exec_token *xt_list, struct exec_token *xt)
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

struct dict_entry *
new_dict_entry(void)
{
    struct dict_entry *result;

    result = malloc(sizeof(struct dict_entry));
    if (!result)
	abort();
    result->next = NULL;
    result->name = NULL;
    result->xt = NULL;
    return result;
}

struct dict_entry *
new_core_dict_entry(char *name, xt_code *code)
{
    struct dict_entry *result;

    result = new_dict_entry();
    result->name = name;
    result->xt = new_code_exec_token(code);
    return result;
}

void
add_dict_entry(char *name, xt_code *code)
{
    struct dict_entry *entry;

    entry = new_core_dict_entry(name, code);
    if (dict == NULL)
	dict = entry;
    else {
	entry->next = dict;
	dict = entry;
    }
}

void
init_dict(void)
{
    add_dict_entry("exit", do_exit);
    add_dict_entry("dup", do_dup);
    add_dict_entry("*", do_mul);
}

struct dict_entry *
find_dict_entry(char *name)
{
    struct dict_entry *p;

    for (p = dict; p; p = p->next)
	if (!strcmp(name, p->name))
	    return p;
    return NULL;
}

void
add_square(void)
{
    struct dict_entry *entry;

    entry = new_dict_entry();
    entry->name = "square";
    entry->xt = new_code_exec_token(do_enter);
    append_xt(entry->xt, find_dict_entry("dup")->xt);
    append_xt(entry->xt, find_dict_entry("*")->xt);
    append_xt(entry->xt, find_dict_entry("exit")->xt);
    entry->next = dict;
    dict = entry;
}

void
push_number(char *arg)
{
    long n;
    char *tail;

    n = strtol(arg, &tail, 0);
    if (!n && arg == tail)
	abort();
    push((int)n);
}

void
evaluate(struct exec_token *xt)
{
    struct exec_token entry_point;

    entry_point.next = NULL;
    entry_point.param = xt;
    ip = &entry_point;
    while (ip)
	do_next();
}

void
interpret(char *token)
{
    struct dict_entry *entry;

    entry = find_dict_entry(token);
    if (!entry)
	push_number(token);
    else
	evaluate(entry->xt);
}

/* ./a.out 2 dup \* square */
int
main(int argc, char *argv[])
{
    int i;

    init_dict();
    add_square();
    for (i = 1; i < argc; i++)
	interpret(argv[i]);
    printf("%d: %d\n", sp, stack[sp]);
    return 0;
}
