#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

static int next_wp; //下一个wp.NO

void init_wp_pool() {
    int i;
    for (i = 0; i < NR_WP; i ++) {
        wp_pool[i].NO = i;
        wp_pool[i].next = &wp_pool[i + 1];
        wp_pool[i].old_val = 0;
        wp_pool[i].hitNum = 0; //？
    }
    wp_pool[NR_WP - 1].next = NULL;

    head = NULL;
    free_ = wp_pool;
    
    next_wp = 0;
}

/* DONE: Implement the functionality of watchpoint */

bool new_wp(char *args) { //分配新的wp
    if(free_ == NULL) {
        printf("wp is full");
        assert(0);
    }
    //取出一个free_wp
    WP* alloc_wp = free_;
    free_ = free_ -> next;
    alloc_wp -> next = NULL;
    
    //alloc_wp初始化
    alloc_wp -> NO = next_wp; next_wp++; //index后移
   
    alloc_wp -> hitNum = 0;

    strcpy(alloc_wp -> expr, args);//监视的表达式
    bool succ;
    alloc_wp -> old_val = expr(args, &succ); //表达式求值
    if(succ == false) {
        printf("new_wp_expression error\n");
        free_wp(alloc_wp -> NO);
        return false;
    }
    

    //alloc_wp插入链表
    WP *wp_t = head;
    if(wp_t == NULL) { //第一个
        head = alloc_wp;
    }
    else { //链表
        while (wp_t -> next)
        {
            wp_t = wp_t -> next;
        }
        wp_t -> next = alloc_wp;
    }
    printf("new_wp %d, old_val = %d\n", alloc_wp -> NO, alloc_wp -> old_val);
    return true;
}

//删wp
bool free_wp(int num) {
    WP *del_wp = NULL;
    if(head == NULL) {
        printf("wp is empty\n");
        return false;
    }
    bool succ = false;
    //只有一个
    if(head -> NO == num) {
        del_wp = head;
        head = head -> next;
        succ = true;
    }
    else {
        WP *wp_t = head;
        while ( wp_t -> next )
        {
            if(wp_t -> next -> NO == num) {
                del_wp = wp_t -> next;
                wp_t -> next = wp_t -> next -> next;
                succ = true;
                break;
            }
            wp_t = wp_t -> next;
        }
    }

    if(!succ){
        printf("not exists wp_%d \n",num);
        assert(0);
    }

    //加回free链表
    if(del_wp) {
        del_wp -> next = free_;
        free_ = del_wp;
        printf("delete wp_%d",num)
        return true;
    }
    return false;
}

void printAllWp(){ {
    if(!head) {
        printf("wp is empty\n");
        return;
    }
    printf("watchpoint:\n");

    WP *wp_t = head;
    while (wp_t)
    {
        printf("wp_no = %d wp_expr = %s wp_hit = %d\n",
               wp_t -> NO, wp_t -> expr, wp_t -> hitNum);
        wp_t = wp_t ->next;
    }
}

//是否触发wp
bool check_wp() {
    bool succ;
    int val;
    if(head == NULL) {
        return true;
    }
    WP *wp_t = head;
    while (wp_t)
    {
        val = expr(wp_t -> expr, &succ);
        if(val != wp_t -> old_val)
        {
            wp_t -> hitNum += 1;
            printf("wp_%d:%s changed\n", wp_t -> NO, wp_t -> expr);
            printf("Old value:%d\nNew new_value:%d\n\n", wp_t -> old_val, val);
            wp_t -> old_val = val;
            return false;
        }
        wp_t = wp_t -> next;
    }
    return true;
}


