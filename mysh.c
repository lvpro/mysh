/* mySH Shell Project
(C)Clint Kennedy 

This shell is experimental for learning purposes, so on't expect
too much functionality as of yet. I mostly wrote this to learn piping
on unix systems. Specifically I wrote this under FreeBSD 4.2. 

Current Features:
* Piping of (at max) 10 processes together, each with
a maximum of 10 command line params.
* Command parser is pretty much done, but not all
symbols are implemented yet (output redirection, etc).  
* Debug mode for extending shell commands, check
parsing, etc.
  
Play with it, improve, etc. bla.
/*

/* Include Files */

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>  
#include <sys/wait.h>   
#include <unistd.h>     

/* Definitions */

#define MAXLINE 80
#define STDIN 0
#define STDOUT 1

typedef unsigned char BOOL;
typedef enum { T_WORD, T_EQ, T_AMP, T_BAR, T_SEMI, T_GT, T_GTGT,
               T_LT, T_NL, T_EOS, T_ERROR } token;

struct env_var {
	char *name;
	char *value;
	BOOL exported;
	struct env_var *next;	/* Linked List Of These Structs */
};

/* Prototypes */

void process_command(char *cmd);
token get_token(char *string, char *word);
void create_environment(void);
void add_environment_var(char *word);
void print_shell_commands(void);
void execute_command(char *argv[10][11], int process_count);
void free_memory(char *argv[10][11], int process_count);
void rebuild_environment(void);

/* Globals */

struct env_var *front, *current, *temp_current;
extern char **environ;  /* Environment List */
int get_token_pos;	/* Global Used for get_token()'s positioning */
int debugMode = 0;      /* Debug-Mode flag

/* Main Entry Point */
int main(int argc, char **argv)
{
	char cmd[MAXLINE];

	if (argc != 1)	/* Check Command Line Arguments */
                /* If so, look for debug mode option specified */
		if (strcmp(argv[1], "-d") == 0)
			debugMode = 1;

		create_environment();   /* Build internal environemnt storage */

		printf("\nmysh Tiny Shell For Unix\n");
		printf("(C)2001 Clint Kennedy (DJSlakoR@hotmail.com). All Rights Reserved.\n");

		print_shell_commands();

		for (;;)  /* Internal command "exit" provides an exit point */
		{
			/* First we flush the input buffer in case a user entered
			a command > MAXLINE causing characters to carry over 
			to the next command */
		
			fflush(stdin);	/* remove any left-over input-buffer data */
			printf("mysh ready%% ");	/* shell prompt */
			fgets(cmd, MAXLINE, stdin);	/* read command */
			process_command(cmd);           /* start background job */
		}	
	
	return 0;
}

