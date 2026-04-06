/*
 * Things to do:
 *	- Arbitrary-precision math
 *	- Variables
 *	- Programming control statements:
 *	  <expression> if <commands> [else] <commands> end
 *	  begin <expression> while <commands> end
 *	  <lower> <upper> for <variable> <commands> next
 *	  <lower> <upper> for <variable> <commands> <n> step
 *	  <lower> <upper> start <commands> next
 *	  <lower> <upper> start <commands> <n> step
 *	  Some sort of do ... until loop
 *	- Complex numbers
 *	- Different word sizes; better integer/real number support
 *	- Polar/rectangular support
 *	- Radian/degrees mode
 *	- Strings (needed to do macro definition stuff)
 *	- Arrays/vectors/matrices
 *	- xroot, combinatorics, time access
 *	- shell escape
 *	- $RPNINIT, ~/.rpnrc, ~/.rpnstk support
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "rpn.h"

static const char *thiscmd;
static void init_commands(void);

static const char *err_msgs[] = {
	[ERR_DIVBYZERO]  = "Division by zero.",
	[ERR_DOMAIN]     = "Argument is outside of function domain.",
	[ERR_UNKNOWNCMD] = "Unknown command.",
	[ERR_ARGC]       = "Too few arguments.",
	[ERR_RANGE]      = "Number out of range.",
};

void
error(enum rpn_err err)
{
	printf("Error: %s: %s\n", thiscmd, err_msgs[err]);
	S.stop = 1;
}

/*
 * Built-in commands
 */

static void
cmd_not(void)
{
	*topptr() = !*topptr();
}

static void
cmd_ne(void)
{
	double tmpnum = popnum();
	*topptr() = *topptr() != tmpnum;
}

static void
cmd_mod(void)
{
	if (*topptr() == 0)
		error(ERR_DIVBYZERO);
	else {
		double tmpnum = popnum();
		*topptr() = fmod(*topptr(), tmpnum);
	}
}

static void
cmd_bitand(void)
{
	double tmpnum = popnum();
	*topptr() = (uint64_t)*topptr() & (uint64_t)tmpnum;
}

static void
cmd_and(void)
{
	double tmpnum = popnum();
	*topptr() = *topptr() && tmpnum;
}

static void
cmd_mul(void)
{
	double tmpnum = popnum();
	*topptr() *= tmpnum;
}

static void
cmd_add(void)
{
	double tmpnum = popnum();
	*topptr() += tmpnum;
}

static void
cmd_inc(void)
{
	(*topptr())++;
}

static void
cmd_sub(void)
{
	double tmpnum = popnum();
	*topptr() -= tmpnum;
}

static void
cmd_dec(void)
{
	(*topptr())--;
}

static void
cmd_div(void)
{
	if (*topptr() == 0)
		error(ERR_DIVBYZERO);
	else {
		double tmpnum = popnum();
		*topptr() /= tmpnum;
	}
}

static void
cmd_lt(void)
{
	double tmpnum = popnum();
	*topptr() = *topptr() < tmpnum;
}

static void
cmd_bitshl(void)
{
	double tmpnum = popnum();
	*topptr() = (uint64_t)*topptr() << (uint64_t)tmpnum;
}

static void
cmd_le(void)
{
	double tmpnum = popnum();
	*topptr() = *topptr() <= tmpnum;
}

static void
cmd_eq(void)
{
	double tmpnum = popnum();
	*topptr() = *topptr() == tmpnum;
}

static void
cmd_gt(void)
{
	double tmpnum = popnum();
	*topptr() = *topptr() > tmpnum;
}

static void
cmd_ge(void)
{
	double tmpnum = popnum();
	*topptr() = *topptr() >= tmpnum;
}

static void
cmd_bitshr(void)
{
	double tmpnum = popnum();
	*topptr() = (uint64_t)*topptr() >> (uint64_t)tmpnum;
}

static void
cmd_bitxor(void)
{
	double tmpnum = popnum();
	*topptr() = (uint64_t)*topptr() ^ (uint64_t)tmpnum;
}

/* Someday, this will be a macro */
static void
cmd_abs(void)
{
	if (*topptr() < 0)
		*topptr() = -*topptr();
}

static void
cmd_acos(void)
{
	if (*topptr() < -1 || *topptr() > 1)
		error(ERR_DOMAIN);
	else
		*topptr() = acos(*topptr());
}

