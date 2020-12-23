#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>     // strcmp(2)
#include <assert.h>     // assert()
#include <sys/stat.h> 
#include <fcntl.h>      // O_RDONLY, O_CREAT, O_WRONLY
#include <sys/wait.h>   // wait(1)
#include <sys/types.h>
#include <signal.h> 
#include <sys/prctl.h>

#define MAX_LINE 80              /* The maximum length command */
char* history[MAX_LINE / 2 + 1]; // history buffer
char* input = NULL;              // input redirection filename
char* output = NULL;             // output redirection filename

// tokenize splits line at space and places the tokens into args
// returns number of tokens (arguments)
int tokenize(char *line, char *args[]) {
  char *token;
  char *whitespace = " ";
  int index = 0;

  token = strtok(line, whitespace); // get first token
  while (token != NULL) {
    args[index++] = token; // add token to args
    token = strtok(NULL, whitespace); // get next token
  }

  args[index] = NULL; // set end of args
  return index; 
}

// readInput reads the user's input and places each argument into args
// returns number of arguments
int readInput(char *args[]) {
  char *line = NULL;
  ssize_t bufsize = 0; // getline allocates buffer
  int size = getline(&line, &bufsize, stdin); // get input until user enters return
  if (size > 0) {
    line[size - 1] = 0;
    size = tokenize(line, args);
  }

  return size;
}

// printArgs prints args w/spaces
void printArgs(char *args[]) {
  int i = 0;
  while (args[i] != NULL) {
    printf("%s", args[i]);
    printf(" ");
    i++;
  }
  printf("\n");
}

// addHistory copies args to history buffer
void addHistory(char *args[]) {
  int i = 0;
  while (args[i] != NULL) {
    history[i] = args[i];
    i++;
  }
  history[i] = NULL;
}

// getHistory copies history buffer to args
void getHistory(char *args[]) {
  int i = 0;
  while (history[i] != NULL) {
    args[i] = history[i];
    i++;
  }
  args[i] = NULL;
}

// contains checks if args contains c
// returns 1 if args has c, returns 0 if not
int contains(char *args[], char *c) {
  int i = 0;
  while (args[i] != NULL) {
    if (strcmp(args[i], c) == 0) return 1;
    i++;
  }

  return 0;
}

// contains checks if args contains l or r
// returns 0 if first occurence is l, returns 1 if r occurs first
// returns -1 if neither occur
int containsFirst(char *args[], char *l, char *r) {
  int i = 0;
  while (args[i] != NULL) {
    if (strcmp(args[i], l) == 0) return 0;
    if (strcmp(args[i], r) == 0) return 1;
    i++;
  }

  return -1;
}

// getFile checks if args contains rediraction operators
// if there are those operators then the next arg is assigned to input or output (fields)
void getFile(char* args[]) {
  int i = 0;
  while (args[i] != NULL) {
    if (strcmp(args[i], "<") == 0) { // if <
      input = args[i + 1] != NULL ? args[i + 1] : NULL; // assign next arg if it exists. NULL otherwise
      args[i] = NULL;
      return;
    }
    
    if (strcmp(args[i], ">") == 0) { // if >
      output = args[i + 1] != NULL ? args[i + 1] : NULL;
      args[i] = NULL;
      return;
    }

    i++;
  }
}

// startProcess executes args
// parent process waits if background flag is 0, otherwise it continues
void startProcess(char *args[], int background) {
  int pid = fork(); // create child process

  if (pid == -1) {
    perror("fork error");
    exit(1);
  }

  //child
  if (pid == 0) {
    int r = prctl(PR_SET_PDEATHSIG, SIGTERM); // kill child when parent dies

    if (r == -1) {
      perror(0);
      exit(1);
    }

    if (getpid() == 1) { // check if parent exited
      exit(1);
    }

    if (input != NULL) { // if input redirection operator
      int in = open(input, O_RDONLY);
      dup2(in, STDIN_FILENO); // redirect stdin
      close(in);
    }

    if (output != NULL) { // if output redirection operator
      int out = open(output, O_WRONLY|O_CREAT, 0666);
      dup2(out, STDOUT_FILENO);
      close(out);
    }
    
    int success = execvp(args[0], args); // execute program
    assert(success >= 0);
    exit(1);
  }

  //parent ampersand
  else if (pid > 0 && background > 0) {
    return; // go back to main
  }

  //parent wait
  else {
    int w, status;
    while (wait(&status) > 0); // wait until child finishes
  }
}

// getLeft returns a string array containing all arguments to the left of c in args
char** getLeft(char* args[], char *c) {
  char** left = malloc(MAX_LINE * sizeof(char*)); // allocate memory
  for (int j = 0; j < MAX_LINE; j++) {
    left[j] = malloc(MAX_LINE);
  }
  int i = 0;
  while (strcmp(args[i], c) != 0) { // while current string not c
    left[i] = args[i]; // add to array
    i++;
  }
  left[i] = NULL;
  return left;
}

// getRight returns a string array containing all arguments to the right of c in args
char** getRight(char* args[], char *c) {
  char** right = malloc(MAX_LINE * sizeof(char*)); // allocate memory
  for (int j = 0; j < MAX_LINE; j++) {
    right[j] = malloc(MAX_LINE);
  }
  int i = 0, j = 0;
  while (strcmp(args[i], c) != 0) { // while current string not c
    i++;
  }
  i++;
  while (args[i] != NULL) { // while current string not NULL
    right[j] = args[i]; // add to array
    i++;
    j++;
  }
  right[j] = NULL; // set end of array NULL
  return right;
}

