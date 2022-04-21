/****************************************************************************
*                PRÀCTICA 2 SISTEMES OPERATIUS 21/22                        *
*                                                                           *
*                my_shell.c : nivelD comentat completament i                *
*                             sense comentaris de debugging                 *
*                                                                           *
*                   VERSIÓ FINAL DEL MINISHELL                              *
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

//Si no val 1 el prompt será només el carácter de PROMPT
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

#define COMMAND_LINE_SIZE 1024 //Tamany de la linea de comandos
#define ARGS_SIZE 64 // Tamany dels arguments
#define N_JOBS 24  //Quantitat de treballs permesos

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
int internal_cd(char **args);   //No implementada
int internal_export(char **args); //No implementada
int internal_source(char **args); //No implementada
int internal_jobs();
int internal_bg(char **args);
int internal_fg(char **args);
int is_output_redirection (char **args);
char *read_line(char *line);
int parse_args(char **args, char *line);
int execute_line(char *line);
void reaper(int signum);
void ctrlc(int signum);
void imprimir_prompt();
void ctrlz(int signum);
int jobs_list_add(pid_t pid, char status, char *command_line);
int jobs_list_find(pid_t pid);
int jobs_list_remove(pid_t pid);
int is_background (char **args);
int internal_fg(char **args);
int internal_bg(char **args);

static int pidshell;
static int n_pids;

//Variable global per guardar el nom del minishell
static char mi_shell[COMMAND_LINE_SIZE]; 

//Estructura d'un procés 
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

/*********************   FUNCIONS INTERNALS DONADES   ***********************/

void imprimir_prompt();

/*
*   Funcio: check_internal
*   -----------------
*   Retorna la funció si es interna
*   
*   char **args: arguments del treball
* 
*   (funció donada per esqueleto.c)
*/
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

/*
*   Funcions: 
*   - internal_cd
*   - internal_export
*   - internal_source
*   -----------------
* 
*   NO IMPLEMENTADES
*   (funcions donades per esqueleto.c)
*/
int internal_cd(char **args) {
    /*printf("[internal_cd()→ comando interno no implementado]\n");*/
    return 1;
} 

int internal_export(char **args) {
    /*printf("[internal_export()→ comando interno no implementado]\n");*/
    return 1;
}

int internal_source(char **args) {
    /*printf("[internal_source()→ comando interno no implementado]\n");*/
    return 1;
}

/*
*   Funcio: internal_jobs
*   -----------------
*   Imprimeix tots els treballs actuals
*   
*   retorna: 1, a part de l'impresió
*/
int internal_jobs() {
    int contador=0;
    //Per tots els treballs actuals
    while (contador<n_pids){
        contador++;
        //Imprimim els treballs actuals, amb el seu PID, estat i comando
        printf("[%i] %d     %c      %s \n",contador,jobs_list[contador].pid,
                jobs_list[contador].status,jobs_list[contador].cmd);
        }
    return 1;
}

/*
*   Funcio: internal_fg
*   -----------------
*   Agafa un procés i el passa a foreground
*   
*   char **args: arguments del procés
* 
*   retorna:  1 si exit 
*             0 si error   
*/
int internal_fg(char **args) {
    //Comprovam que hi ha arguments
    if(args[1] != NULL){
        int pos=atoi(args[1]);
        //Comprovam que existeix el treball
        if(pos>n_pids || pos==0){
            printf("fg %i: No existe ese trabajo\n",pos);
            return 1;
        }else{
            //Si existeix i esta detenido, el posam en marxa amb SIGCONT
            if(jobs_list[pos].status=='D'){
                kill(jobs_list[pos].pid,SIGCONT);
                jobs_list[pos].status = 'E';
                /*printf("[internal_fg()→ Señal %i (SIGCONT) enviada "
                        "a %d (%s)]\n",SIGCONT,jobs_list[pos].pid,
                        jobs_list[pos].cmd);*/
            }
            int contador=0;
            //Eliminam l'indicador & del procés
            while(jobs_list[pos].cmd[contador]!=0){
                if(jobs_list[pos].cmd[contador]=='&'){
                    jobs_list[pos].cmd[contador]=0;
                }
                contador++;       
            }
            // Passam el procés al foreground
            jobs_list[0]=jobs_list[pos];
            jobs_list_remove(pos);
            printf("%s\n",jobs_list[0].cmd);
            // El pare espera, ja que ara el fill está a foreground
            while (jobs_list[0].pid > 0){
                    pause();
            }
        }
    }else{
        perror("Sintaxis del procés incorrecte");
        return 0;
    }
    return 1;
}

