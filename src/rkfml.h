#ifndef _RK_FORMULA_H_
#define _RK_FORMULA_H_

#include "rktype.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif 

#define FML_STACK_SIZE 100

#define FML_VAR_PREFIX "x"

typedef enum {
	STACKOPTR = 0, /* operator stack */
	STACKOPND,	/* operand stack */
} STACK_TYPE_T;

/* operator & operand stack */
typedef struct rkFmlStack {
	uint8_t optrTop; /* stack top of operator */
	uint8_t opndTop; /* stack top of operand */
	char optr[FML_STACK_SIZE]; /* used to store operator */
	float opnd[FML_STACK_SIZE]; /* used to store operand */
} rkFmlStack_t;

#define FML_SYMBOL_TABLE_MAX_NUM	64
/* symbol & value converter */
typedef struct rkFmlSymTbl{
	char sym[FML_SYMBOL_TABLE_MAX_NUM][8]; /* symbol */
	float val[FML_SYMBOL_TABLE_MAX_NUM]; /* value */
} rkFmlSymTbl_t;

int rkFmlSymTblInit();
int rkFmlStackInit(struct rkFmlStack *stack);
int rkFmlStackPush(struct rkFmlStack *stack, STACK_TYPE_T type, const void *val);
int rkFmlStackPop(struct rkFmlStack *stack, STACK_TYPE_T type, void *val);
int rkFmlStackIsEmpty(struct rkFmlStack *stack, STACK_TYPE_T type);
int rkFmlStackIsFull(struct rkFmlStack *stack, STACK_TYPE_T type);
int rkFmlStackGetTopElement(struct rkFmlStack *stack, STACK_TYPE_T type, void *val);

int rkFmlUpdateSymTbl(const char *sym, float val);
int rkFmlConvExpr(const char *in, char *out, const char *code);
float rkFmlEvaluateExpr(const char *expr);
int rkFmlIsOptr(const char in);
int rkFmlIsNum(const char *buf);
char rkFmlCmpOptrPri(const char dest, const char src);
int rkFmlGetOptrIndex(const char optr);
float rkFmlBasicOptr(float x, char optr, float y);
int rkFmlLookupSymTbl(const char *sym, float *val);
int rkFmlDropBlankSpace(char *buf);

#endif /* _RK_FORMULA_H_ */