/* Handles All Command-Line Processing */
void process_command(char *cmd)
{
	token current_token;
	char word[MAXLINE];
	int tokenCount = 0;	
	int i;                  /* Counter Control */

	/* Command Data 
	Allows 10 Processes To Be Piped Together, With
	Each Process Having A Total of 10 Parameters It May Use */ 

	char *argv[10][11];
	int process_count = 0;
	int current_param = 0;

	get_token_pos = 0;	/* Reset Position For New Command */

	current_token = get_token(cmd, word);
	while(current_token != T_EOS)
	{
		switch(current_token) {
		case T_WORD:
			
			tokenCount++;

			/* Process Internal Shell Commands
			Extra commands may easily be added to
			this list at a later time.  This code
			is placed here since the internal command
			must be the first T_WORD, and may need
			to "get" more T_WORDs to execute */
                        
			if (tokenCount == 1)
			{
				if (strcmp(word, "exit") == 0)
					exit(0);
				else if (strcmp(word, "debug") == 0)
				{
					debugMode = (debugMode == 1) ? 0 : 1;
					if (debugMode)
						printf("mysh notification: Debug Mode Turned On!\n");
					else
						printf("mysh notification: Debug Mode Turned Off!\n");
					return;        
				}
				else if (strcmp(word, "env") == 0)
				{
					for (i=0; environ[i] != NULL; i++)
						printf("%s\n", environ[i]);
					return;
				}
				else if (strcmp(word, "env+") == 0)
				{
					temp_current = front;
					while (temp_current != NULL)
					{
						printf("%s=%s , ", temp_current->name, temp_current->value);
						if (temp_current->exported)
							printf("Exported\n");
						else
							printf("Non-Exported\n");
					}
					return;
				}
				else if (strcmp(word, "export") == 0)
				{
					current_token = get_token(cmd, word);
					if (current_token != T_WORD)
					{
						printf("Error: Invalid syntax! Usage: export var-name\n");
						return ;
					}

					temp_current = front;
					while (temp_current != NULL)
					{
						if (strcmp(temp_current->name, word)==0)
						{
							temp_current->exported = 1;
							printf("mysh notification: \"%s\" exported\n", word); 
							rebuild_environment();
							return;
						}
						temp_current=temp_current->next;
					}
					printf("Error: \"%s\" is not a current environment variable!\n", word);
					return;
				}
				else if (strcmp(word, "set") == 0)
				{
					current_token = get_token(cmd, word);
					if (current_token != T_EQ)
					{
						printf("Error: Invalid syntax! Usage: set value=name\n");
						return;
					}
					add_environment_var(word);
					return;
				}
				else if (strcmp(word, "?") == 0)
				{
					print_shell_commands();
					return;
				}
			}

			if (process_count>9)
			{
				printf("Error: Only 10 Processes Pipeable!\n");
				free_memory(argv, process_count);
				return;
			}
			if (current_param>9)
			{
				printf("Error: Parameter Length Exceeded!\n");
				free_memory(argv, process_count);
				return;
			}

			/* Add T_WORD to command data */
			argv[process_count][current_param] = (char *) malloc(strlen(word)+1);
			strcpy(argv[process_count][current_param], word);
			current_param++;

			if (debugMode)
				printf("T_WORD <%s>\n", word);
			break;

		case T_AMP:
			if (debugMode)
				printf("T_AMP\n");
			tokenCount++;
			printf("Error: Unsupported Token Encountered.\n");
			free_memory(argv, process_count);
			break;
		case T_BAR:
			argv[process_count][current_param] = NULL;
			process_count++;
			current_param = 0;
			if (debugMode)
				printf("T_BAR\n");
			tokenCount++;
			break;

		case T_SEMI:
			if (debugMode)
				printf("T_SEMI\n");
			tokenCount++;
			free_memory(argv, process_count);
			break;
		case T_GT:		
			if (debugMode)
				printf("T_GT\n");
			printf("Error: Unsupported token encountered.\n");
			free_memory(argv, process_count);
			break;
		case T_GTGT:
			if (debugMode)
				printf("T_GTGT\n");
			tokenCount++;
			printf("Error: Unsupported token encountered.\n");
			free_memory(argv, process_count);
			break;
		case T_LT:
			if (debugMode)
				printf("T_LT\n");
			tokenCount++;
			printf("Error: Unsupported token encountered.\n");
			free_memory(argv, process_count);
			break;
		case T_NL:
			if (debugMode)
				printf("T_NL\n");
			if (tokenCount != 0)
				process_count++;
			break;
		case T_ERROR:
			return;
		}
		
		current_token = get_token(cmd, word);
	}	

	if (debugMode)
	{
		printf("T_EOS\n");
		printf("Token Count: %d\n", tokenCount);
	}

	argv[process_count-1][current_param] = NULL;

	if(process_count)
		execute_command(argv, process_count);

	free_memory(argv, process_count);
}