/*
* Función: internal_bg
* --------------------
*  Agafa un procés i el passa a background
*
*  char **args: arguments del procés
*
*   retorna:  1 si exit 
*             0 si error   
*/
int internal_bg(char **args) {
    //Comprovam que hi ha arguments
    if(args[1] != NULL){
        int pos=atoi(args[1]);
        //Comprovam que existeix el treball
        if(pos>n_pids || pos==0){ 
            printf("bg %i: no existe ese trabajo\n",pos);
            return 1;
        }else{
            //Si existeix i esta executantse, ja está a background
            if(jobs_list[pos].status=='E'){ 
                printf("bg %i: el trabajo ya está en segundo plano\n",pos);
                return 1;
            }else{
                //Si existeix i está detenido, el posam en marxa amb SIGCONT i 
                //li afegim l'extensió &
                jobs_list[pos].status='E';
                char andd[2] = " &";
                strncat(jobs_list[pos].cmd,andd,2);
                kill(jobs_list[pos].pid,SIGCONT);
                /*printf("\n[internal_bg()→ Señal %i (SIGCONT) enviada a %d "
                        "(%s)]\n",SIGCONT,jobs_list[pos].pid,
                        jobs_list[pos].cmd);*/

                printf("[%i] %d     %c      %s \n",pos,jobs_list[pos].pid,
                jobs_list[pos].status,jobs_list[pos].cmd);
            }
        }
    }else{
        perror("Sintaxis del procés incorrecte");
        return 0;
    }
    return 1;
}

/*
* Función: imprimir_prompt
* --------------------
*  Imprimeix el prompt a la terminal amb una font i colors determinats
*
*   (funció donada a esqueleto.c)  
*/
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

/***************    FUNCIONS RELACIONADES AMB JOBS    ***********************/

/*
* Función: jobs_list_add
* --------------------
*  Afegeix un procés a la llista de jobs actual
*
*  pid_t pid: PID del procés a afegir
*  char status: estat del procés a afegir
*  char *cmd: comando del procés a afegir
*
*  retorna:  0 si s'ha afegit a la llista
*            -1 si no s'ha afegit   
*/
int jobs_list_add(pid_t pid, char status, char *cmd){
    //Miram que hi capiguen més procesos a la llista
    if(n_pids < N_JOBS-1){
        //Si hi ha lloc, l'afegim al següent
		jobs_list[n_pids+1].pid = pid;
		strcpy(jobs_list[n_pids+1].cmd,cmd);
		jobs_list[n_pids+1].status = status;
		n_pids ++;
		return 0;
	}
    return -1;
}

/*
* Función: jobs_list_find
* --------------------
*  Cerca la posició que ocupa a la llista un procés donat
*
*  pid_t pid: PID del procés a cercar
*
*  retorna:  la posició en la llista
*            -1 si no l'ha trobat   
*/
int jobs_list_find(pid_t pid){
    int i=1;
    //Mentre no l'hagim trobat i encara quedin jobs, el cercam
    while (jobs_list[i].pid!=pid && (i<n_pids && i<N_JOBS)){
        i++;
    }
    //Si l'hem trobat amb el PID, retornam la seva posició a la llista
    if (jobs_list[i].pid==pid){
        return i;
    }
    return -1;
}

/*
* Función: jobs_list_remove
* --------------------
*  Elimina un procés donat de la llista segons la seva posició a dita llista
*
*  int pos: posició del procés a eliminar
*
*  retorna:  0   
*/
int jobs_list_remove(int pos){
    //Si la posició es menor al nombre de procesos, eliminam el procés
    if (pos < n_pids) {
        jobs_list[pos] = jobs_list[n_pids];
    }
    //Reduim el nombre de procesos
    n_pids--;
    return 0;
}

/**************   FUNCIONS RELACIONADES AMB INTERRUPCIONS    ***************/

/*
* Función: reaper
* --------------------
*  Controla si el fill que s'acaba d'executar s'ha executat en primer pla,
*  maneja la señal SIGCHLD (enterrador de fills)
*
*  int signum: nombre de señal
*/
void reaper(int signum) {
  pid_t ended;
  int status;
  signal(SIGCHLD, reaper);
  //Esperam a que acabi un fill
  while ((ended = waitpid(-1, &status, WNOHANG)) > 0) {
      //Si el fill esteia en foreground ho enunciam, sigui per interrupció o no
      if (jobs_list[0].pid == ended) {
        if(status!=0){
            /*printf("[reaper()→ Proceso hijo %d en foreground (%s) "
                    "finalizado por señal: %d]\n",
                    ended, jobs_list[0].cmd, status);*/
        }else{
            /*printf("[reaper()→ Proceso hijo %d en foreground (%s) "
                    "finalizado con exit code: %d]\n",
                     ended, jobs_list[0].cmd, status);*/
        }
        jobs_list[0].pid = 0;
        jobs_list[0].status = 'F';
      }else{
            //Si el fill esteia en background ho enunciam
            int pos=jobs_list_find(ended);
            printf("\nTerminado PID %d (%s) en jobs_list[%i] con "
                    "status %i \n", ended,jobs_list[pos].cmd,pos,status);
            /*printf("\n[reaper()→ Proceso hijo %d en background (%s) "
                    "finalizado con exit code %d]\n", 
                    ended,jobs_list[pos].cmd,WEXITSTATUS(status));*/
            jobs_list_remove(pos);
        }
  }
}

