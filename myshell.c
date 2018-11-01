
/****************************************************************
 * Name         in  Ting Yin, 918161925                            *
 * Class        in  CSC 415                                        *
 * Date         in  Jul 6, 2018                                    *
 * Description  in   Writting a simple bash shell program          *
 *                that will execute simple commands. The main   *
 *                goal of the assignment is working with        *
 *                fork, pipes and exec system calls.            *
 ****************************************************************/
//May add more includes
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/wait.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>

//Max amount allowed to read from input
#define BUFFERSIZE 256
//Shell prompt
#define PROMPT "myshell >> "
//sizeof shell prompt
#define PROMPTSIZE sizeof(PROMPT)

#define ARGVMAX 32 
#define ARGSIZEMAX 32


char* buffer[BUFFERSIZE]; 
//int f = 0;   //to save change in directory
int start = 0, end = 0;
 
 
//shell0 in  initialize variables and then go into an infinite loop 
//until stdin detects EOF (i.e. the user enters CTL-­­D).
void 
parse(char* file, char** myargv)//argv
{
    const char* delimeter = " \t";
    int myargc = 0; 
    memset(myargv, 0, sizeof(char*)*ARGSIZEMAX);
    //fills the first 32 bytes of the memory area pointed to by myargv with the constant byte 0
    
    char* saveptr = NULL;
    while (1)
    {
        char* token = strtok_r(file, delimeter, &saveptr);
        if (token == NULL)
        {
            break;
        }
        myargv[myargc] = token;// store each token of cmd divided by " /t" in myargv
        file = saveptr;

        myargc++;
        }
} 
 
char* 
ctrim(char* cmd)//remove extra spaces
{
    int i = 0;
    int j = 0;
    char* ptr = malloc(sizeof(char*)*strlen(cmd));
    for (i = 0; cmd[i] != '\0'; i++)
    if (cmd[i] != ' ')
    {
        ptr[j] = cmd[i];
        j++;
    }
    ptr[j] = '\0';
    cmd = ptr;
    return cmd;
}
 
 
//create child process
void 
execute(char** myargv)
{
    if (strcmp(myargv[0], "exit") == 0)
        exit(0);
    
    if (strcmp(myargv[0], "cd") == 0)
    {
        int ch = chdir(myargv[1]);
        return; 
    }

    pid_t pid;
    int status;
    if ((pid = fork()) < 0)
    {
        printf("error in fork.\n");
        exit(1);
    }
    else if (pid == 0)
    {        
        //if (execvp(myargv[0], myargv) < 0 && strcmp(myargv[0], "cd"))
        if (execvp(myargv[0], myargv) < 0)
            printf("invalid command.\n");
        exit(0);
    }
    else
    {
        while (wait(&status) != pid)
            ;
    }
}
 
//IO redirection 
void  
exeFile(char** myargv, char* output)
{
    pid_t pid;
    int status, flag;
    char* file = NULL;
    if ((pid = fork()) < 0)
    {
        printf("error in fork.\n");
        exit(1);//fclose(fd1);
    }
    else if (pid == 0)
    {
        if (strstr(output, ">")>0)
        {
            strtok_r(output, ">", &file);
            ++output;
            file = ctrim(file);
            flag = 1;
            int old_stdout = dup(1);
            FILE* f1 = freopen(output, "w+", stdout);
            exeFile(myargv, file);
            fclose(stdout);
            FILE* f2 = fdopen(old_stdout, "w");
            *stdout = *f2;
            exit(0);
        }

        if (strstr(output, ">>")>0)
        {
            strtok_r(output, ">>", &file);
            output += 1; 
            file = ctrim(file);
            flag = 1;
            int old_stdout = dup(1);
            FILE* f1 = freopen(output, "a+", stdout);
            exeFile(myargv, file);
            fclose(stdout);
            FILE* f2 = fdopen(old_stdout, "a+");
            *stdout = *f2;
            exit(0);
        }

        if (strstr(output, "<") > 0)
        {
            strtok_r(output, "<", &file);
            file = ctrim(file);
            flag = 1;
            int fd = open(file, O_RDONLY);
            if (fd<0)
            {
                exit(0);
            }
        }

        int old_stdout = dup(1);
        FILE* f1 = freopen(output, "w+", stdout);
        if (execvp(myargv[0], myargv) < 0)
            printf("error in execvp\n");
        fclose(stdout);
        FILE* f2 = fdopen(old_stdout, "w");
        *stdout = *f2;
        exit(0);
    }
    else
    {
        while (wait(&status) != pid)
            ;
    }
}
 