token get_token(char *string, char *word)
{
	int word_pos=0;	/* Offset of word[] */
	int assignmentInWord=0; /* T_EQ flag for env table entries */

	while (string[get_token_pos] != '\0')
	{
		switch(string[get_token_pos++])
		{
		case ' ':			/* Skip Space And Tabs */
		case '\t':
			break;
		case '&':
			return T_AMP;
		case '|':
			return T_BAR;
		case ';':
			return T_SEMI;
		case '<':
			return T_LT;
		case '\n':
			return T_NL;
		case '>':
			if (string[get_token_pos] == '>')
			{
				get_token_pos++;
				return T_GTGT;
			}
			else
				return T_GT;
		case '"':
			while (string[get_token_pos] != '"')
			{
				if((string[get_token_pos] == '\n') ||
				   (string[get_token_pos] == '\0'))
				{
					printf("Premature End Of Input: Missing Closing \"\n");
					return T_ERROR;
				}
				word[word_pos++] = string[get_token_pos++];
			}
			get_token_pos++;
			word[word_pos] = '\0';
			return T_WORD;
		default:				
			while ((string[get_token_pos-1] != ' ') &&
				   (string[get_token_pos-1] != '\t') &&
				   (string[get_token_pos-1] != '&') &&
				   (string[get_token_pos-1] != '|') &&
				   (string[get_token_pos-1] != ';') &&
				   (string[get_token_pos-1] != '<') &&
				   (string[get_token_pos-1] != '>') &&
				   (string[get_token_pos-1] != '\n') &&
				   (string[get_token_pos-1] != '\0'))
			{
				word[word_pos++] = string[get_token_pos-1];
				if(string[get_token_pos-1] == '=')
					assignmentInWord=1;	
				get_token_pos++;
			}
			get_token_pos--;
			word[word_pos] = '\0';
			if(assignmentInWord)
				return T_EQ;
			else
				return T_WORD;
		}
	}

	return T_EOS;
}

void create_environment(void)
{
	int i;
	char *eq;	

	/* Check For Empty Environment List */
	if (environ[0] == NULL)
	{
		front = current = NULL;
		return;
	}

	eq = strchr(environ[0], '=');
	
	front = (struct env_var *) malloc(sizeof(struct env_var));
	current = front;

	current->name = (char *) malloc(eq-environ[0]+1);
	strncpy(current->name, environ[0], eq-environ[0]);
	current->name[eq-environ[0]] = '\0';

	current->value = (char *) malloc(strlen(eq));
	strncpy(current->value, eq+1, strlen(eq));

	current->exported = 1;
	current->next = NULL;

	for(i=1; environ[i] != NULL; i++)
	{
		eq = strchr(environ[i], '=');
		current->next = (struct env_var *) malloc(sizeof(struct env_var));
		current = current->next;

		current->name = (char *) malloc(eq-environ[i]+1);
		strncpy(current->name, environ[i], eq-environ[i]);
		current->name[eq-environ[i]] = '\0';

		current->value = (char *) malloc(strlen(eq));
		strncpy(current->value, eq+1, strlen(eq));

		current->exported = 1;
		current->next = NULL;
	}
}

void add_environment_var(char *word)
{
	char *eq;
	struct env_var *previous;

	eq = strchr(word, '=');

	current->next = (struct env_var *) malloc(sizeof(struct env_var));
	current = current->next;
	current->next = NULL;

	current->name = (char *) malloc(eq-word+1);
	strncpy(current->name, word, eq-word);
	current->name[eq-word] = '\0';

	current->value = (char *) malloc(strlen(eq));
	strncpy(current->value, eq+1, strlen(eq));

	current->exported = 0;

	printf("mysh notification: Successfully Set \"%s\" to \"%s\", non-exported.\n", current->name, current->value);
}

void print_shell_commands(void)
{
	printf("\nShell Commands\n--------------\n");
	printf("debug  - Start / Stop Debug Mode\n");
	printf("env    - Display Environment Settings\n");
	printf("env+   - Display Exported AND Non-Exported Environment Settings\n");
	printf("exit   - Exit This Shell\n");
	printf("export - export <name> exports a variable created with the set command\n");
	printf("set    - set <name=value> creates a non-exported variable entry\n");	
	printf(" ?     - This list of shell commands\n");
	printf("\n");
}

