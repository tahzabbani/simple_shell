/*
* Name: Tahseen Robbani
* BlazerID: tar0025
* Project #: 3
*
*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <readline/readline.h>

#define BUFFSIZE 1024

char **cmd;
char *stored[BUFSIZ];
long storedPid[BUFSIZ];
int storedIndex = 0;
int pidIndex = 0;
long *gottenPid;

static void sig_usr(int signo) {
    int status;
    switch(signo) {
        case SIGINT: 
            printf(" received SIGINT for child process %ld\n", *gottenPid);
            break;

        case SIGTSTP:
            printf(" received SIGTSTP signal for child process %ld\n", *gottenPid);
            stored[storedIndex] = cmd[0];
            storedPid[storedIndex] = *gottenPid;
            storedIndex++;
            break;
            
        case SIGCHLD: // go back to child
                waitpid(-1, &status, WNOHANG);
                break;

        default:
            printf("received signal %d\n", signo);
    }
}


int main(int argc, char *argv[]) {
    char *prompt = "blazersh> ";
    char *user_in;
    char **historylog = malloc(100 * sizeof(char*));
    int cmdIndex = 0; // for displaying history loop

    char cwd[BUFFSIZE];

    pid_t pid;
    int i;

    getcwd(cwd, sizeof(cwd)); // for singular cd

    FILE *fp = fopen("blazersh.log", "w+");

    while(1) {
        cmd = malloc(8 * sizeof(char*));
        char *token;
        char *delim = " ";
        int index = 0;
        int in = 0;
        int out = 0;
        char input[BUFFSIZE], output[BUFFSIZE];

        user_in = readline(prompt);
        
        // separate commands
        token = strtok(user_in, delim);
        while (token != NULL) {
            cmd[index] = token;
            index++;
            token = strtok(NULL, delim);
        }

        cmd[index] = NULL;

        if (!cmd[0]) {
            free(cmd);
            continue;
        }

        for(i = 0; i < index; i++)
        {
            if(strcmp(cmd[i],"<")==0)
            {        
                strcpy(input, cmd[i+1]);
                in = 1;           
            }               

            if(strcmp(cmd[i],">")==0)
            {      
                strcpy(output, cmd[i+1]);
                out = 1;
            }         
        }

        if (strcmp(cmd[0], "jobs") == 0) {
            printf("Process\t\tPID\n");
            for (i = 0; i < storedIndex; i++) {
                printf("%s\t\t%ld\n", stored[i], storedPid[i]);
            }
            historylog[cmdIndex] = cmd[0];
            cmdIndex++;
            continue;
        }

        if (strcmp(cmd[0], "continue") == 0) {
            if(kill(atoi(cmd[1]), SIGCONT) == 1) {
                perror("error with kill");
                exit(-1);
            }
            historylog[cmdIndex] = cmd[0];
            cmdIndex++;
            continue;
        }

        if (strcmp(cmd[0], "help") == 0) {
            printf("list - list all the files in the current directory.\n");
            printf("cd - cd <directory> – change the current directory to the <directory>.\n");
            printf("help - display the internal commands and a short description on how to use them.\n");
            printf("quit - quit the shell program.\n");
            printf("history - display all the previous command entered into the shell program.\n");
            printf("jobs - lists the processes along with their corresponding process id that were stopped when the user enters Control-Z\n");
            printf("continue <pid> -  sends the continue signal to the process with process id <pid>\n");
            fprintf(fp, "%s\n", cmd[0]);
            historylog[cmdIndex] = cmd[0];
            cmdIndex++;
            continue;
        }

        if (strcmp(cmd[0], "cd") == 0) {
            if (cmd[1]) {
                chdir(cmd[1]);
                fprintf(fp,"%s %s\n", cmd[0], cmd[1]);
                historylog[cmdIndex] = cmd[0];
                cmdIndex++;
            }
            else {
                chdir(cwd);
            }
            continue;
        }

        if (strcmp(cmd[0], "list") == 0) {
            cmd[0] = "ls";
            fprintf(fp, "list\n");
            historylog[cmdIndex] = "list";
            cmdIndex++;
        }

        if (strcmp(cmd[0], "quit") == 0) {
            printf("Exiting shell...\n");
            fprintf(fp, "%s\n", cmd[0]);
            historylog[cmdIndex] = cmd[0];
            cmdIndex++;
            exit(1);
        }

        if (strcmp(cmd[0], "history") == 0) {
            fprintf(fp, "%s\n", cmd[0]);
            historylog[cmdIndex] = cmd[0];
            cmdIndex++;
            for (i = 0; i < cmdIndex; i++) {
                printf("%s\n", historylog[i]);
            }
            continue;
        }

        gottenPid = mmap(NULL, sizeof *gottenPid, PROT_READ | PROT_WRITE, // makes gotten pid shareable
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

        pid = fork(); // FORK
        if (pid == 0) {
            *gottenPid = (long)getpid();

        if(in)
        {   
            int fd0;
            if ((fd0 = open(input, O_CREAT | O_RDONLY, 0)) < 0) {
                perror("Couldn't open input file");
                exit(0);
            }           
            // dup2() copies content of fdo in input of preceeding file
            dup2(fd0, 0);

            close(fd0); 
        }

        //if '>' char was found in string inputted by user 
        if (out)
        {

            int fd1 ;
            if ((fd1 = open(output, O_CREAT | O_APPEND | O_WRONLY, 0755)) == -1) {
                perror("Couldn't open the output file");
                exit(0);
            }           

            dup2(fd1, 1);
            close(fd1);
        }

            execvp(cmd[0], cmd);
            printf("Exec failed if you see this\n");
            exit(-1);
        }
        else if (pid > 0) {

            if (signal(SIGINT, sig_usr) == SIG_ERR) { // signal handlers for ^C and ^Z
                printf("Unable to catch SIGINT");
                exit(-1);
            } 
            
            if (signal(SIGTSTP, sig_usr) == SIG_ERR) {
                printf("Unable to catch SIGTSTP");
                exit(-1);
            } 
            if (signal(SIGCHLD, sig_usr) == SIG_ERR) {
                printf("Unable to catch SIGCHLD");
                exit(-1);
            }

        
            pause(); 

        }
        else {
            perror("fork"); 
            exit(EXIT_FAILURE);
        }

        free(cmd);
        
    }
    fclose(fp);
    return 0;

}