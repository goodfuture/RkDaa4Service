/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.0
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkfml.c
 * Created At		: 2013-09-23 14:29
 * Last Modified	: 2013-09-27 15:48
 * Description		: 
 * History			: 
*******************************************************************************/

#include "rkfml.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define OPTR_TABLE_SIZE 7

/* operator table */
static const char optrTbl[OPTR_TABLE_SIZE] = { '+', '-', '*', '/', '(', ')', '^' };

/* operator priority table */
static const char optrPriTbl[OPTR_TABLE_SIZE][OPTR_TABLE_SIZE] =
{
	/* '+' '-' '*' '/' '(' ')' '^' */
	{ '<', '<', '>', '>', '>', '<', '>' },  /*'+'*/  
	{ '<', '<', '>', '>', '>', '<', '>' },  /*'-'*/  
	{ '<', '<', '<', '<', '>', '<', '>' },  /*'*'*/  
	{ '<', '<', '<', '<', '>', '<', '>' },  /*'/'*/  
	{ '>', '>', '>', '>', '>', '=', '>' },  /*'('*/  
	{ '<', '<', '<', '<', '=', '<', '<' },  /*')'*/  
	{ '<', '<', '<', '<', '>', '<', '<' },  /*'^'*/ 
};

int rkFmlStackInit(struct rkFmlStack *stack)
{
	stack->optrTop = 0;
	stack->opndTop = 0;

	return 0;
}

int rkFmlStackPush(struct rkFmlStack *stack, STACK_TYPE_T type, const void *val)
{
	if (rkFmlStackIsFull(stack, type)) {
		return -1;
	}

	if (type == STACKOPTR) {
		stack->optr[stack->optrTop] = *(char *)val;
		stack->optrTop++;
	} else {
		stack->opnd[stack->opndTop] = *(float *)val;
		stack->opndTop++;
	}

	return 0;
}

int rkFmlStackPop(struct rkFmlStack *stack, STACK_TYPE_T type, void *val)
{
	if (rkFmlStackIsEmpty(stack, type)) {
		return -1;
	}

	if (type == STACKOPTR) {
		stack->optrTop--;
		*(char *)val = stack->optr[stack->optrTop];
	} else {
		stack->opndTop--;
		*(float *)val = stack->opnd[stack->opndTop];
	}

	return 0;
}

int rkFmlStackIsEmpty(struct rkFmlStack *stack, STACK_TYPE_T type)
{
	if (type == STACKOPTR) {
		return !stack->optrTop ? TRUE : FALSE;
	} else {
		return !stack->opndTop ? TRUE : FALSE;
	}
}

int rkFmlStackIsFull(struct rkFmlStack *stack, STACK_TYPE_T type)
{
	if (type == STACKOPTR) {
		return stack->optrTop >= FML_STACK_SIZE ? TRUE : FALSE;
	} else {
		return stack->opndTop >= FML_STACK_SIZE ? TRUE : FALSE;
	}
}

int rkFmlIsOptr(const char in)
{
	int i;
	for (i = 0; i < OPTR_TABLE_SIZE; i++) {
		if (in == optrTbl[i]) {
			return TRUE;
		}
	}

	return FALSE;
}

int rkFmlIsNum(const char *buf)
{
	char *ptr = (char *)buf;

	for (; *ptr; ptr++) {
		if ((*ptr < '0' && *ptr != '.') || *ptr > '9') {
			return FALSE;
		}
	}

	return TRUE;
}

int rkFmlStackGetTopElement(struct rkFmlStack *stack, STACK_TYPE_T type, void *val)
{
	if (rkFmlStackIsEmpty(stack, type)) {
		return -1;
	}

	if (type == STACKOPTR) {
		*(char *)val = stack->optr[stack->optrTop - 1];
	} else {
		*(float *)val = stack->opnd[stack->opndTop - 1];
	}

	return 0;
}

int rkFmlGetOptrIndex(const char optr)
{
	int i;
	for (i = 0; i < OPTR_TABLE_SIZE; i++) {
		if (optrTbl[i] == optr) {
			return i;
		}
	}

	return -1;
}

char rkFmlCmpOptrPri(const char dest, const char src)
{
	int row, col;

	row = rkFmlGetOptrIndex(dest);
	col = rkFmlGetOptrIndex(src);

	if (row == -1 || col == -1) {
		return ' ';
	} else {
		return optrPriTbl[row][col];
	}
}

float rkFmlBasicOptr(float x, char optr, float y)
{
	switch(optr) {
		case '+':
			return x + y;
		case '-':
			return x - y;
		case '*':
			return x * y;
		case '/':
			return x / y;
		case '^':
			return pow(x, y);
		default:
			break;
	}

	return 0;
}

static struct rkFmlSymTbl symtbl;

int rkFmlSymTblInit()
{
	bzero(&symtbl, sizeof(struct rkFmlSymTbl));

	return 0;
}

