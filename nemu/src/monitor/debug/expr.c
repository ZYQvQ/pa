#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256,
  TK_DEC,
  TK_HEX,
  TK_REG_32,
    TK_REG_16,
    TK_REG_8,
    TK_EQ,
    TK_NE,
    TK_NEG,
    TK_AND,
    TK_OR,
    TK_DEREF

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  //正则表达式
  {"0x[1-9a-fA-F][0-9a-fA-F]*", TK_HEX},
  {"[1-9][0-9]*|0",TK_DEC},
  {"\\$(eax|ecx|edx|ebx|esp|ebp|esi|edi|eip)",TK_REG_32},   // $也需要转义
  {"\\$(ax|cx|dx|bx|sp|bp|si|di)",TK_REG_16},
  {"\\$(al|cl|dl|bl|ah|ch|dh|bh)",TK_REG_8},
  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus转义
  {"==", TK_EQ},         // equal
  {"!=", TK_NE},
  {"&&",TK_AND},
  {"\\|\\|",TK_OR},

  {"\\+", '+'},
  {"-", '-'},
  {"\\*", '*'},
  {"/", '/'},
  {"\\(", '('},
  {"\\)", ')'},
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;
//根据rules正则匹配tokens
  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;//匹配的token长度

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        //不允许大于32
          if(substr_len > 32) {
              printf("substr不能超过32位");
              assert(0);
          }
//          // 不允许超过32个，应该也不会
//          if(nr_token > 32 ){
//              printf("tokens过长");
//              assert(0);
//          }
          if(rules[i].token_type == TK_NOTYPE) break; //空格不用处理
          
          tokens[nr_token].type=rules[i].token_type;
        switch (rules[i].token_type) {
            case TK_DEC:
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                tokens[nr_token].str[substr_len+1] = '\0';  
                break;
            case TK_HEX:
                strncpy(tokens[nr_token].str, substr_start + 2, substr_len - 2); //0x
                tokens[nr_token].str[substr_len+1] = '\0';  
                break;
            case TK_REG_32:
                strncpy(tokens[nr_token].str, substr_start + 1 , substr_len - 1);  // 跳过 $
                tokens[nr_token].str[substr_len] = '\0';
                break;
            case TK_REG_16:
                strncpy(tokens[nr_token].str, substr_start + 1 , substr_len - 1);  
                tokens[nr_token].str[substr_len] = '\0';
                break;
            case TK_REG_8:
                strncpy(tokens[nr_token].str, substr_start + 1 , substr_len - 1); 
                tokens[nr_token].str[substr_len] = '\0';
        }
        nr_token+=1;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


bool check_parentheses(int begin, int end){   // 括号表达式判断 && （）匹配
    if(begin > end){
        printf("check_parentheses：begin>end\n");
        assert(0);
    }else if( begin == end){
        return false;    
    }else{
        // begin、end都必（ ）
        if(!(tokens[begin].type == '(' && tokens[end].type == ')')){
            return false;
        }
        // c没有stack库
        int cnt = 0; //未匹配的左括号
        for(int i = begin; i <= end ;i++){
            if(tokens[i].type == '('){
                cnt ++;
            }else if(tokens[i].type == ')'){
                if(cnt > 0){
                    cnt--;
                }else{
                    printf("未找到对应 ”(“ \n");
                    assert(0);
                    return false;
                }
            }
        }
        if(cnt == 0){
            return true;
        }else{
            printf("未找到对应 ”)“ \n");
            assert(0);
            return false;
        }
    }

}

//最后一步运算符位置
int getDominantOp(int begin, int end) {
    int level=0;
    int pri[5]={-1, -1, -1, -1, -1};
    for(int i = begin; i < end; i++){
        if(tokens[i].type=='(') {
            level++;
        }
        else if(tokens[i].type==')') {
            level--;
        }
        else if(level == 0) {
            if(tokens[i].type == TK_AND || tokens[i].type == TK_OR) {
                pri[0] = i;
            }
            if(tokens[i].type == TK_EQ || tokens[i].type == TK_NE) {
                pri[1] = i;
            }
            if(tokens[i].type == '+' || tokens[i].type == '-') {
                pri[2] = i;
            }
            if(tokens[i].type == '*' || tokens[i].type == '/') {
                pri[3] = i;
            }
            if(tokens[i].type == TK_NEG || tokens[i].type == TK_DEREF || tokens[i].type == '!') {
                pri[4] = i;
            }
        }

    }
    for(int i = 0; i < 5; i++) {
        if(pri[i] != -1) {
            return pri[i];
        }
    }
    printf("can't get DominantOp\n");
    printf("[begin=%d,end=%d]\n",begin,end);
    assert(0);
}

//uint32_t -> int 负数
int eval(int p, int q) {
    if(p > q) {
        printf("error:p>q in eval, p = %d, q = %d\n", p, q);
        assert(0);
    }
    if(p == q) {
        int num;
        switch (tokens[p].type){
            case TK_NUMBER:
                sscanf(tokens[p].str, "%d", &num);
                return num;
            case TK_HEX:
                sscanf(tokens[p].str, "%x", &num);
                return num;
            case TK_REG:
                for(int i = 0; i < 8; i++) {
                    if(strcmp(tokens[p].str, regsl[i]) == 0) {
                        return reg_l(i);
                    }
                    if(strcmp(tokens[p].str, regsw[i]) == 0) {
                        return reg_w(i);
                    }
                    if(strcmp(tokens[p].str, regsb[i]) == 0) {
                        return reg_b(i);
                    }
                }
                if(strcmp(tokens[p].str, "eip") == 0) {
                    return cpu.eip;
                }
                else {
                    printf("error in TK_REG in eval()\n");
                    assert(0);
                }
        }
    }
    if(check_parentheses(p, q) == true) {
        return eval(p + 1, q - 1);
    }
    else {
        int op = findDominantOp(p, q);
        vaddr_t addr;
        int result;
        switch (tokens[op].type) {
            case TK_NEGATIVE: //负号
                return -eval(p + 1, q);
            case TK_DEREF: //指针求值
                addr = eval(p + 1, q);
                result = vaddr_read(addr, 4);
                printf("adddr=%u(0x%x)---->value=%d(0x%08x)\n", addr, addr, result, result);
                return result;
            case '!':
                result = eval(p + 1, q);
                if(result != 0) {
                    return 0;
                }
                else {
                    return 1;
                }
        }
        uint32_t val1 = eval(p, op - 1);
        uint32_t val2 = eval(op + 1, q);
        switch(tokens[op].type) {
            case '+':
                return val1 + val2;
            case '-':
                return val1 - val2;
            case '/':
                return val1 / val2;
            case '*':
                return val1 * val2;
            case TK_EQ:
                return val1 == val2;
            case TK_NEQ:
                return val1 != val2;
            case TK_AND:
                return val1 && val2;
            case TK_OR:
                return val1 || val2;
            default:
                assert(0);
        }
    }
    return 1;
}


int expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
}