/*
* Función: ctrlc
* --------------------
*  Maneja el ctrl+c, enviant la señal SIGTERM per finalitzar el procés
*  que está en primer pla
*
*  int signum: nombre de señal
*/
void ctrlc(int signum) {
  signal(SIGINT, ctrlc);
  /*printf("\n[ctrlc()→ Soy el proceso con PID %d, el proceso en foreground "
        "es %d (%s)]\n", getpid(), jobs_list[0].pid, jobs_list[0].cmd);*/

  //Si el procés está en foreground...
  if (jobs_list[0].pid > 0) {
    //...i no es el shell en si, l'eliminam
    if ( strcmp(jobs_list[0].cmd, mi_shell) != 0) {
      /*printf("[ctrlc()→ Señal %i (SIGTERM) enviada a %d (%s) por "
            "%d]\n",SIGTERM,jobs_list[0].pid, jobs_list[0].cmd, pidshell);*/
      kill(jobs_list[0].pid, SIGTERM);
    } else {
        /*printf("[ctrlc()→ Señal %i (SIGTERM) no enviada debido a que "
        "el proceso en foreground es el shell]\n",SIGTERM);*/
    }
  } else {
    /*printf("[ctrlc()→ Señal %i (SIGTERM) no enviada debido a que no hay "
            "proceso en foreground]\n",SIGTERM);*/
  }
}

/*
* Función: ctrlz
* --------------------
*  Maneja el ctrl+z, enviant la señal SIGSTOP per detendre el procés
*  que está en primer pla
*
*  int signum: nombre de señal
*/
void ctrlz(int signum){
    signal(SIGTSTP,ctrlz);
    char mensaje[1200];
    /*sprintf(mensaje, "\n[ctrlz()→ Soy el proceso con PID %d, el proceso en "
            "foreground es %d (%s)]\n",getpid(),jobs_list[0].pid,
            jobs_list[0].cmd);
    write(2, mensaje, strlen(mensaje)); */

    //Si el procés esta en foreground...
    if(jobs_list[0].pid>0){ 
        //...i no es el shell en si, l'aturam
        if(strcmp(jobs_list[0].cmd, mi_shell) != 0){
            /*printf("[ctrlz()→ Señal %i (SIGSTOP) enviada a %d (%s) por " 
                    "%d (%s)]\n",SIGSTOP,jobs_list[0].pid,jobs_list[0].cmd,
                    getpid(),mi_shell);*/
            kill(jobs_list[0].pid,SIGSTOP);
            jobs_list[0].status='D';
            jobs_list_add(jobs_list[0].pid,
                        jobs_list[0].status,
                        jobs_list[0].cmd);
            int contador=jobs_list_find(jobs_list[0].pid);
            jobs_list[0].pid=0;
            printf("\n[%i] %d     %c      %s \n",contador,
                    jobs_list[contador].pid,
                    jobs_list[contador].status,
                    jobs_list[contador].cmd);
        }else{
            /*printf("[ctrlz()→ Señal %i (SIGSTOP) no enviada por " 
                    "%d (%s) debido a que el proceso en foreground "
                    "es el shell]\n",SIGSTOP,getpid(),mi_shell);*/
           
        }
    }else{
        /*sprintf(mensaje, "[ctrlz()→ Señal %i (SIGSTOP) no enviada por "
                "%d debido a que no hay proceso en foreground]\n",
                SIGSTOP,getpid());
        write(2, mensaje, strlen(mensaje));*/
    }
}

/***********   FUNCIONS RELACIONADES AMB EL MANEIG DE COMANDO    ************/

/*
* Función: is_background
* --------------------
*  Ens indica si un procés esta en background o no
*
*  char **args: arguments del procés
*
*  retorna:  1 si està en background
*            0 si no està en background
*/
int is_background(char **args){
    int c=0;
    //Ens colocam al final del fluxe
    while (args[c]!=NULL){
       c++;
    }
    //Comprovam si l'argument té & al final, es a dir, si està en segón pla
    if (strcmp (args[c-1],"&") == 0){
        args[c-1] = NULL;
        return 1;
    }
    return 0;
}

