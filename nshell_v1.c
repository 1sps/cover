/*
  nshell_v1.c
  Last modified: sps, 6-29-2015
*/
/*
  Code of a command line interpreter.
  ----
*/
/*Headers*/
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/wait.h>

/*Constants*/
#define MAX_ARGS 10
#define MAX_SUB_COMMANDS 5

/*Structures*/
struct SubCommand
{
	char *line;
	char *argv[MAX_ARGS];
};
struct Command
{
	struct SubCommand sub_commands[MAX_SUB_COMMANDS];
	char *stdin_redirect;
	char *stdout_redirect;
	int background;
	int num_sub_commands;
}; 
 
/*Function prototype declarations*/
void read_args_test(void);
void read_args(char *, char **, int);
void print_args(char **);
void ReadCommand(char *, struct Command *);
void PrintCommand(struct Command *);
void read_rd_bg(struct Command *);
void shell_tester(void);
void shell(void);
void child_do_work(int, struct Command *, int *);

/*Program starts*/
int main(int argc, char **argv)
{
	if(argc != 1)
	{
		printf("No command line arguments.\n");
		printf("Try again ...\n");
		exit(1);
	}
	shell_tester();
	return 0;
}

/*Shell tester*/
void shell_tester(void)
{
	shell();
	return;
}
/*Shell*/
void shell(void)
{
	int i;
	char s[200];
	int previous_command_background = 0;
	int previous_command_subcommands = 0;
	
	struct Command command;
	for(;;)
	{
		/*Read a string from user*/
		printf("nshell $  ");
		fgets(s, sizeof(s), stdin);
		s[strlen(s)-1] = '\0';
		/*Read the command*/
		ReadCommand(s, &command);
		read_rd_bg(&command);
		//PrintCommand(&command);
		/*For more than one subcommand*/
		if(command.num_sub_commands > 1)
		{
			int *pipe_arr = malloc((command.num_sub_commands-1) * 2 * sizeof(int));
			int *pipe_arr_current = pipe_arr;
			for(i=0; i<command.num_sub_commands-1; i++)	
			{
				pipe(pipe_arr_current);
				pipe_arr_current += 2;
			}
			pipe_arr_current = pipe_arr;
			/*Now that we have all the pipes*/
			int *child_pid_arr = malloc(command.num_sub_commands * sizeof(int));
			int *child_pid_current = child_pid_arr;
			for(i=0; i<command.num_sub_commands; i++)
			{
				child_pid_current[i] = fork();
				if(child_pid_current[i] == -1)
					exit(1);
				else if(child_pid_current[i] == 0)
				{
					child_do_work(i, &command, pipe_arr);
					i = command.num_sub_commands;	
					exit(1);
				}
				else if(child_pid_current[i] > 0)
				{
					if(i > 0)
					{
						close(pipe_arr_current[0]);
						close(pipe_arr_current[1]);
						pipe_arr_current += 2;
					}
				}
			}
			if(command.background ==1)
			{
				if(previous_command_background == 1)		
				{
					for(i=0; i<previous_command_subcommands; i++)	
						wait(NULL);
				}
				printf("[%d]\n", child_pid_arr[command.num_sub_commands-1]);
				previous_command_background = 1;
				previous_command_subcommands = command.num_sub_commands;
			}
			else if(command.background == 0)
			{
				if(previous_command_background == 1)
				{
					for(i=0; i<previous_command_subcommands; i++)	
						wait(NULL);
				}
				previous_command_background = 0;
				previous_command_subcommands = 0;
				for(i=0; i<command.num_sub_commands; i++)
				{
					wait(NULL);
				}
			}
		}
		/*If only one subcommand*/
		else if(command.num_sub_commands == 1)
		{
			int dummy = 1;
			int child_pid = fork();
			if(child_pid == -1)
				exit(1);
			else if(child_pid == 0)
				child_do_work(-1, &command, &dummy);
			else if(child_pid > 0)
			{
				if(command.background == 1)
				{
					if(previous_command_background == 1)		
					{
						for(i=0; i<previous_command_subcommands; i++)	
							wait(NULL);
					}
					printf("[%d]\n", child_pid);
					previous_command_background = 1;
					previous_command_subcommands = command.num_sub_commands;
				}
				else if(command.background == 0)
				{
					if(previous_command_background == 1)		
					{
						for(i=0; i<previous_command_subcommands; i++)	
							wait(NULL);
					}
					previous_command_background = 0;
					previous_command_subcommands = 0;
					wait(NULL);
				}
			}
		}
		/*If 0 subcommand, i.e. user just pressed 'Enter'*/
		else if(command.num_sub_commands == 0)
		{
			continue;	
		}
	}
	/*Shell complete*/
	return;
}