static void
cmd_asin(void)
{
	if (*topptr() < -1 || *topptr() > 1)
		error(ERR_DOMAIN);
	else
		*topptr() = asin(*topptr());
}

static void
cmd_atan(void)
{
	*topptr() = atan(*topptr());
}

static void
cmd_ceil(void)
{
	*topptr() = ceil(*topptr());
}

static void
cmd_cos(void)
{
	*topptr() = cos(*topptr());
}

static void
cmd_cosh(void)
{
	*topptr() = cosh(*topptr());
}

static void
cmd_depth(void)
{
	pushnum(S.M->d);
}

static void
cmd_ntohs(void) {
	unsigned short s = (unsigned short) popnum();
	pushnum(ntohs(s));
}

static void
cmd_htons(void) {
	unsigned short s = (unsigned short) popnum();
	pushnum(htons(s));
}

static void
cmd_ntohl(void) {
	unsigned s = (unsigned) popnum();
	pushnum(ntohl(s));
}

static void
cmd_htonl(void) {
	unsigned s = (unsigned) popnum();
	pushnum(htonl(s));
}

static void
cmd_stack(void) {
	S.stackmode = S.stackmode ? 0 : 1 ;
}

static void
cmd_pad(void) {
	unsigned s = (unsigned) popnum();
	S.padcount = s;
}

static void
cmd_ipaddr(void) {
	unsigned addr = *topptr();
	pushnum(((u_char *)&addr)[0]);
	pushnum(((u_char *)&addr)[1]);
	pushnum(((u_char *)&addr)[2]);
	pushnum(((u_char *)&addr)[3]);
}

static void
cmd_drop(void)
{
	popnum();
}

/* Someday, this will be a macro */
static void
cmd_dropn(void)
{
	double tmpnum;
	for (tmpnum = popnum(); tmpnum > 0; tmpnum--)
		cmd_drop();
}

static void
cmd_dup(void)
{
	pushnum(*topptr());
}

/* Someday, this will be a macro */
static void
cmd_dupn(void)
{
	unsigned n = (unsigned)popnum();
	size_t base_idx = S.M->d - n;
	unsigned i;
	for (i = 0; i < n; i++)
		pushnum(S.M->data[base_idx + i]);
}

static void
cmd_e(void)
{
	pushnum(2.7182818284590452354);
}

static void
cmd_exp(void)
{
	*topptr() = exp(*topptr());
}

/* Someday, this will be a macro */
static void
cmd_fact(void)
{
	double tmpnum;
	if (*topptr() < 0 || modf(*topptr(), &tmpnum) != 0)
		error(ERR_DOMAIN);
	else if (*topptr() == 0)
		*topptr() = 1;
	else
		for (tmpnum = *topptr(), *topptr() = 1; tmpnum > 1; tmpnum--)
			*topptr() *= tmpnum;
}

static void
cmd_floor(void)
{
	*topptr() = floor(*topptr());
}

static void
cmd_fp(void)
{
	double tmpnum;
	*topptr() = modf(*topptr(), &tmpnum);
}

static void
cmd_getbase(void)
{
	pushnum(S.base);
}

static void
cmd_ip(void)
{
	modf(*topptr(), topptr());
}

static void
cmd_ln(void)
{
	if (*topptr() < 0)
		error(ERR_DOMAIN);
	else
		*topptr() = log(*topptr());
}

static void
cmd_log(void)
{
	if (*topptr() < 0)
		error(ERR_DOMAIN);
	else
		*topptr() = log10(*topptr());
}

/* Someday, this will be a macro */
static void
cmd_max(void)
{
	removenth(*topptr() > *nthptr(1) ? 1 : 0);
}

/* Someday, this will be a macro */
static void
cmd_min(void)
{
	removenth(*topptr() < *nthptr(1) ? 1 : 0);
}

static void
cmd_pi(void)
{
	pushnum(3.14159265358979323846);
}

static void
cmd_pick_roll(void)
{
	unsigned n = (unsigned)popnum();

	if (n == 0)
		return;

	pushnum(*nthptr(n - 1));

	if (strcmp(thiscmd, "roll") == 0)
		removenth(n);
}

static void
cmd_pow(void)
{
	double tmpnum;
	if ((*nthptr(1) == 0 && *topptr() <= 0) ||
	    (*nthptr(1) < 0 && modf(*topptr(), &tmpnum) != 0))
		error(ERR_DOMAIN);
	else {
		tmpnum = popnum();
		*topptr() = pow(*topptr(), tmpnum);
	}
}