void execute_command(char *argv[10][11], int process_count)
{
	int pfd[9][2];  /* We'll need a maximum of 9 pipes */
	int i, j;

	for(i=0; i<(process_count-1); i++)
		pipe(pfd[i]);
        
	/* Special Case, Left Most Process */

	switch(fork())
	{
	case 0:
		if(process_count == 1)
		{
			 /* Close All The Pipes We Don't Have Power Over */
			for(i=0; i<(process_count-1); i++)
			{
				close(pfd[i][0]);		
				close(pfd[i][1]);
			}

			execvp(argv[0][0], argv[0]);
			printf("Error: Command not found.\n");
			exit(0);
		}
		/* Otherwise we need to redirect stdout to next process */
		else
		{
			close(STDOUT);
			dup2(pfd[0][1], STDOUT);

			/* Close All The Pipes We Don't Have Power Over */
			for(i=0; i<(process_count-1); i++)
			{
				close(pfd[i][0]);
				close(pfd[i][1]);
			}

			execvp(argv[0][0], argv[0]);
			printf("Error: Command not found.\n");
			exit(0);
		}
		default:
			if(process_count == 1)
			{
				/* Close All The Pipes We Don't Have Power Over */
				for(i=0; i<(process_count-1); i++)
				{
					close(pfd[i][0]);
					close(pfd[i][1]);
				}

				while(wait(NULL) != -1);
				return ;
			}
        }

        /* All "middle" cases */

		for(i=1; i<(process_count-1); i++)
		{
			switch(fork())
			{
			case 0:
				close(STDIN);
				dup2(pfd[i-1][0], STDIN);
				close(pfd[i-1][0]);
				close(pfd[i-1][1]);
				close(STDOUT);
				dup2(pfd[i][1], STDOUT);
				close(pfd[i][0]);
				close(pfd[i][1]);

				for(j=0; j<(process_count-1); j++)
				{
					close(pfd[j][0]);
					close(pfd[j][1]);
				}

				execvp(argv[i][0], argv[i]);
				printf("Error: Command not found.\n");
				exit(0);
			}
        }

		/* Special Case, Right Most Process */
		switch(fork())
		{
		case 0:
			close(STDIN);
			dup2(pfd[process_count-2][0], STDIN);

			/* Close All The Pipes We Don't Have Power Over */
			for(i=0; i<(process_count-1); i++)
			{
				close(pfd[i][0]);
				close(pfd[i][1]);
			}

			execvp(argv[process_count-1][0], argv[process_count-1]);
			printf("Error: Command not found.\n");
			exit(0);
        }

        /* Close All The Pipes We Don't Have Power Over */
        for(i=0; i<(process_count-1); i++)
        {
			close(pfd[i][0]);
			close(pfd[i][1]);
        }

		while(wait(NULL) != -1) ;   /* Wait For All Children To Finish */
}

void free_memory(char *argv[10][11], int process_count)
{
	int i, j;

	for(i=0; i<process_count; i++)
	{
		for(j=0; argv[i][j] != NULL; j++)
			free(argv[i][j]);
	}
}

void rebuild_environment(void)
{
	static int first_build = 1;
	int i;

	char *new_environment[1000];   /* 1000 Environment Vars Aughta Do It */

	/* Free Up The Previously Allocated Memory */
	if (!first_build)
	{
		for(i=0; environ[i] != NULL; i++)
			free(environ[i]);
	}

	if (first_build) first_build = 0;

	i=0;
	temp_current = front;
	while (temp_current != NULL)
	{
		if (temp_current->exported)
		{
			new_environment[i] = (char *) malloc(strlen(temp_current->name)+
									strlen(temp_current->value)+
                                                             2); /* = and null */
			strcpy(new_environment[i], temp_current->name);
			strcat(new_environment[i], "=");
			strcat(new_environment[i], temp_current->value);
			i++;
		}
		
		temp_current = temp_current->next;
	}

	new_environment[i] = NULL;

	environ = new_environment;
}
