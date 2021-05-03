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

int parse(char *line, char **argv);

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
char **split_line(char *line);

void print_echo(char **text);

void display_history();

// global variables
int counter = 0, status = 0, size = 0, child = 0;
int echo_counter = 0;
pid_t val;
int pids[MAX];
char *history[MAX];
char **echo;

void jobs();

void background(char **line, int length);

int foreground(char **line);

void display_echo(char **args, int length);

void free_history();

// Main
int main() {
    paths = newStack();
    char input[MAX]; // Array of the "raw" input
    char *argv[MAX]; // Array for each command
    char *str;
    int length = 0;
    int foreground_ret = 0;
    while (1) {
        printf("$ ");
        fflush(stdout);
        fgets(input, 100, stdin);
        str = (char *) malloc((strlen(input) + 1) * sizeof(char));
        input[strcspn(input, "\n")] = '\0';
        strcpy(str, input);
        history[counter] = str;
        length = parse(input, argv);
        if (length <= 0)
            continue;
        if (strcmp(argv[0], "jobs") == 0) {
            jobs();
        } else if (strcmp(argv[0], "history") == 0) {
            display_history();
        } else if (strcmp(argv[0], "cd") == 0) {
            change_directory(argv[1]);
        } else if (strcmp(argv[0], "echo") == 0) {
            display_echo(argv, length);
        } else if (strcmp(argv[length - 1], "&") == 0) {
            background(argv, length);
        } else if (strcmp(argv[0], "exit") == 0) {
            destroyStack(&paths);
            free_history();
            exit(0);
        } else {
            // replacing execute
            foreground_ret = foreground(argv);
            if (foreground_ret == 0) {
                break;
            }
        }
    }
}

void free_history() {
    int i;
    for (i = 0; i < counter; i++) {
        free(history[i]);
    }
}

void display_echo(char **args, int length) {
    int i;
    const char ignore[2] = "\"";
    char *token;
    for (i = 1; i < length; i++) {

        token = strtok(args[i], ignore);
        while (token != NULL) {
            printf("%s", token);
            fflush(stdout);
            token = strtok(NULL, ignore);
        }
        printf(" ");
        fflush(stdout);
    }
    printf("\n");
}

void jobs() {
    int j;
    for (j = 0; j < counter; j++) {
        //procces is running
        if (waitpid(pids[j], &status, WNOHANG) == 0) {
            printf("%s\n", history[j]);
        }
    }
    //updating the pid of the procces
    pids[counter] = (int) getpid();
    counter++;
}

int foreground(char **line) {
    int ret_code;
    val = fork();
    pids[counter] = (int) val;
    counter++;
    if (val == 0) {
        //in child
        ret_code = execvp(line[0], line);
        if (ret_code == -1) {
            printf("exec failed\n");
        }
        return 0;
    } else if (val < 0) {
        printf("exec failed\n");
    } else {
        //in father
        wait(&ret_code);
    }
    return 1;
}

void background(char **line, int length) {
    int ret_code;
    val = fork();
    pids[counter] = (int) val;
    counter++;
    if (val == 0) {
        //in child
        //remove the '&' from the command
        // child = 1;
        line[length - 1] = NULL;
        ret_code = execvp(line[0], line);
        if (ret_code == -1) {
            printf("exec failed\n");
        }
    } else if (val < 0) {
        printf("exec failed\n");
    } else {
        //in father
        waitpid(-1, &ret_code, WNOHANG);
    }
}


void display_history() {
    int j;
    pids[counter] = (int) getpid();
    counter++;
    for (j = 0; j < counter; j++) {
        if (waitpid(pids[j], &status, WNOHANG) == 0 ||
            (strcmp((char *) history[j], "history") == 0)) {
            printf("%s RUNNING\n", history[j]);
        } else {
            //done
            printf("%s DONE\n", history[j]);
        }
    }
}

void change_directory(const char *arg) {
    pids[counter] = (int) getpid();
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
int parse(char *line, char **argv) {
    // Check if theres anymore input
    int count = 0;
    while (*line != '\0') {
        while (*line == ' ' || *line == '\t' || *line == '\n')
            // Replace " " with zero
            *line++ = '\0';
        // Store given input
        *argv++ = line;
        while (*line != '\0' && *line != ' ' &&
               *line != '\t' && *line != '\n')
            line++;
        count++;
    }
    *argv = '\0';
    return count;
}

/* Execute the commandI
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
*/

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




