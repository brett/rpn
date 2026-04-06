#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <stdint.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include "rpn.h"

struct rpn_state S = { .base = DEFBASE };

static void process(char *);

double *
topptr(void) {
	return &S.M->data[S.M->d - 1];
}

double *
nthptr(unsigned n) {
	return &S.M->data[S.M->d - 1 - n];
}

static void
grow(void)
{
	if (S.M->d >= S.M->cap) {
		S.M->cap *= 2;
		S.M->data = realloc(S.M->data, S.M->cap * sizeof(double));
		if (S.M->data == NULL) {
			perror("Error: realloc");
			exit(1);
		}
	}
}

void
pushnum(double num)
{
	grow();
	S.M->data[S.M->d++] = num;
}

double
popnum(void)
{
	return S.M->data[--S.M->d];
}

void
removenth(unsigned n)
{
	size_t idx = S.M->d - 1 - n;
	if (n > 0)
		memmove(&S.M->data[idx], &S.M->data[idx + 1], n * sizeof(double));
	S.M->d--;
}

#define CONVERTMAX 99
static char *
convertbase(uint64_t num, int base, char *converted)
{
	static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	int i;

	converted[CONVERTMAX] = '\0';
	for (i=CONVERTMAX - 1; num && i; i--) {
		converted[i] = digits[num % base];
		num = num / base;
	}
	while (S.padcount > CONVERTMAX - 1 - i ) {
		converted[i--] = digits[0];
	}
	return &converted[i+1];
}

static char *
format_stack(const char *sep, const char *prompt)
{
	char *str, buf[100], cvt[CONVERTMAX + 1];
	int len;
	size_t i;

	for (len = 0, i = 0; i < S.M->d; i++) {
		if (S.base == 10) {
			snprintf(buf, sizeof(buf), "%.12g%s", S.M->data[i], sep);
			len += strlen(buf);
		} else
			len += strlen(convertbase(S.M->data[i], S.base, cvt)) + strlen(sep);
	}
	str = calloc(1, len + strlen(prompt) + 1);
	if (str == NULL) {
		perror("Error: calloc");
		exit(1);
	}
	int total = len + strlen(prompt) + 1;
	for (len = 0, i = 0; i < S.M->d; i++) {
		if (S.base == 10) {
			len += snprintf(str + len, total - len, "%.12g%s", S.M->data[i], sep);
		} else
			len += snprintf(str + len, total - len, "%s%s", convertbase(S.M->data[i], S.base, cvt), sep);
	}
	snprintf(str + len, total - len, "%s", prompt);
	return str;
}

static void
eval(char *cmd)
{
	char *operation;
	static int doingmacro = 0;
	long numargs;
	static char prevcmd[MAXSIZE] = { '\0' };
	struct command *cmdptr;

	if (!doingmacro) {
		if (strcmp(cmd, ".") == 0 && prevcmd[0])
			cmd = prevcmd;
		else {
			strncpy(prevcmd, cmd, MAXSIZE-1);
			prevcmd[MAXSIZE-1] = '\0';
		}
	}

	if ((operation = findmacro(cmd)) != NULL) {
		doingmacro = 1;
		process(operation);
		doingmacro = 0;
	} else if ((cmdptr = findcmd(cmd)) != NULL) {
		if (cmdptr->numargs == -1) {
			if (S.M->d == 0)
				numargs = 1;
			else if (*topptr() < 0)
				numargs = -1;
			else
				numargs = *topptr() + 1;
		} else
			numargs = cmdptr->numargs;
		if (numargs == -1 || (long)S.M->d < numargs)
			error(ERR_ARGC);
		else
			cmdptr->function();
	} else
		error(ERR_UNKNOWNCMD);
}

#define isnum(s) (isdigit(s[0])						\
		  || ((s[0] == '-' || s[0] == '.') && isdigit(s[1]))	\
		  || (s[0] == '-' && s[1] == '.' && isdigit(s[2])))
#define isnotfloat(s) ((s[0] == '0' && s[1] != '.')			\
		       || (s[0] == '-' && s[1] == '0' && s[2] != '.'))

