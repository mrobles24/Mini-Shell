# Mini-Shell
Implementación de un Mini-Shell básico basado en el bash de Linux.

El minishell tendrá las siguientes características:
- Admitirá varios comandos internos (implementados por nosotr@s) que serán
ejecutados por el propio minishell y que no generarán la creación de un nuevo proceso:
  - jobs: Muestra el PID de los procesos que se están ejecutando en background o que estén detenidos.
  - fg: Envía un trabajo del background al foreground, o reactiva la ejecución en foreground de un trabajo que había sido detenido
  - bg: Reactiva un proceso detenido para que siga ejecutándose pero en segundo plano
  - exit: Permite salir del mini shell (además de poder hacerlo con Ctrl+D).
  
- Admitirá comandos propios del shell estándard. El programa leerá una línea de comandos, la ejecutará con un nuevo proceso (fork() + execvp()) y permanecerá en espera a que el proceso termine para leer la siguiente línea de comandos
- Admitirá redireccionamiento de la salida de un comando a un fichero, indicando “> fichero” al final de la línea
- Admitirá procesos en segundo plano (background). Si al final de la línea de comandos hay un “&” el proceso se ejecutrá en segundo plano. Es decir, el shell no esperará a que acabe para seguir leyendo los siguientes comandos (aunque avisará cuando el proceso haya acabado, de forma similar a cómo lo hace el shell estándard).
- Se admitirá la interrupción con Ctrl + C y Ctrl + Z