static void
cmd_quit(void)
{
	exit(0);
}

static void
cmd_rand(void)
{
	pushnum(random());
}

static void
cmd_repeat(void)
{
	if (*topptr() <= 0)
		error(ERR_DOMAIN);
	else
		S.pending_repeat = popnum();
}

static void
cmd_rolld(void)
{
	unsigned n = (unsigned)popnum();

	if (n <= 1)
		return;

	double val = S.M->data[S.M->d - 1];
	memmove(&S.M->data[S.M->d - n + 1], &S.M->data[S.M->d - n], (n - 1) * sizeof(double));
	S.M->data[S.M->d - n] = val;
}

static void
cmd_setbase(void)
{
	int num = popnum();
	if (num < 2 || num > 36)
		error(ERR_DOMAIN);
	else
		S.base = num;
}

/* Someday, this will be a macro */
static void
cmd_sign(void)
{
	if (*topptr())
		*topptr() = *topptr() < 0 ? -1 : 1;
}

static void
cmd_sin(void)
{
	*topptr() = sin(*topptr());
}

static void
cmd_sinh(void)
{
	*topptr() = sinh(*topptr());
}

static void
cmd_sqrt(void)
{
	if (*topptr() < 0)
		error(ERR_DOMAIN);
	else
		*topptr() = sqrt(*topptr());
}

static void
cmd_swap(void)
{
	double tmpnum = *nthptr(1);
	*nthptr(1) = *topptr();
	*topptr() = tmpnum;
}

static void
cmd_tanh(void)
{
	*topptr() = tanh(*topptr());
}

static void
cmd_version(void)
{
	pushnum(VERSION);
}

static void
cmd_bitor(void)
{
	double tmpnum = popnum();
	*topptr() = (uint64_t)*topptr() | (uint64_t)tmpnum;
}

static void
cmd_or(void)
{
	double tmpnum = popnum();
	*topptr() = *topptr() || tmpnum;
}

static void
cmd_bitcmpl(void)
{
	*topptr() = ~(uint64_t)*topptr();
}

/*
 * Hash table infrastructure (djb2, open addressing, power-of-2 size)
 */
static unsigned
djb2(const char *str)
{
	unsigned hash = 5381;
	int c;
	while ((c = *str++))
		hash = ((hash << 5) + hash) + c;
	return hash;
}

/*
 * Macro hash table
 */
#define MACRO_HT_SIZE 256
static struct macro macro_ht[MACRO_HT_SIZE];

static void
addmacro(char *name, char *operation)
{
	unsigned idx = djb2(name) & (MACRO_HT_SIZE - 1);
	while (macro_ht[idx].occupied) {
		if (strcmp(macro_ht[idx].name, name) == 0) {
			macro_ht[idx].operation = operation;
			return;
		}
		idx = (idx + 1) & (MACRO_HT_SIZE - 1);
	}
	macro_ht[idx].name = name;
	macro_ht[idx].operation = operation;
	macro_ht[idx].occupied = 1;
}

char *
findmacro(const char *name)
{
	unsigned idx = djb2(name) & (MACRO_HT_SIZE - 1);
	while (macro_ht[idx].occupied) {
		if (strcmp(macro_ht[idx].name, name) == 0)
			return macro_ht[idx].operation;
		idx = (idx + 1) & (MACRO_HT_SIZE - 1);
	}
	return NULL;
}

void
init_macros(void)
{
	char *env = getenv("HOME");

	init_commands();

	addmacro("total", "depth -- repeat +");
	addmacro("tan", "dup sin swap cos /");
	addmacro("sq", "2 pow");
	addmacro("sec", "cos inv");
	addmacro("rot", "3 roll");
	addmacro("rep", "repeat");
	addmacro("over", "2 pick");
	addmacro("oct", "8 setbase");
	addmacro("inv", "1 swap /");
	addmacro("hex", "16 setbase");
	addmacro("exit", "quit");
	addmacro("q", "quit");
	addmacro("dec", "10 setbase");
	addmacro("csc", "sin inv");
	addmacro("cot", "tan inv");
	addmacro("clr", "depth dropn");
	addmacro("chs", "-1 *");
	addmacro("bin", "2 setbase");
	addmacro("aven", "dup -- swap depth rolld repeat + depth roll /");
	addmacro("ave", "depth aven");
	addmacro("alog", "10 swap pow");
	addmacro("?", "help");

	if (env) {
		char buf[10240];
		FILE *fp;
		char *p;
		snprintf(buf, sizeof(buf), "%s/.rpn_macros", env);

		fp = fopen(buf, "r");
		if (fp) {
			while (fgets(buf, sizeof(buf), fp) != NULL) {
				if (buf[0] == '#')
					continue;

				p = buf;
				while (*p && *p != ' ')
					p++;

				if (*p) {
					*p++ = 0;
					addmacro(strdup(buf), strdup(p));
				}
			}
			fclose(fp);
		}
	}
}

