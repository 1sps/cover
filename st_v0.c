/*This is the string part. st_v0.c 
 ---------------------------------
 
 It does following:
 
 1. Read a line from keyboard.
 2. Splits the line into subcommands separeted by pipe '|'.
 3. Splits each subcommand into separete args. 

 # It also finds following:
 			   Background operator
			   File to redirect input
			   File to redirect outpout.
 # It does this after finishing step 2 before starting step 3.
 # This step is like step 2.5.
 */
/*Last modified: SPS, June 26, 2015*/
 
/*This program reads a line (Command)
  and splits it into SubCommands (and prints them)*/
/*This program, then also finds out if
  there are any redirection or 
  background operators.*/
/*SPS, June 25, 2015*/

/*Headers*/
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

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
	/*Added in mod4_f1*/
	char *stdin_redirect;
	char *stdout_redirect;
	int background;
	/*---*/
	int num_sub_commands;
}; 
 
/*Function prototype declarations*/
void read_args_test(void);
void read_args(char *, char **, int);
void print_args(char **);

/*mod3_f2.c*/
void shell_tester(void);
void ReadCommand(char *, struct Command *);
void PrintCommand(struct Command *);

/*mod4_f1.c*/
void readRedirectsAndBackground(struct Command *);

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

/*-----v1-----*/
/*EXTRACT*/
/*This function will take a string 
  and extract arguments (Just like how main() 
  does with command line arguments)*/
void read_args(char *in, char **argv, int max_args)
{
	int argc;
	char *token = NULL;
	/*First get rid of the last '\n' that is 
	  present at the end of the string,
	  because of fgets()*/
	//in[strlen(in) - 1] = '\0';

	/*Now use strtok() to extract arguments (tokens)*/
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
/*This function is from mod1_f1.c
  but modified in mod3_f1.c*/
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

/*Shell Tester*/
void shell_tester(void)
{
	char s[200];
	struct Command command;
	for(;;)
	{
		/*Read a string from user*/
		printf("nshell $  ");
		fgets(s, sizeof(s), stdin);
		/*Get rid of the '\n'
		  that is present at
		  the end because of fgets()*/
		s[strlen(s)-1] = '\0';
		/*Note at top*/	
		ReadCommand(s, &command);
		readRedirectsAndBackground(&command);
		PrintCommand(&command);
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
/*This are the steps:
  1. Look at last subcommand for
     background operator and output
     redirector.
  2. Then come to first subcommand to 
     to look for input redirector.*/
/*So there are three cases
  1. If there is 0 command.
     ::return;
  2. If there is one subcommand. 
     Then it is both the last command and first command.
     ::In this case do following:
       ::First find background operator in last token.Modify argv as necessary.
         Then find the 2nd last token of the argv.
	 ::Check if its > or < 
	   ::If found update the stdin_redirect/stdout_redirect. Modify argv as necessary.
	     Then find the 2nd last token of the argv
	     ::Check if it is > or <
	       ::If found update the stdin_redirect/stdout_redirect. Modify argv as necessary.
  3. If there are two or more subcommands.
     ::First in the last subcommand
       check for the background 
       operator and stdout redirector
    ::Then in the first subcommand
      check for the stdin redirector.*/
void readRedirectsAndBackground(struct Command *command)
{
	/*Variables needed in this function*/
	int i;
	/*For each subcommand*/
	struct SubCommand *sub_command_current = NULL;
	/*For each subcommand*/
	char **argv_current = NULL;

	/*Initialize*/
	command->stdin_redirect = NULL;
	command->stdout_redirect = NULL;
	command->background = 0;

	/*Find out how many sub_commands*/
	int how_many_subcommands = command->num_sub_commands;

	/*Three cases*/
	if(how_many_subcommands == 0)
		return;

	else if (how_many_subcommands == 1)
	{
		/*Get the only subcommand*/		
		sub_command_current = &command->sub_commands[how_many_subcommands-1];
		argv_current = sub_command_current->argv;
		/*Find out how many tokens in this sub_command*/
		i = 0;
		while(argv_current[i] != NULL)
			i++;
		/*So there are i tokens*/
		/*Check for the background operator*/
		if(strcmp(argv_current[i-1], "&") == 0)
		{
			command->background = 1;
			/*So by now, we have found if there
 			  is any background operator or not.
			  Next we need to find the redirect operators 
			  and the filenames*/
			/*Background operator no longer needed.
			  (Also it is not part of the subcommand)*/
			argv_current[i-1] = NULL;
		}
		/* Now check for first redirect operator*/
		i = 0;
		while(argv_current[i] != NULL)
			i++;
		if(strcmp(argv_current[i-2], ">") == 0)
		{
			command->stdout_redirect = strdup(argv_current[i-1]);
			argv_current[i-1] = NULL;
			argv_current[i-2] = NULL;
		}
		else if(strcmp(argv_current[i-2], "<") == 0)
		{
			command->stdin_redirect = strdup(argv_current[i-1]);
			argv_current[i-1] = NULL;
			argv_current[i-2] = NULL;
		}
		/* Now check for second redirect operator*/
		i = 0;
		while(argv_current[i] != NULL)
			i++;
		if(strcmp(argv_current[i-2], ">") == 0)
		{
			command->stdout_redirect = strdup(argv_current[i-1]);
			argv_current[i-1] = NULL;
			argv_current[i-2] = NULL;
		}
		else if(strcmp(argv_current[i-2], "<") == 0)
		{
			command->stdin_redirect = strdup(argv_current[i-1]);
			argv_current[i-1] = NULL;
			argv_current[i-2] = NULL;
		}
	}

	else if (how_many_subcommands >= 2)
	{
		/*Find the last subcommand*/	
		sub_command_current = &command->sub_commands[how_many_subcommands-1];
		argv_current = sub_command_current->argv;
		/*Find out how many tokens in this sub_command*/
		i = 0;
		while(argv_current[i] != NULL)
			i++;
		/*So there are i tokens*/
		/*Check for background operator*/
		if(strcmp(argv_current[i-1], "&") == 0)
		{
			command->background = 1;
			/*So by now, we have found if there
 			  is any background operator or not.
			  Next we need to find the redirect operators 
			  and the filenames*/
			/*Background operator no longer needed.
			  (Also it is not part of the subcommand)*/
			argv_current[i-1] = NULL;
		}
		/*Now check for output redirector*/
		i = 0;
		while(argv_current[i] != NULL)
			i++;
		if(strcmp(argv_current[i-2], ">") == 0)
		{
			command->stdout_redirect = strdup(argv_current[i-1]);
			argv_current[i-1] = NULL;
			argv_current[i-2] = NULL;
		}
		/*As we have already checked for
		  background and output redirector
		  in last subcommand, now we can
		  check for the input redirector in
		  first subcommand.*/
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
		if(strcmp(argv_current[i-2], "<") == 0)
		{
			command->stdin_redirect = strdup(argv_current[i-1]);
			argv_current[i-1] = NULL;
			argv_current[i-2] = NULL;
		}
	}

	return;
}