/*This function will take a string 
  and extract arguments (Just like how main() 
  does with command line arguments)*/
void read_args(char *in, char **argv, int max_args)
{
	int argc;
	char *token = NULL;

	/*Use strtok() to extract arguments (tokens)*/
	for(argc=0; argc<max_args; argc++)
	{
		if (argc==0)
			token = strtok(in, " ");
		else
			token = strtok(NULL, " ");
		if(token != NULL)
			/*Duplicate the token*/
			argv[argc] = strdup(token);
		else
		{
			argv[argc] = NULL;
			break;
		}
	}
	if(argc == max_args)
	{
		free(argv[argc-1]);
		argv[argc-1] = NULL;
		
	}
	return;
}
/*PRINT*/
/*This function prints the arguments*/
void print_args(char **argv)
{
	int i = 0;
	while(argv[i] != NULL)
	{
		printf("argv[%d] = '%s'\n", i, argv[i]);
		i++;
	}
	return;
}


/*Read the Command from the line (get/extract all the SubCommands)*/
void ReadCommand(char *s, struct Command *command)
{
	char *sub_command_current = NULL;
	int i;
	for(i=0; i<MAX_SUB_COMMANDS; i++)
	{
		if(i==0)	
			sub_command_current = strtok(s, "|");
		else
			sub_command_current = strtok(NULL, "|");
		if(sub_command_current != NULL)	
		{
			command->sub_commands[i].line = strdup(sub_command_current);
		}
		else
			break;
	}
	command->num_sub_commands = i;
	
	/*Call read_args() to get the tokens for each sub-command*/
	for(i=0; i<command->num_sub_commands; i++)
	{
		read_args(command->sub_commands[i].line, command->sub_commands[i].argv, MAX_ARGS);
	}
	return;
}
/*Print the command (all subcommands)*/
/*For debugging*/
void PrintCommand(struct Command *command)
{
	if(command->num_sub_commands == 0)
		return;
	int i;
	for(i=0; i<command->num_sub_commands; i++)
	{
		printf("\nCommand %d:\n", i)	;
		print_args(command->sub_commands[i].argv);
	}
	printf("\n");
	if(command->stdin_redirect != NULL)
		printf("Redirect stdin: %s\n", command->stdin_redirect);
	if(command->stdout_redirect != NULL)
		printf("Redirect stdout: %s\n", command->stdout_redirect);
	if(command->background == 1)
		printf("Background: Yes\n");
	else
		printf("Background: No\n");
	printf("\n");
	return;
}

/*Reads redirections and background operator*/
void read_rd_bg(struct Command *command)
{
	/*Variables needed in this function*/
	int i;
	struct SubCommand *sub_command_current = NULL;
	char **argv_current = NULL;

	/*Initialize*/
	command->stdin_redirect = NULL;
	command->stdout_redirect = NULL;
	command->background = 0;

	int how_many_subcommands = command->num_sub_commands;

	/*Three cases*/
	/*No subcommand*/
	if(how_many_subcommands == 0)
		return;

	/*Only one subcommand*/
	else if (how_many_subcommands == 1)
	{
		/*Get the only subcommand*/		
		sub_command_current = &command->sub_commands[how_many_subcommands-1];
		argv_current = sub_command_current->argv;
		/*Find out how many tokens in this sub_command*/
		i = 0;
		while(argv_current[i] != NULL)
			i++;
		/*Check for the background operator*/
		if(strcmp(argv_current[i-1], "&") == 0)
		{
			if(i != 1)
			{
				command->background = 1;
				argv_current[i-1] = NULL;
			}
		}
		/* Now check for first redirect operator*/
		i = 0;
		while(argv_current[i] != NULL)
			i++;
		if(i > 1 && strcmp(argv_current[i-2], ">") == 0)
		{
			command->stdout_redirect = strdup(argv_current[i-1]);
			argv_current[i-1] = NULL;
			argv_current[i-2] = NULL;
		}
		else if(i > 1 && strcmp(argv_current[i-2], "<") == 0)
		{
			command->stdin_redirect = strdup(argv_current[i-1]);
			argv_current[i-1] = NULL;
			argv_current[i-2] = NULL;
		}
		/* Now check for second redirect operator*/
		i = 0;
		while(argv_current[i] != NULL)
			i++;
		if(i > 1 && strcmp(argv_current[i-2], ">") == 0)
		{
			command->stdout_redirect = strdup(argv_current[i-1]);
			argv_current[i-1] = NULL;
			argv_current[i-2] = NULL;
		}
		else if(i > 1 && strcmp(argv_current[i-2], "<") == 0)
		{
			command->stdin_redirect = strdup(argv_current[i-1]);
			argv_current[i-1] = NULL;
			argv_current[i-2] = NULL;
		}
	}

	/*2 or more subcommands*/
	else if (how_many_subcommands >= 2)
	{
		/*Find the last subcommand*/	
		sub_command_current = &command->sub_commands[how_many_subcommands-1];
		argv_current = sub_command_current->argv;
		/*Find out how many tokens in this sub_command*/
		i = 0;
		while(argv_current[i] != NULL)
			i++;
		/*Check for background operator*/
		if(strcmp(argv_current[i-1], "&") == 0)
		{
			if(i != 1)
			{
				command->background = 1;
				argv_current[i-1] = NULL;
			}
		}
		/*Now check for output redirector*/
		i = 0;
		while(argv_current[i] != NULL)
			i++;
		if(i > 1 && strcmp(argv_current[i-2], ">") == 0)
		{
			command->stdout_redirect = strdup(argv_current[i-1]);
			argv_current[i-1] = NULL;
			argv_current[i-2] = NULL;
		}
		/*Find the first subcommand*/
		sub_command_current = &command->sub_commands[0];
		argv_current = sub_command_current->argv;
		/*Find out how many tokens in this sub_command*/
		i = 0;
		while(argv_current[i] != NULL)
			i++;
		/*Now check for input redirector*/
		i = 0;
		while(argv_current[i] != NULL)
			i++;
		if(i > 1 && strcmp(argv_current[i-2], "<") == 0)
		{
			command->stdin_redirect = strdup(argv_current[i-1]);
			argv_current[i-1] = NULL;
			argv_current[i-2] = NULL;
		}
	}

	return;
}

