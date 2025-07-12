#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <fnmatch.h>
#include <signal.h>

#define SIZE 1024
#define MAX_TOKENS 128
#define MAX_CMD 1024
#define MAX_ARG 100

struct Command {
    char *arguments[MAX_ARG];
    int numArgs;
    char *executable;
    char *inputFile;
    char *outputFile;
    int hasPipe;
    int hasAnd;
    int hasOr;
};

//function declarations
void interactiveMode();
void batchMode(FILE *input);
void tokenizer(char *input, char *tokens[], int *token_count);
void commandParser(char* tokens[], int numsOfTokens, struct Command* command);
void wildcards(char *tokens[], int *token_count);
int execute(struct Command *cmd);
int executePipe(struct Command *L_cmd, struct Command *R_cmd);
void andOrHandler(struct Command *cmd);
int isBuiltInCommand(struct Command *cmd);
void handleSigint();

void handleSigint() {
    printf("\nType 'exit' to quit the shell.\n");   
    fflush(stdout);
}

void interactiveMode(){
    char buffer[SIZE];
    char *tokens[MAX_TOKENS];
    int token_count;

    signal(SIGINT, handleSigint);
    printf("Welcome to my shell!\n");

    //reading and ready for input
    while(1){
        printf("mysh> "); //indicates ready for input
        fflush(stdout);

        //if not read
        ssize_t bytesRead = read(STDIN_FILENO, buffer, SIZE);
        if(bytesRead <= 0){
            break;
        }

        //null terminate string
        buffer[bytesRead] = '\0';

        //if given exit or die
        if(strcmp(buffer, "exit\n") == 0 || strcmp(buffer, "die\n") == 0){
            exit(EXIT_FAILURE);
        }

        //Step 1 - tokenize input DONE
        tokenizer(buffer, tokens, &token_count);

        //Step 2 - wildcards
        wildcards(tokens, &token_count);

        //print wildcards to test
        /*printf("\n");
        printf("After wildcards:\n");
        for (int i = 0; i < token_count; i++) {
            printf("Token %d: %s\n", i + 1, tokens[i]);
        }*/

        if(token_count > 0){
            //need to account for other commands later
            struct Command cmd = {0};
            commandParser(tokens, token_count, &cmd);
            if (isBuiltInCommand(&cmd)) {
                continue;
            }
            andOrHandler(&cmd);

            //free each token after use - was moved from tokenizer in order to test command parser
            for (int i = 0; i < token_count; i++) {
                free(tokens[i]);
            }
        }
    }
    printf("Exiting my shell.\n");
}

void batchMode(FILE *input){ //no need for welcome and exit statements
    char buffer[SIZE];
    char *tokens[MAX_TOKENS];
    int token_count;

    while(fgets(buffer, SIZE, input)){
        //if given exit or die
        if(strcmp(buffer, "exit\n") == 0 || strcmp(buffer, "die\n") == 0){
            exit(EXIT_FAILURE);
        }

        //Step 1 - tokenizer
        tokenizer(buffer, tokens, &token_count);

        //Step 2 - wildcards
        wildcards(tokens, &token_count);

        //print wildcards to test
        /*printf("\n");
        printf("After wildcards:\n");
        for (int i = 0; i < token_count; i++) {
            printf("Token %d: %s\n", i + 1, tokens[i]);
        }*/

        if(token_count > 0){
            //need to account for other commands later
            struct Command cmd = {0};
            commandParser(tokens, token_count, &cmd);
            if (isBuiltInCommand(&cmd)) {
                continue;
            }
            andOrHandler(&cmd);

            //free each token after use from testing- was moved from tokenizer in order to test command parser
            for (int i = 0; i < token_count; i++) {
                free(tokens[i]);
            }
        }
    }
}

//Tokenizer
//note command containing only a comment is treated as an empty command, comments start wit #
void tokenizer(char *input, char *tokens[], int *token_count) {
    char *current_token = NULL;
    *token_count = 0;
    size_t input_len = strlen(input);

    for (size_t i = 0; i < input_len; i++) {
        char ch = input[i];

        // if the character is part of word add it to current token
        if (!isspace(ch) && ch != '#' && ch != '\n') {
            if (current_token == NULL) {
                current_token = malloc(SIZE * sizeof(char));
                memset(current_token, 0, SIZE);
            }
            size_t len = strlen(current_token);
            current_token[len] = ch;
            current_token[len + 1] = '\0';
        } else if (ch == ' ' || ch == '\t' || ch == '\n') { // if space, tab, pr newline finalize the token
            if (current_token != NULL) {
                tokens[*token_count] = current_token;
                (*token_count)++;
                current_token = NULL;
            }
        } else if (ch == '#') {
            //indicates a new comment, continues until the newline character
            break;
        }

    }

    if (current_token != NULL) {
        tokens[*token_count] = current_token;
        (*token_count)++;
    }

    //print tokens to test
    /*for (int i = 0; i < *token_count; i++) {
        printf("Token %d: %s\n", i + 1, tokens[i]);
    }*/

}

