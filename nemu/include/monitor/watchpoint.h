#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
    int NO;
    struct watchpoint *next;

    /* TODO: Add more members if necessary */
    int old_val; //旧的值
    char expr[32]; //表达式
    int hitNum; //记录触发次数

} WP;


bool free_wp(int num);
bool new_wp(char *arg);
void printAllWp();
bool check_wp();

#endif
