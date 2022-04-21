/****************************************************************************
*                PRÀCTICA 2 SISTEMES OPERATIUS 21/22                        *
*                              NIVEL B                                      *
*                                                                           *
*                   Autors:                                                 *
*                 - Mohamed Rida Chellak El Ouahabi                         *
*                 - Miquel Robles Mclean                                    *
*                 - Soufyane Youbi                                          *
*****************************************************************************/

/****************************************************************************
*              DEFINES, INCLUDES, VARIABLES GLOBALS I ESTÀTIQUES            *
*****************************************************************************/

#define _POSIX_C_SOURCE 200112L

#define DEBUGN1 1 //parse_args()
#define DEBUGN3 1 //execute_line()

// si no vale 1 el prompt será solo el carácter de PROMPT
#define PROMPT_PERSONAL 1 

#define RESET_FORMATO "\x1b[0m"
#define NEGRO_T "\x1b[30m"
#define NEGRO_F "\x1b[40m"
#define ROJO_T "\x1b[31m"
#define VERDE_T "\x1b[32m"
#define AMARILLO_T "\x1b[33m"
#define AZUL_T "\x1b[34m"
#define MAGENTA_T "\x1b[35m"
#define CYAN_T "\x1b[36m"
#define BLANCO_T "\x1b[37m"
#define NEGRITA "\x1b[1m"

#define COMMAND_LINE_SIZE 1024
#define ARGS_SIZE 64
#define N_JOBS 24 // cantidad de trabajos permitidos

char const PROMPT = '$';

#include <errno.h>  //errno
#include <stdio.h>  //printf(), fflush(), fgets(), stdout, stdin, stderr...
#include <stdlib.h> //setenv(), getenv()
#include <string.h> //strcmp(), strtok(), strerror()
#include <unistd.h> //NULL, getcwd(), chdir()
#include <sys/types.h> //pid_t
#include <sys/wait.h>  //wait()

int check_internal(char **args);
int internal_cd(char **args);
int internal_export(char **args);
int internal_source(char **args);
int internal_jobs();
int internal_bg(char **args);
int internal_fg(char **args);
char *read_line(char *line);
int parse_args(char **args, char *line);
int execute_line(char *line);

static int pidshell;

//variable global para guardar el nombre del minishell
static char mi_shell[COMMAND_LINE_SIZE];

struct info_process {
	pid_t pid;
	char status;
	char cmd[COMMAND_LINE_SIZE];
};

//Tabla de procesos. La posición 0 será para el foreground
static struct info_process jobs_list[N_JOBS]; 


/****************************************************************************
*                            FUNCIONS i MÈTODES                             *
*****************************************************************************/

void imprimir_prompt();

int check_internal(char **args) {
    if (!strcmp(args[0], "cd"))
        return internal_cd(args);
    if (!strcmp(args[0], "export"))
        return internal_export(args);
    if (!strcmp(args[0], "source"))
        return internal_source(args);
    if (!strcmp(args[0], "jobs"))
        return internal_jobs(args);
    if (!strcmp(args[0], "bg"))
        return internal_bg(args);
    if (!strcmp(args[0], "fg"))
        return internal_fg(args);
    if (!strcmp(args[0], "exit"))
        exit(0);
    return 0; // no es un comando interno
}

int internal_cd(char **args) {
    printf("[internal_cd()→ comando interno no implementado]\n");
    return 1;
} 

int internal_export(char **args) {
    printf("[internal_export()→ comando interno no implementado]\n");
    return 1;
}

int internal_source(char **args) {
    printf("[internal_source()→ comando interno no implementado]\n");
    return 1;
}

int internal_jobs(char **args) {
    #if DEBUGN1 
        printf("[internal_jobs()→ Esta función mostrará el PID de los procesos que no estén en foreground]\n");
    #endif
    return 1;
}

int internal_fg(char **args) {
    #if DEBUGN1 
        printf("[internal_fg()→ Esta función enviará un trabajo detenido al foreground reactivando su ejecución, o uno del background al foreground ]\n");
    #endif
    return 1;
}

int internal_bg(char **args) {
    #if DEBUGN1 
        printf("[internal_bg()→ Esta función reactivará un proceso detenido para que siga ejecutándose pero en segundo plano]\n");
    #endif
    return 1;
}

void imprimir_prompt() {
#if PROMPT_PERSONAL == 1
    printf(NEGRITA ROJO_T "%s" BLANCO_T ":", getenv("USER"));
    printf(AZUL_T "MINISHELL" BLANCO_T "%c " RESET_FORMATO, PROMPT);
#else
    printf("%c ", PROMPT);

#endif
    fflush(stdout);
    return;
}

