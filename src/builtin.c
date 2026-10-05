#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "jobs.h"
#include "builtin.h"
#include "job_control.h"


static int builtin_cd(command_t *cmd)
{
    const char *directory;

    if (cmd->argc == 1)
    {
        directory = getenv("HOME");

        if (directory == NULL)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return -1;
        }
    }
    else if (cmd->argc == 2)
    {
        directory = cmd->argv[1];
    }
    else
    {
        fprintf(stderr, "cd: too many arguments\n");
        return -1;
    }

    if (chdir(directory) != 0)
    {
        perror("cd");
        return -1;
    }

    return 0;
}


static int builtin_pwd(command_t *cmd)
{
    char current_directory[4096];

    if (cmd->argc > 1)
    {
        fprintf(stderr, "pwd: too many arguments\n");
        return -1;
    }

    if (getcwd(current_directory,
               sizeof(current_directory)) == NULL)
    {
        perror("pwd");
        return -1;
    }

    printf("%s\n", current_directory);

    return 0;
}


static int builtin_echo(command_t *cmd)
{
    for (int i = 1; i < cmd->argc; i++)
    {
        printf("%s", cmd->argv[i]);

        if (i < cmd->argc - 1)
            printf(" ");
    }

    printf("\n");

    return 0;
}


/* fork command */

static int builtin_fork(command_t *cmd)
{
    pid_t pid;

    if (cmd->argc > 1)
    {
        printf("fork: no arguments required\n");
        return 1;
    }

    fflush(stdout);

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        printf("Child process created\n");
        printf("Child PID: %d\n", getpid());
        printf("Child is executing...\n");

        sleep(1);

        printf("Child execution completed\n");

        fflush(stdout);

        _exit(0);
    }

    printf("Parent process\n");
    printf("Child PID: %d\n", pid);

    fflush(stdout);

    waitpid(pid, NULL, 0);

    return 0;
}


/* forkexec command */

static int builtin_forkexec(command_t *cmd)
{
    pid_t pid;

    if (cmd->argc > 1)
    {
        printf("forkexec: no arguments required\n");
        return 1;
    }

    fflush(stdout);

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        char *args[] = {"ls", NULL};

        printf("Child process created\n");
        printf("Child PID: %d\n", getpid());
        printf("Child executing: ls\n");

        fflush(stdout);

        execvp("ls", args);

        perror("forkexec");

        _exit(1);
    }

    printf("Parent process\n");
    printf("Child PID: %d\n", pid);

    fflush(stdout);

    waitpid(pid, NULL, 0);

    printf("Child execution completed\n");

    return 0;
}


/* sleep command */

static int builtin_sleep(command_t *cmd)
{
    int seconds;

    if (cmd->argc != 2)
    {
        printf("sleep: usage: sleep <seconds>\n");
        return 1;
    }

    seconds = atoi(cmd->argv[1]);

    if (seconds < 0)
    {
        printf("sleep: invalid time\n");
        return 1;
    }

    printf("Process sleeping for %d second(s)...\n",
           seconds);

    fflush(stdout);

    sleep((unsigned int)seconds);

    printf("Sleep completed\n");

    return 0;
}


/* exit */

static int builtin_exit(command_t *cmd)
{
    if (cmd->argc > 1)
    {
        fprintf(stderr,
                "exit: too many arguments\n");

        return -1;
    }

    return 1;
}


/* check builtin */

int is_builtin(const command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
        return 0;

    if (strcmp(cmd->argv[0], "cd") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "pwd") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "echo") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "fork") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "forkexec") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "sleep") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "exit") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "jobs") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "fg") == 0)
        return 1;

    if (strcmp(cmd->argv[0], "bg") == 0)
        return 1;

    return 0;
}


/* execute builtin */

int execute_builtin(command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
        return -1;

    if (strcmp(cmd->argv[0], "cd") == 0)
        return builtin_cd(cmd);

    if (strcmp(cmd->argv[0], "pwd") == 0)
        return builtin_pwd(cmd);

    if (strcmp(cmd->argv[0], "echo") == 0)
        return builtin_echo(cmd);

    if (strcmp(cmd->argv[0], "fork") == 0)
        return builtin_fork(cmd);

    if (strcmp(cmd->argv[0], "forkexec") == 0)
        return builtin_forkexec(cmd);

    if (strcmp(cmd->argv[0], "sleep") == 0)
        return builtin_sleep(cmd);

    if (strcmp(cmd->argv[0], "exit") == 0)
        return builtin_exit(cmd);

    if (strcmp(cmd->argv[0], "jobs") == 0)
        return builtin_jobs(cmd);

    if (strcmp(cmd->argv[0], "fg") == 0)
        return builtin_fg(cmd);

    if (strcmp(cmd->argv[0], "bg") == 0)
        return builtin_bg(cmd);

    return -1;
}


/* jobs */

int builtin_jobs(command_t *cmd)
{
    (void)cmd;

    jobs_print();

    return 0;
}


/* fg */

int builtin_fg(command_t *cmd)
{
    int job_id;
    job_t *job;
    int status;

    if (cmd->argc < 2)
    {
        printf("fg: usage: fg <job number>\n");
        return 1;
    }

    job_id = atoi(cmd->argv[1]);

    job = job_find(job_id);

    if (job == NULL)
    {
        printf("fg: no such job: %d\n",
               job_id);

        return 1;
    }

    give_terminal_to(job->pgid);

    if (job->state == JOB_STOPPED)
    {
        kill(-job->pgid, SIGCONT);
    }

    job_continue(job->pgid);

    while (1)
    {
        pid_t result =
            waitpid(-job->pgid,
                    &status,
                    WUNTRACED);

        if (result < 0)
            break;

        if (WIFSTOPPED(status))
        {
            job_stop(job->pgid);
            break;
        }

        if (WIFEXITED(status) ||
            WIFSIGNALED(status))
        {
            job_done(job->pgid);
            break;
        }
    }

    take_terminal_back();

    if (job->state == JOB_DONE)
        job_remove(job_id);

    return 0;
}


/* bg */

int builtin_bg(command_t *cmd)
{
    int job_id;
    job_t *job;

    if (cmd->argc < 2)
    {
        fprintf(stderr,
                "bg: usage: bg <job number>\n");

        return 1;
    }

    job_id = atoi(cmd->argv[1]);

    job = job_find(job_id);

    if (job == NULL)
    {
        fprintf(stderr,
                "bg: job not found: %d\n",
                job_id);

        return 1;
    }

    if (kill(-job->pgid, SIGCONT) == -1)
    {
        perror("bg: kill");
        return 1;
    }

    job_continue(job->pgid);

    printf("[%d] %s &\n",
           job->job_id,
           job->command);

    return 0;
}
