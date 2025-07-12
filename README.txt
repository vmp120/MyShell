Authors: Himani Mehta & Vaishvi Patel
NetID: hjm89 vmp120

TEST PLAN:
METHODS TO BE IMPLEMENTED
interactive mode & batch mode working
tokenizer - done, still need to handle wildcards
 - edgecases: just space (done), comments (done)
command parser - done
wildcards - done
redirection - done
execute - done handling child and parent processes

1. Input Handling - check isatty() for interactive/batch mode, read() to ensure are read and processed
2. Tokenizer() - parse command line into tokens, split and handle comments
3. Command Execution - check if the command is built in or external
4. Redirection Handling
    a. Input Redirection < - Look through the list of token and find <. Once it finds this token, program should reads from specificed file rather than standard input. Use dup2() to redirect input to file descriptor. REMEMBER TO CLOSE
    b. Output Redirection > - ^^Same thing but creating the file for write only rather than read only AND CLOSE
    c. DON'T FORGET TO REMOVE THE <> FROM THE TOKEN LIST 
    d. also create a new file for opening if one doesn't exist
    e. Pipelines | - Look through the list of tokens and find |. Once it finds this token, program generates pipe using pipe(), and then uses dup2() to set the standard output for the right side of the pipe, standard input for left side of the pipe. Two child processes are generated for the left and right side of the pipeline for the two commands using fork(), and both sides are executed.
    f. Conditionals and/or - Similar process to the pipe, but first the left process is either executed or not executed. For and, if the left command executed (true), then the right command is executed. For or, if the left command failed (false), then the right command is executed.

TESTING STRATEGY AND CASES:
When writing the program we initially started by writing the tokenizer and command parser methods. 
By making sure they worked individually and together, and then accomodating for other edge cases like just having a space for input or comments, we could then go forward and make adjustments for piping and interactive vs batch mode.
From here we knew we had to work on methods for piping, executing piping, redirections, conditionals, and also built in commands.
Functions were designed seperately for piping, conditionals, wildcards, and built-in commands to make it easier for debugging and testing.
While handling built in commands and accomadating for different modes would be smaller tasks to do, we also had to make sure things worked in both modes.
This was tested and debugged through several print statements in each method, to do things like make sure the tokens were correct, pipes were working both ways, and more.