/*This function is executed by child processes*/
void child_do_work(int i, struct Command *command, int *pipe_fds)
{
	/*If the only subcommand*/
	if(i == -1)
	{
	 	if(command->num_sub_commands > 0)
		{
			/*Do redirection if needed*/
			/*input*/
			if(command->stdin_redirect != NULL)
			{
				close(0);
				int fd = open(command->stdin_redirect, O_RDONLY);
				if(fd == -1)
				{
					fprintf(stderr, "Error: %s : Could not open file\n", command->stdin_redirect);
					exit(1);
				}
			}
			/*Output*/
			if(command->stdout_redirect != NULL)
			{
				close(1);
				int fd = open(command->stdout_redirect, O_WRONLY | O_CREAT | O_TRUNC, 0660);
				if(fd == -1)
				{
					fprintf(stderr, "Error: %s : Could not open file\n", command->stdout_redirect);
					exit(1);
				}

			}

			/*Execute the subcommand*/
			int check_exec = execvp(command->sub_commands[0].argv[0], command->sub_commands[0].argv);
			if (check_exec == -1)
			{
				fprintf(stderr, "%s: Command not found\n", command->sub_commands[0].argv[0]);
				exit(2);
			}
		}
		else
			exit(0);
	}
	/*Not the only subcommand*/
	else
	{
		/*Write to the pipe*/
		if(i < command->num_sub_commands-1)
		{
			/*Close read end*/				
			close(pipe_fds[2*i]);
			/*Close stdout*/
			close(1);
			dup(pipe_fds[(2*i)+1]);
		}
		/*Read from pipe*/
		if(i > 0)
		{
			close(pipe_fds[(i-1)*2 + 1]);
			close(0);
			dup(pipe_fds[(i-1)*2]);

		}
		/*Do redirection if needed*/
		if(i==0 && command->stdin_redirect != NULL)
		/*input*/
		{
			close(0);
			int fd = open(command->stdin_redirect, O_RDONLY);
			if(fd == -1)
			{
				fprintf(stderr, "Error: %s : Could not open file\n", command->stdin_redirect);
				exit(1);
			}
		}
		/*Output*/
		if(i==command->num_sub_commands-1 && command->stdout_redirect != NULL)
		{
			close(1);
			int fd = open(command->stdout_redirect, O_WRONLY | O_CREAT | O_TRUNC, 0660);
			if(fd == -1)
			{
				fprintf(stderr, "Error: %s : Could not open file\n", command->stdout_redirect);
				exit(1);
			}

		}
		/*Execute*/
		int check_exec = execvp(command->sub_commands[i].argv[0], command->sub_commands[i].argv);
		if (check_exec == -1)
		{
			fprintf(stderr, "%s: Command not found\n", command->sub_commands[i].argv[0]);
			exit(2);
		}
	}
	return;
}