/*
 * Command hash table
 */
#define CMD_HT_SIZE 256

struct cmd_slot {
	struct command cmd;
	int occupied;
};

static struct cmd_slot cmd_ht[CMD_HT_SIZE];

static void cmd_help(void);

static void
cmd_ht_insert(const char *name, long numargs, void (*function)(void))
{
	unsigned idx = djb2(name) & (CMD_HT_SIZE - 1);
	while (cmd_ht[idx].occupied) {
		if (strcmp(cmd_ht[idx].cmd.name, name) == 0) {
			cmd_ht[idx].cmd.numargs = numargs;
			cmd_ht[idx].cmd.function = function;
			return;
		}
		idx = (idx + 1) & (CMD_HT_SIZE - 1);
	}
	cmd_ht[idx].cmd.name = name;
	cmd_ht[idx].cmd.numargs = numargs;
	cmd_ht[idx].cmd.function = function;
	cmd_ht[idx].occupied = 1;
}

static void
init_commands(void)
{
	cmd_ht_insert("!",	1,	cmd_not);
	cmd_ht_insert("!=",	2,	cmd_ne);
	cmd_ht_insert("%",	2,	cmd_mod);
	cmd_ht_insert("&",	2,	cmd_bitand);
	cmd_ht_insert("&&",	2,	cmd_and);
	cmd_ht_insert("*",	2,	cmd_mul);
	cmd_ht_insert("+",	2,	cmd_add);
	cmd_ht_insert("++",	1,	cmd_inc);
	cmd_ht_insert("-",	2,	cmd_sub);
	cmd_ht_insert("--",	1,	cmd_dec);
	cmd_ht_insert("/",	2,	cmd_div);
	cmd_ht_insert("<",	2,	cmd_lt);
	cmd_ht_insert("<<",	2,	cmd_bitshl);
	cmd_ht_insert("<=",	2,	cmd_le);
	cmd_ht_insert("==",	2,	cmd_eq);
	cmd_ht_insert(">",	2,	cmd_gt);
	cmd_ht_insert(">=",	2,	cmd_ge);
	cmd_ht_insert(">>",	2,	cmd_bitshr);
	cmd_ht_insert("^",	2,	cmd_bitxor);
	cmd_ht_insert("abs",	1,	cmd_abs);
	cmd_ht_insert("acos",	1,	cmd_acos);
	cmd_ht_insert("asin",	1,	cmd_asin);
	cmd_ht_insert("atan",	1,	cmd_atan);
	cmd_ht_insert("ceil",	1,	cmd_ceil);
	cmd_ht_insert("cos",	1,	cmd_cos);
	cmd_ht_insert("cosh",	1,	cmd_cosh);
	cmd_ht_insert("depth",	0,	cmd_depth);
	cmd_ht_insert("drop",	1,	cmd_drop);
	cmd_ht_insert("dropn",	-1,	cmd_dropn);
	cmd_ht_insert("dup",	1,	cmd_dup);
	cmd_ht_insert("dupn",	-1,	cmd_dupn);
	cmd_ht_insert("e",	0,	cmd_e);
	cmd_ht_insert("exp",	1,	cmd_exp);
	cmd_ht_insert("fact",	1,	cmd_fact);
	cmd_ht_insert("floor",	1,	cmd_floor);
	cmd_ht_insert("fp",	1,	cmd_fp);
	cmd_ht_insert("getbase",0,	cmd_getbase);
	cmd_ht_insert("help",	0,	cmd_help);
	cmd_ht_insert("hnl",	1,	cmd_htonl);
	cmd_ht_insert("hns",	1,	cmd_htons);
	cmd_ht_insert("ip",	1,	cmd_ip);
	cmd_ht_insert("ipaddr",1,	cmd_ipaddr);
	cmd_ht_insert("ln",	1,	cmd_ln);
	cmd_ht_insert("log",	1,	cmd_log);
	cmd_ht_insert("max",	2,	cmd_max);
	cmd_ht_insert("min",	2,	cmd_min);
	cmd_ht_insert("nhl",	1,	cmd_ntohl);
	cmd_ht_insert("nhs",	1,	cmd_ntohs);
	cmd_ht_insert("pad",	1,	cmd_pad);
	cmd_ht_insert("pi",	0,	cmd_pi);
	cmd_ht_insert("pick",	-1,	cmd_pick_roll);
	cmd_ht_insert("pow",	2,	cmd_pow);
	cmd_ht_insert("quit",	0,	cmd_quit);
	cmd_ht_insert("rand",	0,	cmd_rand);
	cmd_ht_insert("repeat",1,	cmd_repeat);
	cmd_ht_insert("roll",	-1,	cmd_pick_roll);
	cmd_ht_insert("rolld",	-1,	cmd_rolld);
	cmd_ht_insert("setbase",1,	cmd_setbase);
	cmd_ht_insert("sign",	1,	cmd_sign);
	cmd_ht_insert("sin",	1,	cmd_sin);
	cmd_ht_insert("sinh",	1,	cmd_sinh);
	cmd_ht_insert("sqrt",	1,	cmd_sqrt);
	cmd_ht_insert("stack",	0,	cmd_stack);
	cmd_ht_insert("swap",	2,	cmd_swap);
	cmd_ht_insert("tanh",	1,	cmd_tanh);
	cmd_ht_insert("version",0,	cmd_version);
	cmd_ht_insert("|",	2,	cmd_bitor);
	cmd_ht_insert("||",	2,	cmd_or);
	cmd_ht_insert("~",	1,	cmd_bitcmpl);
}