void  
exeInput(char** myargv, char* output)
{
    pid_t pid;
    int fd;
    char* file;
    int flag = 0;
    int status;
    if ((pid = fork()) < 0)
    {
        printf("error in fork\n");
        exit(1);
    }
    else if (pid == 0)
    {
        if (strstr(output, "<")>0)
        {
            char* p = strtok_r(output, "<", &file);
            file = ctrim(file);
            flag = 1;
            fd = open(output, O_RDONLY);
            if (fd<0)
            {
                exit(0);
            }
            output = file;
        }
        if (strstr(output, ">")>0)
        {
            char* p = strtok_r(output, ">", &file);
            file = ctrim(file);
            flag = 1;
            fflush(stdout);
            fflush(stdout);
            int old_stdout = dup(1);
            FILE* f1 = freopen(file, "w+", stdout);
            exeInput(myargv, output);
            fclose(stdout);
            FILE* f2 = fdopen(old_stdout, "w");
            *stdout = *f2;
            exit(0);
        }
        if (strstr(output, "|") > 0)
        {
            char* p = strtok_r(output, "|", &file);
            file = ctrim(file);
            flag = 1;
            char* args32;
            parse(file, myargv);
            int pfds[2];
            pid_t pid, pid2;
            int status, status2;
            pipe(pfds);
            int fl = 0;
            if ((pid = fork()) < 0)
            {
                printf("error in fork\n");
                exit(1);
            }
            if ((pid2 = fork()) < 0)
            {
                printf("error in fork\n");
                exit(1);
            }
            if (pid == 0 && pid2 != 0)
            {
                close(1);
                dup(pfds[1]);
                close(pfds[0]);
                close(pfds[1]);
                fd = open(output, O_RDONLY);
                close(0);
                dup(fd);
                if (execvp(myargv[0], myargv) < 0)
                {
                    close(pfds[0]);
                    close(pfds[1]);
                    printf("error in execvp \n");
                    fl = 1;
                    exit(0);
                }
                close(fd);
                exit(0);
            }
            else if (pid2 == 0 && pid != 0 && fl != 1)
            {
                close(0);
                dup(pfds[0]);
                close(pfds[1]);
                close(pfds[0]);
                if (execvp(myargv[0], myargv) < 0)
                {
                    close(pfds[0]);
                    close(pfds[1]);
                    printf("error in execvp\n");
                    exit(0);
                }
            }
            else
            {
                close(pfds[0]);
                close(pfds[1]);
                while (wait(&status) != pid);
                while (wait(&status2) != pid2);
            }
            exit(0);
        }
        fd = open(output, O_RDONLY);
        close(0);
        dup(fd);
        if (execvp(myargv[0], myargv) < 0)
        {
            printf("error in execvp\n");
        }
        close(fd);
        exit(0);
    }
    else
    {
        while (wait(&status) != pid);
    }
 
}

void 
exePipe(char** myargv, char* output)
{
    int pfds[2], pf[2], flag;
    char* file;
    pid_t pid, pid2, pid3;
    int status, status2, old_stdout;
    pipe(pfds);
    int i = 0;
    char* args[ARGVMAX];
    char* argp[ARGVMAX];
    int fl = 0;
    if ((pid = fork()) < 0)
    {
        printf("error in fork\n");
        exit(1);
    }
    if ((pid2 = fork()) < 0)
    {
        printf("error in fork\n");
        exit(1);
    }
    if (pid == 0 && pid2 != 0)
    {
        close(1);
        dup(pfds[1]);
        close(pfds[0]);
        close(pfds[1]);
        if (execvp(myargv[0], myargv) < 0)
        {
            close(pfds[0]);
            close(pfds[1]);
            printf("error in execvp\n");
            fl = 1;
            kill(pid2, SIGUSR1);
            exit(0);
        }
    }
    else if (pid2 == 0 && pid != 0)
    {
        if (fl == 1){ exit(0); }
        if (strstr(output, "<") > 0)
        {
            char* p = strtok_r(output, "<", &file);
            file = ctrim(file);
            flag = 1;
            parse(output, args);
            exeInput(args, file);
            close(pfds[0]);
            close(pfds[1]);
            exit(0);
        }
        if (strstr(output, ">") > 0)
        {
            char* p = strtok_r(output, ">", &file);
            file = ctrim(file);
            flag = 1;
            parse(output, args);
            fl = 1;
        }
 
        else
        {
            parse(output, args);
        }
        close(0);
        dup(pfds[0]);
        close(pfds[1]);
        close(pfds[0]);
        if (i == 1)
        {
            old_stdout = dup(1);
            FILE* f1 = freopen(file, "w+", stdout);
        }
        if (execvp(args[0], args) < 0)
        {
            fflush(stdout);
            printf("error in execvp\n %d\n", pid);
            kill(pid, SIGUSR1);
            close(pfds[0]);
            close(pfds[1]);
        }
        fflush(stdout);
        if (i == 1)
        {
            fclose(stdout);
            FILE* f2 = fdopen(old_stdout, "w");
            *stdout = *f2;
        }
    }
    else
    {
        close(pfds[0]);
        close(pfds[1]);
        while (wait(&status) != pid);
        while (wait(&status2) != pid2);
    }
}