// removeArg removes c (string) from args (string array)
void removeArg(char* args[], char *c) {
  int i = 0;
  while (args[i] != NULL) { // while current string not NULL
    if (strcmp(args[i], c) == 0) { // if found c
      while (args[i + 1] != NULL) { // while next string not NULL
        args[i] = args[i + 1]; // shift subsequent strings left
        i++;
      }
      args[i] = NULL; // set new end of array
      return;
    }
    i++;
  }
}

// startPipe executes left and right commands and pipes left's output to right's input
void startPipe(char** left, char** right, int background) {
  int PIPE_READ = 0, PIPE_WRITE = 1, status, success;
  int pipes[2]; // pipe file descriptors
  pipe(pipes);
  int pid = fork(); // start child process

  if (pid == 0) {
    int r = prctl(PR_SET_PDEATHSIG, SIGTERM); // kill child when parent dies

    if (r == -1) {
      perror(0);
      exit(1);
    }

    if (getpid() == 1) { // check if parent exited
      exit(1);
    }

    pid = fork(); // start grandchild process
    if (pid == 0) { // Grandchild
      int res = prctl(PR_SET_PDEATHSIG, SIGTERM);
      if (res == -1) {
        perror(0);
        exit(1);
      }
      if (getpid() == 1) {
        exit(1);
      }
      close(pipes[PIPE_READ]); // grandchild doesn't need read access.
      dup2(pipes[PIPE_WRITE], STDOUT_FILENO); // only needs write
      close(pipes[PIPE_WRITE]);
      execvp(left[0], left); // execute lhs of lhs | rhs
      exit(1);
    }
    else { // Child
      close(pipes[PIPE_WRITE]); // child doesn't need write
      dup2(pipes[PIPE_READ], STDIN_FILENO); // only needs read to get left's output
      close(pipes[PIPE_READ]);
      execvp(right[0], right); // execute rhs (lhs | rhs)
      exit(1);
    }
  }

  else { // Parent
    close(pipes[PIPE_READ]); // parent doesn't need read or write
    close(pipes[PIPE_WRITE]);
    if (background > 0) { // if ampersand, go back to main
      return;
    }

    while (wait(&status) > 0); // otherwise wait until child is done
  }
}

// execute prepares to execute the command entered into the shell
// returns 0 if exit is entered for args[0]
// returns 1 for every other command
int execute(char *args[], int numArgs) {
  if (args[0] == NULL) return 1; // blank line

  if (strcmp(args[0], "exit") == 0) return 0; // exit

  // cd is not a program
  if (strcmp(args[0], "cd") == 0) {
    chdir(args[1]);
    return 1;
  }

  int background = 0, background2 = 0; // ampersand flags for 2 commands
  char** left = NULL, **right = NULL; // string array for 2 commands

  // history 
  if (strcmp(args[0], "!!") == 0) {
    if (history[0] == NULL) { // nothing in history
      printf("No commands in history.\n");
      return 1;
    }

    getHistory(args); // assign history buffer to args
    printArgs(args);
  }

  addHistory(args); // add command to history buffe

  // if contains &
  if (contains(args, "&") || contains(args, ";")) {
    int cont = containsFirst(args, "&", ";");
    char *c = cont == 0 ? "&" : ";";
    left = getLeft(args, c); // get left of & or ;
    right = getRight(args, c);
    removeArg(args, c); // remove & or ; from args
    background = strcmp(c, "&") == 0 ? 1 : 0; // set ampersand flag to true

    if (right != NULL && (contains(right, "&") || contains(right, ";"))) { // if second command
      c = contains(right, "&") ? "&" : ";";
      removeArg(right, c); // remove c from right and args
      removeArg(args, c);
      background2 = strcmp(c, "&") == 0 ? 1 : 0; // set ampersand flag for second command
    }
  }

  // pipe path
  if (contains(args, "|")) {
    left = getLeft(args, "|"); // get command left of pipe
    right = getRight(args, "|"); // rhs command
    startPipe(left, right, background);
    return 1; // return after piping processes
  }

  // if 2 commands and at least one has an ampersand
  if (left != NULL && right != NULL) {
    startProcess(left, background); // do left and right process
    startProcess(right, background2);
    return 1;
  }

  getFile(args); // check for redirection operaters and get filenames
  startProcess(args, background); // executes command
  return 1;
}

// main starts the shell
// always returns 0
int main(void)
{
  history[0] = NULL; // initialize history buffer
  char* args[MAX_LINE/2 + 1]; /* command line arguments */
  int should_run = 1; /* flag to determine when to exit program */

  while (should_run) {
    printf("osh>");
    fflush(stdout);

    int numArgs = readInput(args); // put input into args and get number of arguments

    /**
    * After reading user input, the steps are:
    * (1) fork a child process using fork()
    * (2) the child process will invoke execvp()
    * (3) parent will invoke wait() unless command included &
    */
    should_run = execute(args, numArgs); // execute command
    input = NULL;
    output = NULL;
  }

  return 0;
}
