// Gil Elbaz 206089690
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define MAX 100


/**
 * Type for individual stack entry
 */
struct stack_entry {
    char *data;
    struct stack_entry *next;
};

/**
 * Type for stack instance
 */
struct stack_t {
    struct stack_entry *head;
    size_t stackSize;  // not strictly necessary, but
    // useful for logging
};

// Declaring functions

// Parse and Execute
void execute(char **argv);
void parse(char *line, char **argv);
//----------------------------------
// Stack Functions
void change_directory(const char *arg);
void pop(struct stack_t *theStack);
char *top(struct stack_t *theStack);
void push(struct stack_t *theStack, const char *value);
void destroyStack(struct stack_t **theStack);
struct stack_t *newStack(void);
struct stack_t *paths = NULL;
//---------------------------------
void display_history();
// global variables
int counter = 0;
int status = 0;
int child = 0;
int size = 0;
int pid = 0;
pid_t val;
int pids[MAX];
char* history[MAX];
void jobs();

// Main
int main() {
    paths = newStack();
    char input[MAX]; // Array of the "raw" input
    char *argv[MAX]; // Array for each command
    char* str;
    while (1) {
        printf("$ ");
        fflush(stdout);
        fgets(input, 100, stdin);
        str = (char*)malloc((strlen(input) + 1) * sizeof(char));
        input[strcspn(input, "\n")] = '\0';
        strcpy(str, input);
        history[counter] = str;
        counter++;
        parse(input, argv);
        if (strcmp(argv[0], "exit") == 0) {
            destroyStack(&paths);
            exit(0);
        } else if (strcmp(argv[0], "cd") == 0) {
            change_directory(argv[1]);
        }
        else if (strcmp(argv[0],"jobs") == 0){
            jobs();
        }
        else if (strcmp(argv[0],"history") == 0){
            display_history();
        }
        else {
            execute(argv);
        }

    }
}

void jobs(){
    int j;
    for (j = 0; j < counter; j++) {
        //procces is running
        if(waitpid(pids[j],&status,WNOHANG) == 0){
            printf("%d %s\n", pids[j], history[j]);
        }
    }
    //updating the pid of the procces
    pids[counter] =(int)getpid();
    counter++;
}

void foreground(char** line) {
    int ret_code;
    val = fork();
    pids[counter] = (int)val;
    counter++;
    if (val == 0) {
        //in child
        child = 1;
        pid = (int)getpid();
        printf("%d\n", pid);
        ret_code = execvp(line[0], line);
        if (ret_code == -1) {
            fprintf(stderr, "Error in system call\n");
        } else {
            printf("exec succeed\n");
        }

    } else if (val < 0) {
        fprintf(stderr, "Error in system call\n");
    } else {
        //in father
        wait(&ret_code);

    }
}

void background(char** line){
    int ret_code;
    pid_t val;
    val = fork();
    pids[counter] = (int)val;
    counter++;
    if(val == 0){
        //in child
        child = 1;
        printf("%d\n", (int)getpid());
        //remove the '&' from the command
        line[size - 1] = NULL;
        ret_code = execvp(line[0], line);
        if(ret_code == -1){
            fprintf(stderr,"Error ib system call\n");
        } else{
            printf("exec succeed\n");
        }
    }
}


void display_history(){
    pids[counter] = (int)getpid();
    //counter++;
    for (int j = 0; j < counter; j++){
        if(waitpid(pids[j],&status,WNOHANG) == 0 ||
           (j == counter - 1 && strcmp((char*)history[j],"history") == 0)){
            printf("%s RUNNING\n", history[j]);
        } else {
            //done
            printf("%s DONE\n", history[j]);
        }
    }
}

void change_directory(const char *arg) {
    pids[counter] = (int)getpid();
    counter++;
    char cwd[256] = {0};
    getcwd(cwd, 256);
    char *home_dir = "/home";
    if ((arg == NULL) || (!(strcmp(arg, "~") != 0 && strcmp(arg, "~/") != 0))) {
        chdir(home_dir);
        return;
    } else if (!(strcmp(arg, "-"))) {
        char *last_path = top(paths);
        if (last_path == NULL) {
            printf("An error occurred \n");
            return;
        }
        chdir(last_path);
        pop(paths);
        return;
    }
    if (chdir(arg) < 0)
        printf("Too many arguments \n");
    else {
        push(paths, cwd);
    }

}

// Parse given input
void parse(char *line, char **argv) {
    // Check if theres anymore input
    while (*line != '\0') {
        while (*line == ' ' || *line == '\t' || *line == '\n')
            // Replace " " with zero
            *line++ = '\0';
        // Store given input
        *argv++ = line;
        while (*line != '\0' && *line != ' ' &&
               *line != '\t' && *line != '\n')
            line++;
    }
    *argv = '\0';
}

// Execute the command
void execute(char **argv) {
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {
        printf("fork failed \n");
        exit(0);
    } else if (pid == 0) {
        if (execvp(*argv, argv) < 0) {
            printf("exec failed \n");
            exit(0);
        }
    }
        // Parent
    else {
        while (wait(&status) != pid);
    }
}


/**
 * Create a new stack instance
 */
struct stack_t *newStack(void) {
    struct stack_t *stack = (struct stack_t *) malloc(sizeof *stack);
    if (stack) {
        stack->head = NULL;
        stack->stackSize = 0;
    }
    return stack;
}

/**
 * Make a copy of the string to be stored (assumes
 * strdup() or similar functionality is not
 * available
 */
char *copyString(char *str) {
    char *tmp = (char *) malloc(strlen(str) + 1);
    if (tmp)
        strcpy(tmp, str);
    return tmp;
}

/**
 * Push a value onto the stack
 */
void push(struct stack_t *theStack, const char *value) {
    struct stack_entry *entry = malloc(sizeof *entry);
    if (entry) {
        entry->data = copyString(value);
        entry->next = theStack->head;
        theStack->head = entry;
        theStack->stackSize++;
    } else {
        // handle error here
    }
}

/**
 * Get the value at the top of the stack
 */
char *top(struct stack_t *theStack) {
    if (theStack && theStack->head)
        return theStack->head->data;
    else
        return NULL;
}

/**
 * Pop the top element from the stack; this deletes both
 * the stack entry and the string it points to
 */
void pop(struct stack_t *theStack) {
    if (theStack->head != NULL) {
        struct stack_entry *tmp = theStack->head;
        theStack->head = theStack->head->next;
        free(tmp->data);
        free(tmp);
        theStack->stackSize--;
    }
}

/**
 * Clear all elements from the stack
 */
void clear(struct stack_t *theStack) {
    while (theStack->head != NULL)
        pop(theStack);
}

/**
 * Destroy a stack instance
 */
void destroyStack(struct stack_t **theStack) {
    clear(*theStack);
    free(*theStack);
    *theStack = NULL;
}




