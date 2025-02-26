#include "lab.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>

#define lab_VERSION_MAJOR 1
#define lab_VERSION_MINOR 0

char *get_prompt(const char *env)
{
    char *prompt = getenv(env);
    if (prompt == NULL)
    {
        prompt = "shell>";
    }
    char *result = (char *)malloc(strlen(prompt) + 1);
    if (result != NULL)
    {
        strcpy(result, prompt);
    }
    return result;
}

int change_dir(char **dir)
{
    if (dir[1] == NULL)
    {
        return chdir(getenv("HOME"));
    }
    return chdir(dir[1]);
}

char **cmd_parse(char const *line)
{
    int size = 64;
    int position = 0;
    char **tokens = malloc(size * sizeof(char *));
    if (tokens == NULL)
    {
        fprintf(stderr, "Memory allocation failed for tokens\n");
        return NULL;
    }

    char *token;
    char *line_copy = strdup(line);
    if (line_copy == NULL)
    {
        fprintf(stderr, "Memory allocation failed for line copy\n");
        free(tokens);
        return NULL;
    }

    // Tokenize the input line
    token = strtok(line_copy, " \t\r\n\a");
    while (token != NULL)
    {
        // Check if exceed the maximum allowed arguments
        if (position >= sysconf(_SC_ARG_MAX))
        {
            fprintf(stderr, "Error: Too many arguments\n");
            free(line_copy);
            free(tokens);
            return NULL;
        }

        // Expand the tokens array if necessary
        if (position >= size)
        {
            size += 64;
            char **temp = realloc(tokens, size * sizeof(char *));
            if (temp == NULL)
            {
                fprintf(stderr, "Memory allocation failed\n");
                free(line_copy);
                free(tokens);
                return NULL;
            }
            tokens = temp;
        }

        // Allocate memory for the token and copy it
        tokens[position] = strdup(token);
        if (tokens[position] == NULL)
        {
            fprintf(stderr, "Memory allocation failed for token\n");
            free(line_copy);
            free(tokens);
            return NULL;
        }
        position++;

        // Get the next token
        token = strtok(NULL, " \t\r\n\a");
    }

    // Null-terminate the token list
    tokens[position] = NULL;

    free(line_copy);
    return tokens;
}

void cmd_free(char **line)
{
    if (line == NULL)
        return;

    for (int i = 0; line[i] != NULL; i++)
    {
        free(line[i]);
    }
    free(line);
}

char *trim_white(char *line)
{
    while (isspace(*line))
    {
        line++;
    }

    if (*line == 0)
    {
        return line;
    }

    char *end = line + strlen(line) - 1;
    while (end > line && isspace(*end))
    {
        end--;
    }

    end[1] = '\0';
    return line;
}

bool do_builtin(struct shell *sh, char **argv)
{
    UNUSED(sh);
    if (strcmp(argv[0], "-v") == 0)  
    {
        printf("Shell version %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
        exit(EXIT_SUCCESS);  
    }
    if (strcmp(argv[0], "exit") == 0)
    {
        exit(EXIT_SUCCESS);
    } 
    else if (strcmp(argv[0], "cd") == 0)
    {
        if (argv[1] == NULL)
        {
            // No argument, change to home directory
            char *home = getenv("HOME");
            if (home == NULL)
            {
                struct passwd *pw = getpwuid(getuid());
                home = pw->pw_dir;
            }
            chdir(home);
        }
        else
        {
            if (chdir(argv[1]) != 0)
            {
                perror("cd failed");
            }
        }
        return true; 
    }
    else if (strcmp(argv[0], "history") == 0)
    {
        // Print command history
        HIST_ENTRY **hist_list = history_list();
        for (int i = 0; hist_list && hist_list[i]; i++)
        {
            printf("%d %s\n", i + history_base, hist_list[i]->line);
        }
        return true;
    }

    return false;
}


void sh_init(struct shell *sh)
{
    UNUSED(sh);
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_pgid = getpid();
    sh->shell_is_interactive = isatty(sh->shell_terminal);
    tcgetattr(sh->shell_terminal, &sh->shell_tmodes);

    if (sh->shell_is_interactive)
    {
        // Set the shell's process group to its own
        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);
        // Terminate signal handling
        signal(SIGINT, SIG_DFL); 
    }

    // Set default prompt
    sh->prompt = get_prompt("SHELL_PROMPT");

    // Ignores the signals for child processes
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}

void sh_destroy(struct shell *sh)
{
    free(sh->prompt);
    tcsetattr(sh->shell_terminal, TCSANOW, &sh->shell_tmodes);
}

void parse_args(int argc, char **argv)
{
    if (argc > 1)
    {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }
}