//wildcards
void wildcards(char *tokens[], int *tokenCount){
    int i = 0;
    while(i < *tokenCount) {
        if(strchr(tokens[i], '*') || strchr(tokens[i], '?')){
            //found a wildcard
            DIR *dir = opendir(".");
            if(!dir){
                perror("opendir");
                return;
            }

            struct dirent *entry;
            char *pattern = tokens[i];
            char *matches[MAX_TOKENS]; //temporary array to hold matches
            int matchCount = 0;

            while((entry = readdir(dir)) != NULL){
                if(fnmatch(pattern, entry->d_name, 0) == 0){
                    matches[matchCount] = strdup(entry->d_name);
                    matchCount++;
                }
            }

            //close the damn directory my god
            closedir(dir);

            if(matchCount > 0){
                //make room for matching tokens
                int shift = matchCount - 1;
                if(*tokenCount + shift >= MAX_TOKENS){
                    fprintf(stderr, "Too many tokens.\n");
                    return;
                }

                //lord it took me a couple business days to think of this
                for(int j = *tokenCount - 1; j > i; j--){
                    tokens[j + shift] = tokens[j];
                }

                for(int j = 0; j < matchCount; j++){
                    tokens[i + j] = matches[j];
                }

                *tokenCount += shift;
                i += matchCount;
            }else{
                i++; //if theres no matches then leave the list
            }
        }else{
            i++;
        }
    }
}

void commandParser(char* tokens[], int numOfTokens, struct Command* command) {
    int i = 0;
    int argIndex = 0;
    command->executable = tokens[0];
    
    for (i = 1; i < numOfTokens; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            i++;
            if (i < numOfTokens) {
                command->inputFile = tokens[i];
            }
        } else if (strcmp(tokens[i], ">") == 0) {
            i++;
            if (i < numOfTokens) {
                command->outputFile = tokens[i];
            }
        } else if (strcmp(tokens[i], "|") == 0) {
            command->hasPipe = 1;
            command->arguments[argIndex++] = tokens[i]; //add pipe as an argument
            (command->numArgs)++;
        } else if (strcmp(tokens[i], "and") == 0) {
            command->hasAnd = 1;
            command->arguments[argIndex++] = tokens[i]; //add 'and' as an argument
            (command->numArgs)++;
        } else if (strcmp(tokens[i], "or") == 0) {
            command->hasOr = 1;
            command->arguments[argIndex++] = tokens[i]; //add 'or' as an argument
            (command->numArgs)++;
        } else {
            command->arguments[argIndex++] = tokens[i];
            (command->numArgs)++;
        }
    }
    
    //null terminate the argument list
    command->arguments[argIndex] = NULL;
}

int isBuiltInCommand(struct Command *cmd) {
    if (strcmp(cmd->executable, "cd") == 0) {
        if (cmd->numArgs > 0) {
            if (chdir(cmd->arguments[0]) != 0) {
                perror("cd");
            }
        } else {
            chdir(getenv("HOME"));
        }
        return 1;
    } else if (strcmp(cmd->executable, "pwd") == 0) {
        char cwd[SIZE];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
        return 1;
    }
    return 0;
}