/*
* Función: parse_args
* --------------------
*  Separa el comando en els seus diferents arguments
*
*  char **args: arguments del procés
*  char *line: comando
*
*  retorna:  nombre d'arguments
*
*  (funció donada per esqueleto.c)
*/
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

/*
* Función: execute_line
* --------------------
*  S'executa el comando introduit al minishell
*
*  char *line: comando
*
*  retorna:  1 si exit
*            0 si error
*/
int execute_line(char *line) {
    char *args[ARGS_SIZE];
    pid_t pid, status;
    char command_line[COMMAND_LINE_SIZE];

    //Guardam el comando a memoria, ja que parse_args el canviarà
    memset(command_line, '\0', sizeof(command_line)); 
    strcpy(command_line, line);

    //Comprovam que hi ha arguments
    if (parse_args(args, line) > 0){
        //Comprovam que sigui un comando intern
        if (check_internal(args) == 0) {
            int isbg = is_background(args);
            pid = fork();
            //Si el pid es 0, serà un fill
            if (pid == 0) {
                signal(SIGCHLD, SIG_DFL);
                signal(SIGINT, SIG_IGN);
                signal(SIGTSTP,SIG_IGN);
                is_output_redirection(args);
                //Executam el comando
                if (execvp(args[0], args) < 0){
                    fprintf(stderr,"%s: no se encontró la orden \n",args[0]);
                    exit(EXIT_FAILURE);
                }
            //Si el pid es més gran que 0, serà un pare
            } else if (pid > 0) {
                pidshell = getpid();
                /*printf("[execute_line()→ PID padre: %d (%s)]\n",
                pidshell,mi_shell);*/
                /*printf("[execute_line()→ PID hijo: %d (%s)]\n",
                pid,command_line);*/

                //Si està en background, l'afegim a la llista de jobs
                if (isbg == 1){
                    if(jobs_list_add(pid,'E',command_line) == 0){
                        printf("[%d] %d     %c      %s\n", n_pids, 
                        jobs_list[n_pids].pid,
                        jobs_list[n_pids].status, 
                        jobs_list[n_pids].cmd);
                    }else{
                        exit(EXIT_FAILURE);
                    }
                //Si no està en background, el posam al foreground i esperam
                //que s'executi
                }else{
                    jobs_list[0].pid = pid;
                    jobs_list[0].status = 'E';
                    strcpy(jobs_list[0].cmd, command_line);
                    while (jobs_list[0].pid != 0) {
                        pause();
                    } 
                }
            }else {
                perror("Error al Fork");
                exit(EXIT_FAILURE);
            }
        }
        return 1;
    }
    return 0;
}

/*************   FUNCIONS RELACIONADES AMB INPUT/OUTPUT    **************/

/*
* Función: read_line
* --------------------
*  Llegeix una linia de comando
*
*  char *line: comando
*
*  retorna:  punter a la linea llegida
*  
*  (funció donada per esqueleto.c)
*/
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

/*
* Función: is_output_redirection
* --------------------
*  Mira si el procés conté ">". Si el té, obrirem el fitxer per redireccionar
*
*  char **args: arguments del procés
*
*   retorna:  1 si exit 
*             0 si error   
*/
int is_output_redirection (char **args){
    int file;
    int cont = 0;
    //Comprovam que hi ha arguments
    while (args[cont] != NULL){ 
        //Si l'argument conté ">" obrim el fitxer indicat pel següent argument
        if (*args[cont] == '>' && args[cont+1] != NULL){
            file=open (args[cont+1],O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
            //Si el fitxer es correcte, llevam ">" i escrivim al fitxer
            if(file>=0){
                args[cont]=0;
                args[cont+1]=0; 
                dup2(file,1);
                close(file);
                return 1;
            }else{
                perror("Error al fitxer");
                return 0;
            }
            return 0;
        }
        cont++;
    }
  return 0;
}

/*******************   FUNCIÓ PRINCIPAL DEL PROGRAMA   *********************/

/*
* Función: main
* --------------------
*  Inicia les senyals, i executa les linies que va llegint
*
*  char *argv: arguments de la linea de comando
*
*   retorna:  0  
*/
int main(int argc, char *argv[]) {
    char line[COMMAND_LINE_SIZE];
    memset(line, 0, COMMAND_LINE_SIZE);
    strcpy(mi_shell,argv[0]);
    //Iniciam senyals
    signal(SIGCHLD, reaper);
    signal(SIGINT, ctrlc);
    signal(SIGTSTP,ctrlz);
    //Llegim i executam
    while (1) {
        if (read_line(line)) {
            execute_line(line);
        }
    }
    return 0;
}