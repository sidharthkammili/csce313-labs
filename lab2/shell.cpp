/****************
LE2: Introduction to Unnamed Pipes
****************/
#include <unistd.h> // pipe, fork, dup2, execvp, close
#include <sys/wait.h> // wait, waitpid
#include <cstdlib> // exit
using namespace std;

int main () {
    // lists all the files in the root directory in the long format
    char cmd1_0[] = "ls";                                                
    char cmd1_1[] = "-al";                                           
    char cmd1_2[] = "/";                                             
    char* cmd1[]   = { cmd1_0, cmd1_1, cmd1_2, nullptr };
    // translates all input from lowercase to uppercase
    char cmd2_0[] = "tr";                                      
    char cmd2_1[] = "a-z";                                   
    char cmd2_2[] = "A-Z";                               
    char* cmd2[]   = { cmd2_0, cmd2_1, cmd2_2, nullptr };

    // TODO: add functionality
    // Create unidirectional pipe: fd[0] = read end; fd[1] = write end
    int fd[2];               //file descriptors for pipe
    if (pipe(fd) == -1) {    //create unnamed pipe
        return 1;            //if pipe creation fails, exit with error
    }

    // Create child #1 to run first command
    // In child, redirect output to write end of pipe
    // Close the read end of the pipe on the child side.
    // In child, execute the command
    pid_t c1 = fork();      //fork first child
    if (c1 == -1) {         //check fork error
        close(fd[0]);       //close read end on error path
        close(fd[1]);       //close write end on error path
        return 1;           //exit with an error
    }
    if (c1 == 0) { //for child 1
        if (dup2(fd[1], STDOUT_FILENO) == -1) {  //redirect child's stdout to pipe's write end
            _exit(1);                            //if dup2 fails, exit child
        }
        close(fd[0]);       //child #1 does NOT read: close read end
        close(fd[1]);       //close original write fd because stdout now points to pipe

        execvp(cmd1[0], cmd1);   //replace child #1 image with "ls -al /"
        _exit(127);              //if execvp returns, it failed → exit child
        
    }

    // Create another child #2 to run second command
    // In child, redirect input to the read end of the pipe
    // Close the write end of the pipe on the child side.
    // Execute the second command.
    pid_t c2 = fork();                      //fork the second child
    if (c2 == -1) {                         //check fork error
        close(fd[0]);                       //close read end on error path
        close(fd[1]);                       //close write end on error path
        (void)waitpid(c1, nullptr, 0);      //wait for child #1
        return 1;                           //exit with error
    }

    if (c2 == 0) {
        if (dup2(fd[0], STDIN_FILENO) == -1) {   //redirect child's stdin to pipe's read end
            _exit(1);                            //if dup2 fails, exit child 
        }
        close(fd[1]);                            //child #2 does NOT write: close write end
        close(fd[0]);                            //close OG read fd (stdin now points to pipe) 

        execvp(cmd2[0], cmd2);                   //replace child #2 image with "tr a-z A-Z"
        _exit(127);                              //if execvp returns failure then exit child
    }

    // Reset the input and output file descriptors of the parent.
}