void reaper(int signum) {
  signal(SIGCHLD, reaper);
  pid_t ended;
  int status;
  while ((ended = waitpid(-1, &status, WNOHANG)) > 0) {
      if (jobs_list[0].pid == ended) {  //Foreground
        jobs_list[0].pid = 0;
        jobs_list[0].status = 'F';
        if(status!=0){
            printf("[reaper()→ Proceso hijo %d (%s) finalizado por señal: %d]\n",
            ended, jobs_list[0].cmd, status);
        }else{ //Background
            printf("[reaper()→ Proceso hijo %d (%s) finalizado con exit code: %d]\n",
            ended, jobs_list[0].cmd, status);
        }
      }
  }
}

void ctrlc(int signum) {
  signal(SIGINT, ctrlc);
  printf("\n[ctrlc()→ Soy el proceso con PID %d, el proceso en foreground es %d "
         "(%s)]\n", getpid(), jobs_list[0].pid, jobs_list[0].cmd);

  if (jobs_list[0].pid > 0) { // Si está a foreground

    if ( strcmp(jobs_list[0].cmd, mi_shell) != 0) {
      printf("[ctrlc()→ Señal %i enviada a %d (%s) por %d]\n",SIGTERM,
              jobs_list[0].pid, jobs_list[0].cmd, pidshell);
      kill(jobs_list[0].pid, SIGTERM);

    } else printf("[ctrlc()→ Señal %i no enviada debido a que el proceso en "
                  "foreground es el shell]\n",SIGTERM);

  } else printf("[ctrlc()→ Señal %i no enviada debido a que no hay "
                "proceso en foreground]\n",SIGTERM);
}


int parse_args(char **args, char *line) {
    int i = 0;

    args[i] = strtok(line, " \t\n\r");
    #if DEBUGN1 
        //printf("[parse_args()→ token %i: %s]\n", i, args[i]);
    #endif
    while (args[i] && args[i][0] != '#') { // args[i]!= NULL && *args[i]!='#'
        i++;
        args[i] = strtok(NULL, " \t\n\r");
        #if DEBUGN1 
            //printf("[parse_args()→ token %i: %s]\n", i, args[i]);
        #endif
    }
    if (args[i]) {
        args[i] = NULL; // por si el último token es el símbolo comentario
        #if DEBUGN1 
            //printf("[parse_args()→ token %i corregido: %s]\n", i, args[i]);
        #endif
    }
    return i;
}

int execute_line(char *line) {
    char *args[ARGS_SIZE];
    pid_t pid, status;
    char command_line[COMMAND_LINE_SIZE];

    memset(command_line, '\0', sizeof(command_line)); 
    strcpy(command_line, line);

    if (parse_args(args, line) > 0){

        if (check_internal(args) == 0) { // Comando interno?
            pid_t pid; 
            pid = fork();
            if (pid == 0) { //Hijo
                signal(SIGCHLD, SIG_DFL);
                signal(SIGINT, SIG_IGN);
                if (execvp(args[0], args) < 0){
                    fprintf(stderr,"%s: no se encontró la orden \n",args[0]);
                    exit(EXIT_FAILURE);
                }
            } else if (pid > 0) { // Padre
                pidshell = getpid();
                printf("[execute_line()→ PID padre: %d (%s)]\n",pidshell,mi_shell);
                printf("[execute_line()→ PID hijo: %d (%s)]\n", pid,command_line);
                jobs_list[0].pid = pid;
                jobs_list[0].status = 'E';
                strcpy(jobs_list[0].cmd, command_line);
                while (jobs_list[0].pid != 0) {
                    pause();
                } 
            }else { //PID <0: error
                perror("fork");
                exit(EXIT_FAILURE);
            }
        }
        return 1;
    }
    return 0;
}

char *read_line(char *line) {
  
    imprimir_prompt();
    char *ptr=fgets(line, COMMAND_LINE_SIZE, stdin); // leer linea
    if (ptr) {
        // ELiminamos el salto de línea (ASCII 10) sustituyéndolo por el \0
        char *pos = strchr(line, 10);
        if (pos != NULL){
            *pos = '\0';
        } 
	}  else {   //ptr==NULL por error o eof
        printf("\r");
        if (feof(stdin)) { //se ha pulsado Ctrl+D
            fprintf(stderr,"Bye bye\n");
            exit(0);
        }   
    }
    return ptr;
}


int main(int argc, char *argv[]) {
    signal(SIGCHLD, reaper);
    signal(SIGINT, ctrlc);
    strcpy(mi_shell,argv[0]);
    char line[COMMAND_LINE_SIZE];
    memset(line, 0, COMMAND_LINE_SIZE);
    while (1) {
        if (read_line(line)) { // !=NULL
            execute_line(line);
        }
    }
    return 0;
}






