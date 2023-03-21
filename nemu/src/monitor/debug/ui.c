#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_p(char* args){
    bool success = false;
    int val = expr(args, &success);
    if(success){
        printf("result %d\n",val);
    } else{
        printf("expr error");
    }
    return 0;
}

static int cmd_si(char* args){
    uint64_t n = 1;
    if(args != NULL){
        int si_suc = sscanf(args,"%llu",&n);
        if(si_suc <= 0 ){
            printf("args error in cmd_si\n");
            return 0;
        }
    }
    cpu_exec(n);
    return 0;
}

static int cmd_d(char* args){
    //删 NO 监视点
    int NO;
    int NO_ret = sscanf(args, "%d", &NO);

    if(NO_ret<=0){
        printf("args wp_si_error\n");
        return 0;
    }

    free_wp(NO);

    return 0;
}

static int cmd_w(char* args){
    //set监视点
    new_wp(args);
    return 0;
}

//先声明
static int cmd_x(char* args);
static int cmd_help(char *args);
static int cmd_info(char* args);



static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  //Add more commands
  { "si", "execute [N] instructions", cmd_si},
  { "info", "[-r] reg info, [-w] watchpoint info", cmd_info},
  { "p","expr_val",cmd_p},
  { "x", "[N] [expr] memory ", cmd_x},
  { "w", "[expr] watchpoint", cmd_w},
  { "d", "[N] delete the N watchpoint", cmd_d}

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_x(char* args){
    if(args == NULL){
        printf("too few parameter! \n");
        return 0;
    }

    char *arg = strtok(args," ");
    if(arg == NULL){
        printf("too few parameter! \n");
        return 0;
    }
    int num ;
    sscanf(arg, "%d", &num);
    char *expr_args = strtok(NULL," ");
    if(expr_args == NULL){       // 找不到地址参数
        printf("too few parameter! \n");
        return 0;
    }
    if(strtok(NULL," ")!=NULL){  // 还能切出来参数
        printf("too many parameter! \n");
        return 0;
    }

    bool success = true;
    vaddr_t addr = expr(expr_args , &success);
    if (success!=true){
        printf("ERRO!!\n");
        return 1;
    }

    printf("memory:\n");
    for(int i = 0 ; i < num ; i++){

        printf("0x%08x:  ", addr + i*4 );
        for(int j =0 ; j < 4 ; j++){
            printf("0x%02x " , vaddr_read(addr + i * 4 + j,1 ));
        }
        printf("\n");
    }
    return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