static void
cmd_help(void)
{
	size_t x, count;

	puts("\nStandard commands:");
	for (x = 0, count = 0; x < CMD_HT_SIZE; x++) {
		if (!cmd_ht[x].occupied)
			continue;
		printf("%8s", cmd_ht[x].cmd.name);
		if (count++ % 9 == 8)
			putchar('\n');
	}
	if (count % 9)
		puts("\n");
	else
		putchar('\n');

	puts("Macros:");
	for (x = 0, count = 0; x < MACRO_HT_SIZE; x++) {
		if (!macro_ht[x].occupied || macro_ht[x].name[0] == '$')
			continue;
		printf("%8s", macro_ht[x].name);
		if (count++ % 9 == 8)
			putchar('\n');
	}
	if (count % 9)
		puts("\n");
	else
		putchar('\n');
}

void
addcommand(struct command *c)
{
	cmd_ht_insert(c->name, c->numargs, c->function);
}

struct command *
findcmd(const char *cmd)
{
	unsigned idx;
	thiscmd = cmd;
	idx = djb2(cmd) & (CMD_HT_SIZE - 1);
	while (cmd_ht[idx].occupied) {
		if (strcmp(cmd_ht[idx].cmd.name, cmd) == 0)
			return &cmd_ht[idx].cmd;
		idx = (idx + 1) & (CMD_HT_SIZE - 1);
	}
	return NULL;
}

void
completion(const char *buf, linenoiseCompletions *lc)
{
	size_t x;
	char tmp[1000];
	const char *ptr = buf;
	size_t len = strlen(buf);
	if (!len)
		return;

	if (strrchr(buf, ' ')) {
		ptr = strrchr(buf, ' ') + 1;
		len = strlen(ptr);
	}

	for (x = 0; x < CMD_HT_SIZE; x++) {
		if (cmd_ht[x].occupied && !strncmp(ptr, cmd_ht[x].cmd.name, len)) {
			snprintf(tmp, sizeof(tmp), "%.*s%s", (int)(strlen(buf) - strlen(ptr)), buf, cmd_ht[x].cmd.name);
			linenoiseAddCompletion(lc, tmp);
		}
	}

	for (x = 0; x < MACRO_HT_SIZE; x++) {
		if (!macro_ht[x].occupied || macro_ht[x].name[0] == '$')
			continue;
		if (!strncmp(ptr, macro_ht[x].name, len)) {
			snprintf(tmp, sizeof(tmp), "%.*s%s", (int)(strlen(buf) - strlen(ptr)), buf, macro_ht[x].name);
			linenoiseAddCompletion(lc, tmp);
		}
	}
}