void 
exePipe2(char** myargv, char** args, char** argp)
{
    int status;
    int i;
    int pipes[4];
    pipe(pipes);
    pipe(pipes + 2);
    if (fork() == 0)
    {
        dup2(pipes[1], 1);
        close(pipes[0]);
        close(pipes[1]);
        close(pipes[2]);
        close(pipes[3]);
        if (execvp(myargv[0], myargv) < 0)
        {
            fflush(stdout);
            printf("error in execvp\n");
            fflush(stdout);
            close(pipes[0]);
            close(pipes[1]);
            close(pipes[2]);
            close(pipes[3]);
            exit(1);
        }
    }
    else
    {
        if (fork() == 0)
        {
            dup2(pipes[0], 0);
            dup2(pipes[3], 1);
            close(pipes[0]);
            close(pipes[1]);
            close(pipes[2]);
            close(pipes[3]);
            if (execvp(args[0], args) < 0)
            {
                fflush(stdout);
                printf("error in execvp\n");
                fflush(stdout);
                close(pipes[0]);
                close(pipes[1]);
                close(pipes[2]);
                close(pipes[3]);
                exit(1);
            }
        }
        else
        {
            if (fork() == 0)
            {
                dup2(pipes[2], 0);
                close(pipes[0]);
                close(pipes[1]);
                close(pipes[2]);
                close(pipes[3]);
                if (execvp(argp[0], argp) < 0)
                {
                    fflush(stdout);
                    printf("error in execvp\n");
                    fflush(stdout);
                    close(pipes[0]);
                    close(pipes[1]);
                    close(pipes[2]);
                    close(pipes[3]);
                    exit(1);
                }
            }
        }
    }
    close(pipes[0]);
    close(pipes[1]);
    close(pipes[2]);
    close(pipes[3]);
    for (i = 0; i < 3; i++)
        wait(&status);
}

int  
main(int argc, char** argv)
{
    //char line[1024];
    char* myargv[ARGVMAX];
    char* args[ARGVMAX];
    char* saveptr;
    size_t size = 0;
    char ch;
    int myargc = 0;
    char* tri;
    char* second;
    char* file;
    int i;
    for (i = 0; i < BUFFERSIZE; i++)
    {
        buffer[i] = (char*)malloc(150);
    }
 
    while (1)
    {
        myargc = 0;
        int flag = 0;
        char* tok = NULL;
        //char* dire[] = { "pwd" };
        fflush(stdout);
        printf(PROMPT);
        fflush(stdout);
        int len = getline(&tok, &size, stdin);
        if (*tok == '\n')
            continue;
        tok[len - 1] = '\0';
        char* file = NULL;
        int i = 0;
        char* temp = (char*)malloc(150);
        strcpy(temp, tok);
        parse(temp, myargv);
 
        strcpy(buffer[(start + 1) % BUFFERSIZE], tok); 
        start = (start + 1) % BUFFERSIZE;
        end = end + 1;
 
        for (i = 0; tok[i] != '\0'; i++)
        {
 
            if (tok[i] == '>')
            {
                char* p = strtok_r(tok, ">", &file);
                file = ctrim(file);
                flag = 1;
                break;
            }
            else if (tok[i] == '<')
            {
                char* p = strtok_r(tok, "<", &file);
                file = ctrim(file);
                flag = 2;
                break;
            }
            else if (tok[i] == '|')
            {
                char* p = strtok_r(tok, "|", &saveptr);
                flag = 3;
                break;
            }
        }

        if (flag == 1)
        {
            parse(tok, myargv);   //parsed command stored in myargv
            exeFile(myargv, file);
        }
        else if (flag == 2)
        {
            parse(tok, myargv);
            exeInput(myargv, file);
        }
        else if (flag == 3)
        {
            char* argp[ARGVMAX];
            char* output, *file; 
            if (strstr(saveptr, "|") > 0)
            {
                char* p = strtok_r(saveptr, "|", &file);
                parse(tok, myargv);
                parse(saveptr, args);
                parse(file, argp);
                exePipe2(myargv, args, argp);
            }
            else
            {
                parse(tok, myargv);
                exePipe(myargv, saveptr);
            }
        }
        else
        {
            parse(tok, myargv);
            execute(myargv);
        }
    }
}
