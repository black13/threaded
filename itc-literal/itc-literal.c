/*
 * add do_literal to itc-colon.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum interp_state { INTERPRET, COMPILE };
enum word_type { NORMAL, IMMEDIATE };
enum { STACK_SIZE = 5 };

typedef void xt_code(void);

struct literal {
    int value;
};

struct exec_token {
    struct exec_token *next;
    union {
	xt_code *code;
	struct exec_token *param;
	struct literal *lit;
    };
};

struct dict_entry {
    struct dict_entry *next;
    char *name;
    enum word_type type;
    struct exec_token *xt;
};

struct exec_token *ip;
struct exec_token *work;

int sp = -1;
int stack[STACK_SIZE];

int rsp = -1;
struct exec_token *rstack[STACK_SIZE];

struct dict_entry *dict;

enum interp_state state = INTERPRET;

int itoken;
char **program;

static char *get_token(void);
static struct dict_entry *find_dict_entry(char *);
static struct dict_entry *new_dict_entry(void);
static void add_dict_entry(struct dict_entry *);

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
    tail->next = xt;
}

void
append_xt_xt(struct exec_token *xt_list, struct exec_token *xt)
{
    append_xt(xt_list, new_param_exec_token(xt));
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
do_colon(void)
{
    struct dict_entry *entry;

    entry = new_dict_entry();
    entry->name = get_token();
    if (!entry->name)
	exit(0);
    entry->xt = new_code_exec_token(do_enter);
    add_dict_entry(entry);
    rpush(entry->xt);
    state = COMPILE;
}

void
do_semicolon(void)
{
    struct exec_token *xt;

    xt = rpop();
    append_xt_xt(xt, find_dict_entry("exit")->xt);
    state = INTERPRET;
}

void
do_literal(void)
{
    push(ip->lit->value);
    ip = ip->next;
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
    result->type = NORMAL;
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
add_dict_entry(struct dict_entry *entry)
{
    if (dict == NULL)
	dict = entry;
    else {
	entry->next = dict;
	dict = entry;
    }
}

void
add_normal_dict_entry(char *name, xt_code *code)
{
    struct dict_entry *entry;

    entry = new_core_dict_entry(name, code);
    add_dict_entry(entry);
}

void
add_immediate_dict_entry(char *name, xt_code *code)
{
    struct dict_entry *entry;

    entry = new_core_dict_entry(name, code);
    entry->type = IMMEDIATE;
    add_dict_entry(entry);
}

void
init_dict(void)
{
    add_normal_dict_entry("exit", do_exit);
    add_normal_dict_entry(":", do_colon);
    add_immediate_dict_entry(";", do_semicolon);
    add_normal_dict_entry("literal", do_literal);
    add_normal_dict_entry("dup", do_dup);
    add_normal_dict_entry("*", do_mul);
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

int
token_to_int(char *token)
{
    long n;
    char *tail;

    n = strtol(token, &tail, 0);
    if (!n && token == tail)
	abort();
    return (int)n;
}

void
push_number(char *token)
{
    push(token_to_int(token));
}

void
interpret_xt(struct exec_token *xt)
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
	interpret_xt(entry->xt);
}

struct literal *
new_literal(char *token)
{
    struct literal *result;

    result = malloc(sizeof(struct literal));
    if (!result)
	abort();
    result->value = token_to_int(token);
    return result;
}

void
compile_literal(char *token)
{
    struct exec_token *lit_xt;

    append_xt_xt(rstack[rsp], find_dict_entry("literal")->xt);
    lit_xt = new_exec_token();
    lit_xt->lit = new_literal(token);
    append_xt(rstack[rsp], lit_xt);
}

void
compile(char *token)
{
    struct dict_entry *entry;

    entry = find_dict_entry(token);
    if (!entry) {
	compile_literal(token);
	return;
    }
    if (entry->type == IMMEDIATE) {
	interpret_xt(entry->xt);
	return;
    }
    append_xt_xt(rstack[rsp], entry->xt);
}

void
init_keyboard_interp(char **argv)
{
    itoken = 1;
    program = argv;
}

char *
get_token(void)
{
    if (!program[itoken])
	return NULL;
    return program[itoken++];
}

int
main(int argc, char *argv[])
{
    char *token;

    init_dict();
    init_keyboard_interp(argv);
    while ((token = get_token()))
	if (state == INTERPRET)
	    interpret(token);
	else
	    compile(token);
    printf("%d: %d\n", sp, stack[sp]);
    return 0;
}