int rkFmlUpdateSymTbl(const char *sym, float val)
{
	int i;
	//printf("sym = %s, val = %f\n", sym, val);

	if (!sym || !strlen(sym)) {
		return -1;
	}

	/* Look up and update symbol table */
	for (i = 0; i < FML_SYMBOL_TABLE_MAX_NUM; i++) {
		if (!strcmp(symtbl.sym[i], sym)) {
			symtbl.val[i] = val;
			return 0;
		}   
	}   

	/* When failed to look up, find an empty and add symble to it */
	for (i = 0; i < FML_SYMBOL_TABLE_MAX_NUM; i++) {
		if (!strlen(symtbl.sym[i])) {
			strcpy(symtbl.sym[i], sym);
			symtbl.val[i] = val;
			return 0;
		}   
	}   

	return -1; 
} 

int rkFmlLookupSymTbl(const char *sym, float *val)
{
	int i;
	if (!sym || !strlen(sym)) {
		return -1;
	}

	for (i = 0; i < FML_SYMBOL_TABLE_MAX_NUM; i++) {
		if (!strcmp(symtbl.sym[i], sym)) {
			*val = symtbl.val[i];
			return 0;
		}
	}

	return -1;
}

int rkFmlDropBlankSpace(char *buf)
{
	char *tmp, *ptr2;

	int len = strlen(buf);
	tmp = malloc(len);
	if (!tmp) {
		return -1;
	}
	bzero(tmp, len);

	ptr2 = strtok(buf, " ");
	while(ptr2) {
		strcat(tmp, ptr2);
		ptr2 = strtok(NULL, " ");
	}
	sprintf(buf, "%s", tmp);
	free(tmp);

	return 0;
}

int rkFmlConvExpr(const char *in, char *out, const char *code)
{
	char var[16] = { 0 }, tmp[16] = { 0 };
	char *ptr = (char *)in;
	float value;

	sprintf(tmp, "%s%s", FML_VAR_PREFIX, code);
	rkFmlDropBlankSpace(ptr);

	for (; *ptr != '\0'; ptr++) {
		if (rkFmlIsOptr(*ptr)) {
			out[strlen(out)] = *ptr;
			if (*ptr == '(' && *(ptr + 1) == '-') {
				out[strlen(out)] = '0';
			}
			continue;
		} else {
			var[strlen(var)] = *ptr;
			if(rkFmlIsOptr(*(ptr + 1)) || *(ptr + 1) == '\0'){
				//printf("var = %s, tmp = %s\n", var, tmp);
				if (rkFmlIsNum(var) == TRUE) {
					value = atof(var);
				} else if (!strcmp(tmp, var)) {
					rkFmlLookupSymTbl(FML_VAR_PREFIX, &value);
					//printf("tmp = %s, value = %f\n", tmp, value);
				} else if (rkFmlLookupSymTbl(var, &value)) {
#if 0
					fprintf(stderr, "%s : can't find symbol \'%s\'.\n", __func__, var);
#endif
					return -1;
				}
				sprintf(out, "%s%g", out, value);
				bzero(var, sizeof(var));
			}
		}
	}

	return 0;
}

float rkFmlEvaluateExpr(const char *expr)
{
	struct rkFmlStack fmlStack;
	float rest, x, y;
	char *ptr, tmp[64], val;
	int ret;

	bzero(tmp, sizeof(tmp));
	rkFmlStackInit(&fmlStack);
	for (ptr = (char *)expr; *ptr != '\0'; ptr++) {
		if (rkFmlIsOptr(*ptr)) {
			ret = rkFmlStackGetTopElement(&fmlStack, STACKOPTR, &val);
			if (ret == -1) {
				rkFmlStackPush(&fmlStack, STACKOPTR, ptr);
				continue;
			}

			switch(rkFmlCmpOptrPri(val, *ptr)) {
				case '<':
					rkFmlStackPop(&fmlStack, STACKOPTR, &val);
					rkFmlStackPop(&fmlStack, STACKOPND, &y);
					rkFmlStackPop(&fmlStack, STACKOPND, &x);
					rest = rkFmlBasicOptr(x, val, y);
					rkFmlStackPush(&fmlStack, STACKOPND, &rest);
					ptr--;
					break;
				case '=':
					rkFmlStackPop(&fmlStack, STACKOPTR, &val);
					break;
				case '>':
				default:
					rkFmlStackPush(&fmlStack, STACKOPTR, ptr);
					break;
			}
		} else {
			tmp[strlen(tmp)] = *ptr;
			if (rkFmlIsOptr(*(ptr + 1)) || *(ptr + 1) == '\0') {
				rest = atof(tmp);
				rkFmlStackPush(&fmlStack, STACKOPND, &rest);
				bzero(tmp, sizeof(tmp));
			}
		}
	}

	while(!rkFmlStackPop(&fmlStack, STACKOPTR, &val)) {
		rkFmlStackPop(&fmlStack, STACKOPND, &y);
		rkFmlStackPop(&fmlStack, STACKOPND, &x);
		rest = rkFmlBasicOptr(x, val, y);
		rkFmlStackPush(&fmlStack, STACKOPND, &rest);
	}

	rkFmlStackPop(&fmlStack, STACKOPND, &rest);

	return rest;
}