int execute(struct Command *cmd) {
    if (cmd->hasPipe) {
        //find the pipe symbol in the arguments
        int pipeIndex = -1;
        for (int i = 0; i < cmd->numArgs; i++) {
            if (strcmp(cmd->arguments[i], "|") == 0) {
                pipeIndex = i;
                break;
            }
        }
        
        if (pipeIndex == -1) {
            fprintf(stderr, "Error: Pipe symbol not found in arguments\n");
            return -1;
        }
        
        //create left and right command structures
        struct Command left = {0};
        struct Command right = {0};
        
        //set left command executable and arguments
        left.executable = cmd->executable;
        for (int i = 0; i < pipeIndex; i++) {
            left.arguments[i] = cmd->arguments[i];
            left.numArgs++;
        }
        left.inputFile = cmd->inputFile;
        
        //set right command executable and arguments
        if (pipeIndex + 1 < cmd->numArgs) {
            right.executable = cmd->arguments[pipeIndex + 1];
            for (int i = pipeIndex + 2; i < cmd->numArgs; i++) {
                right.arguments[i - (pipeIndex + 2)] = cmd->arguments[i];
                right.numArgs++;
            }
            right.outputFile = cmd->outputFile;
            
            return executePipe(&left, &right);
        } else {
            fprintf(stderr, "Error: No command after pipe\n");
            return -1;
        }
    }

    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        return -1;
    }

    if (child == 0) {
        //child process
        //handle input redirection
        if (cmd->inputFile) {
            int fd = open(cmd->inputFile, O_RDONLY);
            if (fd < 0) {
                perror("open input");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        
        //handle output redirection
        if (cmd->outputFile) {
            int fd = open(cmd->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open output");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        //prepare arguments array for execvp
        char *argv[MAX_ARG + 1];
        argv[0] = cmd->executable;
        for (int i = 0; i < cmd->numArgs; i++) {
            argv[i + 1] = cmd->arguments[i];
        }
        argv[cmd->numArgs + 1] = NULL;

        execvp(cmd->executable, argv);
        perror(cmd->executable);
        exit(EXIT_FAILURE);
    }

    //parent prcess
    int status;
    waitpid(child, &status, 0);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else {
        return -1;
    }
}

int executePipe(struct Command *left, struct Command *right) {
    int pipefd[2];
    
    //create pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }
    
    //fork first child leftcommand
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    
    if (pid1 == 0) {
        //first child process - left command
        //close read end of pipe
        close(pipefd[0]);
        
        //redirect stdout to write end of pipe
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        
        //close the write end of pipe after redirection
        close(pipefd[1]);
        
        //handle input redirection for left command if specified
        if (left->inputFile != NULL) {
            int fd = open(left->inputFile, O_RDONLY);
            if (fd == -1) {
                perror("open input file");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(fd);
        }
        
        //prepare arguments for left command
        char *argv[MAX_ARG + 1];
        argv[0] = left->executable;
        for (int i = 0; i < left->numArgs; i++) {
            argv[i + 1] = left->arguments[i];
        }
        argv[left->numArgs + 1] = NULL;
        
        //execute left command
        execvp(left->executable, argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    
    //fork second child right command
    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        kill(pid1, SIGTERM);
        waitpid(pid1, NULL, 0);
        return -1;
    }
    
    if (pid2 == 0) {
        //in second child process right command
        //close write end of pipe
        close(pipefd[1]);
        
        //redirect stdin to read end of pipe
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        
        //close the read end of pipe after redirection
        close(pipefd[0]);
        
        //handle output redirection for right command if specified
        if (right->outputFile != NULL) {
            int fd = open(right->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("open output file");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(fd);
        }
        
        //prepare arguments for right command
        char *argv[MAX_ARG + 1];
        argv[0] = right->executable;
        for (int i = 0; i < right->numArgs; i++) {
            argv[i + 1] = right->arguments[i];
        }
        argv[right->numArgs + 1] = NULL;
        
        //execute right command
        execvp(right->executable, argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    
    //parent process
    //close both ends of the pipe in the parent
    close(pipefd[0]);
    close(pipefd[1]);
    
    //wait for both child processes to complete
    int status1, status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);
    
    //return exit status of the right command
    if (WIFEXITED(status2)) {
        return WEXITSTATUS(status2);
    } else {
        return -1;
    }
}

void andOrHandler(struct Command *cmd) {
    if (cmd->hasAnd || cmd->hasOr) {
        int condIndex = -1;
        for (int i = 0; i < cmd->numArgs; i++) {
            if ((cmd->hasAnd && strcmp(cmd->arguments[i], "and") == 0) ||
                (cmd->hasOr && strcmp(cmd->arguments[i], "or") == 0)) {
                condIndex = i;
                break;
            }
        }

        struct Command left = {0}, right = {0};

        //left command
        left.executable = cmd->executable;
        for (int i = 0; i < condIndex; i++) {
            left.arguments[i] = cmd->arguments[i];
            left.numArgs++;
        }
        left.arguments[left.numArgs] = NULL;
        left.inputFile = cmd->inputFile;

        //right command
        right.executable = cmd->arguments[condIndex + 1];
        for (int i = condIndex + 2; i < cmd->numArgs; i++) {
            right.arguments[i - (condIndex + 2)] = cmd->arguments[i];
            right.numArgs++;
        }
        right.arguments[right.numArgs] = NULL;
        right.outputFile = cmd->outputFile;

        int exitStatus = execute(&left);
        if ((cmd->hasAnd && exitStatus == 0) || (cmd->hasOr && exitStatus != 0)) {
            execute(&right);
        }

    } else {
        execute(cmd);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        //batch mode with a specified file
        FILE *file = fopen(argv[1], "r");
        if (file == NULL) {
            perror("Error opening file");
            return EXIT_FAILURE;
        }
        batchMode(file);
        fclose(file);
    } else if (isatty(STDIN_FILENO)) {
        //interactive mode
        interactiveMode();
    } else {
        //batch mode from piped input
        batchMode(stdin);
    }

    return EXIT_SUCCESS;
}