static void
process(char *str)
{
	int x;
	char *suffix, word[100];
	char *tmp, *tmp2;

	while (*str != '\0') {
		while (isspace(*str))
			str++;
		if (*str == '\0')
			break;
		for (x = 0; *str != '\0' && !isspace(*str) && x < (int)sizeof(word) - 1; x++)
			word[x] = *str++;
		word[x] = '\0';
		if ((suffix = strchr(word, BASECHAR)) != NULL) {
			*suffix++ = '\0';
			tmp = tmp2 = word;
			while (*tmp2 != '\0') {
				if (*tmp2 == ',')
					tmp2++;
				else
					*tmp++ = *tmp2++;
			}
			*tmp++ = *tmp2++;
			errno = 0;
			if (word[0] == '-')
				pushnum(strtol(word, NULL, atoi(suffix)));
			else
				pushnum(strtoul(word, NULL, atoi(suffix)));
			if (errno == ERANGE)
				error(ERR_RANGE);
		} else if (isnum(word)) {
			tmp = tmp2 = word;
			while (*tmp2 != '\0') {
				if (*tmp2 == ',')
					tmp2++;
				else
					*tmp++ = *tmp2++;
			}
			*tmp++ = *tmp2++;
			errno = 0;
			if (isnotfloat(word)) {
				if (word[0] == '-')
					pushnum(strtol(word, &suffix, 0));
				else
					pushnum(strtoul(word, &suffix, 0));
			} else
				pushnum(strtod(word, &suffix));
			if (errno == ERANGE)
				error(ERR_RANGE);
			else if (*suffix != '\0')
				process(suffix);
		} else {
			int reps = 1;
			if (S.pending_repeat > 0) {
				reps = S.pending_repeat;
				S.pending_repeat = 0;
			}
			for (x = reps; x > 0; x--) {
				eval(word);
				if (S.stop) {
					S.stop = 0;
					return;
				}
			}
		}
	}
}

static struct metastack *
newstack(void) {
	struct metastack *m = malloc(sizeof *m);
	if (m == NULL) {
		perror("Error: malloc");
		exit(1);
	}
	m->data = malloc(STACK_INIT_CAP * sizeof(double));
	if (m->data == NULL) {
		perror("Error: malloc");
		exit(1);
	}
	m->d = 0;
	m->cap = STACK_INIT_CAP;
	m->n = NULL;
	return m;
}

static void
pushstack(void) {
	double val = 0;
	int has_val = S.M->d > 0;
	if (has_val)
		val = *topptr();
	struct metastack *m = newstack();
	m->n = S.M;
	S.M = m;
	if (has_val)
		pushnum(val);
}

static void
freestack(struct metastack *m) {
	free(m->data);
	free(m);
}

static void
popstack(void) {
	double val = 0;
	int has_val = S.M->d > 0;
	if (has_val)
		val = *topptr();
	if (S.M->n) {
		struct metastack *m = S.M;
		S.M = S.M->n;
		if (has_val)
			pushnum(val);
		freestack(m);
	}
}

static void
init(void) {
	struct command pushs = { "pushs", 0, pushstack },
			pops = { "pops", 0, popstack };
	addcommand(&pushs);
	addcommand(&pops);
	srandom(time(NULL));
	init_macros();
	S.M = newstack();
}

int
main(int argc, char *argv[])
{
	char *line, *prompt;
	char histfile[PATH_MAX] = "";

	init();

	if (argc > 1) {
		int x;
		for (x = 1; x < argc; x++)
			process(argv[x]);
		puts(format_stack(" ", ""));
		return 0;
	}

	if (!isatty(0)) {
		char buf[1000];
		while (fgets(buf, sizeof buf, stdin) != NULL)
			process(buf);
		puts(format_stack(" ", ""));
		return 0;
	}

	if (getenv("HOME")) {
		snprintf(histfile, sizeof histfile, "%s/.rpn_history", getenv("HOME"));
		linenoiseHistoryLoad(histfile);
	}

	linenoiseSetCompletionCallback(completion);
	linenoiseSetMultiLine(1);

	prompt = strdup("> ");
	while ((line = linenoise(prompt)) != NULL) {
		if (line[0] != '\0') {
			if (histfile[0]) {
				linenoiseHistoryAdd(line);
				linenoiseHistorySave(histfile);
			}
			process(line);
			free(line);
			free(prompt);
			prompt = format_stack(" ", "> ");
		}
	}

	return 0;
}
