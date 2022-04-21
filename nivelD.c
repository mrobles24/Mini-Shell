/****************************************************************************
*                PRÀCTICA 2 SISTEMES OPERATIUS 21/22                        *
*                              NIVEL C                                      *
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
#include <sys/file.h> // file library, open()
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
static int n_pids;

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
    int contador=0;
      while (contador<n_pids)  //Imprime cada elemento
        {
         contador++;
         printf("[%i] %d     %c      %s \n",contador,jobs_list[contador].pid,
            jobs_list[contador].status,jobs_list[contador].cmd);
        }
    return 1;
}

int internal_fg(char **args) {
    if(args[1] != NULL){ //Se comprueba sintaxis
        int pos=atoi(args[1]);
        if(pos>n_pids || pos==0){ //Se comprueba si existe
            printf("fg %i: No existe ese trabajo\n",pos);
            return 1;
        }else{
            if(jobs_list[pos].status=='D'){ //Si esta detenido
                kill(jobs_list[pos].pid,SIGCONT);
                jobs_list[pos].status = 'E';
                printf("[internal_fg()→ Señal %i (SIGCONT) enviada a %d (%s)]\n",
                        SIGCONT,jobs_list[pos].pid,jobs_list[pos].cmd);
            }
            int contador=0;
            while(jobs_list[pos].cmd[contador]!=0){ //Se busca &
                if(jobs_list[pos].cmd[contador]=='&'){
                    jobs_list[pos].cmd[contador]=0;
                }
                contador++;       
            }
            jobs_list[0]=jobs_list[pos];
            jobs_list_remove(pos);
            printf("%s\n",jobs_list[0].cmd);
            while (jobs_list[0].pid > 0){ //El padre espera
                    pause();
            }
        }
    }else{
        perror("Sintaxis incorrecta");
        return 0;
    }
    return 1;
}

int internal_bg(char **args) {
    if(args[1] != NULL){ //Se comprueba sintaxis
        int pos=atoi(args[1]);
        if(pos>n_pids || pos==0){ //Se comprueba si existe
            printf("bg %i: no existe ese trabajo\n",pos);
            return 1;
        }else{
            if(jobs_list[pos].status=='E'){ // Si esta ejecutandose
                printf("bg %i: el trabajo ya está en segundo plano\n",pos);
                return 1;
            }else{
                jobs_list[pos].status='E';
                char d[2] = " &";
                strncat(jobs_list[pos].cmd,d,2);
                kill(jobs_list[pos].pid,SIGCONT);
                printf("\n[internal_bg()→ Señal %i (SIGCONT) enviada a %d (%s)]\n",
                        SIGCONT,jobs_list[pos].pid,jobs_list[pos].cmd);
                printf("[%i] %d     %c      %s \n",pos,jobs_list[pos].pid,
                jobs_list[pos].status,jobs_list[pos].cmd);
            }
            
        }
    }else{
        perror("Sintaxis incorrecta");
        return 0;
    }
    return 1;
}

int is_output_redirection (char **args){
    int n = 0;
    int file;
    while (args[n] != NULL){ 
        if (*args[n] == '>' && args[n+1] != NULL){
            file=open (args[n+1],O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
            if(file>=0){ //Si fichero correcto
                args[n]=0; //Quitamos >
                args[n+1]=0; //Ponemos NULL a la direccion de fichero
                dup2(file,1);
                close(file);
                return 1;
            }else{
                perror("file");
                return 0;
            }
            return 0;
        }
        n++;
    }
  return 0;
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

///////

int jobs_list_add(pid_t pid, char status, char *cmd){
    if(n_pids < N_JOBS-1){
		jobs_list[n_pids+1].pid = pid;
		strcpy(jobs_list[n_pids+1].cmd,cmd);
		jobs_list[n_pids+1].status = status;
		n_pids ++;
		return 0;
	}
    return -1;
}

int jobs_list_find(pid_t pid){
    int i=1;
    while (jobs_list[i].pid!=pid && (i<n_pids && i<N_JOBS)){
        i++;
    }
    if (jobs_list[i].pid==pid){
        return i;
    }
    return -1;
}

int jobs_list_remove(int pos){
    if (pos < n_pids) {
        jobs_list[pos] = jobs_list[n_pids];
    }
    n_pids--;
    return 0;
}

//////

void reaper(int signum) {
  signal(SIGCHLD, reaper);
  pid_t ended;
  int status;
  while ((ended = waitpid(-1, &status, WNOHANG)) > 0) {
      if (jobs_list[0].pid == ended) {  //Foreground
        if(status!=0){
            printf("[reaper()→ Proceso hijo %d en foreground (%s) finalizado por " 
                    "señal: %d]\n",
            ended, jobs_list[0].cmd, status);
        }else{
            printf("[reaper()→ Proceso hijo %d en foreground (%s) finalizado con "
                    "exit code: %d]\n",
            ended, jobs_list[0].cmd, status);
        }
        jobs_list[0].pid = 0;
        jobs_list[0].status = 'F';
      }else{ //Background
            int pos=jobs_list_find(ended);
            printf("\nTerminado PID %d (%s) en jobs_list[%i] con "
                    "status %i \n", ended,jobs_list[pos].cmd,pos,status);
            printf("\n[reaper()→ Proceso hijo %d en background (%s) finalizado con "
                    "exit code %d]\n", ended,jobs_list[pos].cmd,WEXITSTATUS(status));
            jobs_list_remove(pos);
        }
  }
}

void ctrlc(int signum) {
  signal(SIGINT, ctrlc);
  printf("\n[ctrlc()→ Soy el proceso con PID %d, el proceso en foreground es "
            "%d (%s)]\n", getpid(), jobs_list[0].pid, jobs_list[0].cmd);
  if (jobs_list[0].pid > 0) {  //Si està en foreground
    if ( strcmp(jobs_list[0].cmd, mi_shell) != 0) {
      printf("[ctrlc()→ Señal %i (SIGTERM) enviada a %d (%s) por %d]\n",
            SIGTERM,jobs_list[0].pid, jobs_list[0].cmd, pidshell);
      kill(jobs_list[0].pid, SIGTERM);
    } else printf("[ctrlc()→ Señal %i (SIGTERM) no enviada debido a que el "
                "proceso en foreground es el shell]\n",SIGTERM);
  } else printf("[ctrlc()→ Señal %i (SIGTERM) no enviada debido a que no hay "
                "proceso en foreground]\n",SIGTERM);
}

void ctrlz(int signum){
    signal(SIGTSTP,ctrlz);
    char mensaje[1200];
    sprintf(mensaje, "\n[ctrlz()→ Soy el proceso con PID %d, el proceso en "
            "foreground es %d (%s)]\n",getpid(),jobs_list[0].pid,
            jobs_list[0].cmd);
    write(2, mensaje, strlen(mensaje)); 
    if(jobs_list[0].pid>0){ //Si hay proceso en foreground
        if(strcmp(jobs_list[0].cmd, mi_shell) != 0){ //Si no es el minishell
            printf("[ctrlz()→ Señal %i (SIGSTOP) enviada a %d (%s) "
                    "por %d (%s)]\n",SIGSTOP,jobs_list[0].pid,jobs_list[0].cmd,
                    getpid(),mi_shell);
            kill(jobs_list[0].pid,SIGSTOP);
            jobs_list[0].status='D';
            jobs_list_add(jobs_list[0].pid,jobs_list[0].status,jobs_list[0].cmd);
            int contador=jobs_list_find(jobs_list[0].pid);
            jobs_list[0].pid=0;
            printf("\n[%i] %d     %c      %s \n",contador,jobs_list[contador].pid,
        jobs_list[contador].status,jobs_list[contador].cmd);
        }else{
            printf("[ctrlz()→ Señal %i (SIGSTOP) no enviada por %d (%s) debido a "
                    "que el proceso en foreground es el shell]\n",
                    SIGSTOP,getpid(),mi_shell);
           
        }
    }else{
        sprintf(mensaje, "[ctrlz()→ Señal %i (SIGSTOP) no enviada por %d debido a "
                "que no hay proceso en foreground]\n",SIGSTOP,getpid());
        write(2, mensaje, strlen(mensaje)); 
    }
}

int parse_args(char **args, char *line) {
    int i = 0;

    args[i] = strtok(line, " \t\n\r");
    #if DEBUGN1 
       // printf("[parse_args()→ token %i: %s]\n", i, args[i]);
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
           // printf("[parse_args()→ token %i corregido: %s]\n", i, args[i]);
        #endif
    }
    return i;
}

int is_background(char **args){
    int c=0;
    while (args[c]!=NULL){
       c++;
    }
    if (strcmp (args[c-1],"&") == 0){ // Se comprueba si contiene & al final
        args[c-1] = NULL;
        return 1;
    }
    return 0;
}

int execute_line(char *line) {
    
    char *args[ARGS_SIZE];
    pid_t pid, status;
    char command_line[COMMAND_LINE_SIZE];

    memset(command_line, '\0', sizeof(command_line)); 
    strcpy(command_line, line);

    if (parse_args(args, line) > 0){

        if (check_internal(args) == 0) {
            int isbg=is_background(args);
            pid = fork();
            if (pid == 0) { //hijo
                signal(SIGCHLD, SIG_DFL);
                signal(SIGINT, SIG_IGN);
                signal(SIGTSTP,SIG_IGN);
                is_output_redirection(args);
                if (execvp(args[0], args) < 0){
                    fprintf(stderr,"%s: no se encontró la orden \n",args[0]);
                    exit(EXIT_FAILURE);
                }

            } else if (pid > 0) { // padre
                pidshell = getpid();
                printf("[execute_line()→ PID padre: %d (%s)]\n",pidshell,mi_shell);
                printf("[execute_line()→ PID hijo: %d (%s)]\n", pid,command_line);
                if (isbg == 1){
                    if(jobs_list_add(pid,'E',command_line) == 0){
                        printf("[%d] %d     %c      %s\n", n_pids, jobs_list[n_pids].pid,
                        jobs_list[n_pids].status, jobs_list[n_pids].cmd);
                    }else{
                        exit(EXIT_FAILURE);
                    }
                }else{
                    jobs_list[0].pid = pid;
                    jobs_list[0].status = 'E';
                    strcpy(jobs_list[0].cmd, command_line);
                    while (jobs_list[0].pid != 0) {
                        pause();
                    } 
                }
            }else { //pid <0 error
                perror("fork error");
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
    char line[COMMAND_LINE_SIZE];
    memset(line, 0, COMMAND_LINE_SIZE);
    strcpy(mi_shell,argv[0]);
    signal(SIGCHLD, reaper);
    signal(SIGINT, ctrlc);
    signal(SIGTSTP,ctrlz);
    while (1) {
        if (read_line(line)) { // !=NULL
            execute_line(line);
        }
    }
    return 0;
}






