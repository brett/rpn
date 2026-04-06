#include "linenoise.h"

#define VERSION     0.69

#define MAXSIZE     10
#define DEFBASE     10
#define BASECHAR    '#'

enum rpn_err {
    ERR_DIVBYZERO,
    ERR_DOMAIN,
    ERR_UNKNOWNCMD,
    ERR_ARGC,
    ERR_RANGE
};

#define STACK_INIT_CAP  64

struct metastack {
    double *data;
    size_t d;
    size_t cap;
    struct metastack *n;
};

struct rpn_state {
    struct metastack *M;
    int base, stop, stackmode, padcount;
    int pending_repeat;
};

extern struct rpn_state S;

struct command {
    const char *name;
    long numargs;
    void (*function)(void);
};

struct macro {
    char *name;
    char *operation;
    int occupied;
};

void addcommand(struct command *c);
void completion(const char *, linenoiseCompletions *);
void error(enum rpn_err);
struct command *findcmd(const char *);
char *findmacro(const char *);
void init_macros(void);
double *topptr(void);
double *nthptr(unsigned n);
double popnum(void);
void removenth(unsigned n);
void pushnum(double);
