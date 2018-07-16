#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define HISTORY_DEPTH 10
int count = 0;
int count_list[10];
char history[HISTORY_DEPTH][COMMAND_LENGTH];


int command_arr(char *buff){

	int command_pos = count%10;
	for (int i = 0 ; i < COMMAND_LENGTH; i++)
		history[command_pos][i] = 0;
	
	count_list[command_pos] = count+1;
	sprintf(&history[command_pos][0],"%d",count+1);
	history[command_pos][3] = '	';	
	int index = 0;
	int i = 0;
	for(i=4; i<COMMAND_LENGTH && buff[index]!='\0'; i++)
		history[command_pos][i] = buff[index++];		
	
	history[command_pos][i] = '\0';

	return 0;
}

void print_history(){
	int smallest=count_list[0];
	int smallest_index = 0;
	for (int i = 0; i<10; i++){
		if (smallest > count_list[i]){
			smallest = count_list[i];
			smallest_index = i;
		}
	}
	
	
	for(int i= smallest_index;i<HISTORY_DEPTH;i++){
		write(STDOUT_FILENO, &history[i], COMMAND_LENGTH);
		write(STDOUT_FILENO, "\n", strlen("\n"));
	}

	for (int i = 0; i< smallest_index; i++)
	{
		write(STDOUT_FILENO, &history[i], COMMAND_LENGTH);	
		write(STDOUT_FILENO, "\n", strlen("\n"));		
	}
}


int tokenize_command(char *buff, char *tokens[])
{
	char* token;
	int position = 0;
	if (buff[0] == '!'){
		if (buff[1] == '!'){
			int biggest = count_list[0];
			int biggest_index = 0;
			for (int i = 1; i<10; i++){
				if (biggest < count_list[i]){
					biggest = count_list[i];
					biggest_index = i;
				}
			}
			int len = 0;
			for(len = 0; len < COMMAND_LENGTH && history[biggest_index][len+4] != '\0'; len ++){
					buff[len] = history[biggest_index][len+4];
			}
			buff[len] = '\0';	
		}
	

		else{			
			int command_num_in_hisotry = atoi(&buff[1]);			
			bool RESULT = 	false;
			int i = 0;
			for (i = 0; i< 10; i++){
				if (command_num_in_hisotry == count_list[i]){
					RESULT = true;					
					break;
				}									
			}
			if(RESULT == true){
				int len = 0;
				for(len = 0; len < COMMAND_LENGTH && history[i][len+4] != '\0'; len ++)
					buff[len] = history[i][len+4];					
				buff[len] = '\0';
			}
			if(command_num_in_hisotry == 0 || RESULT == false){
				buff[0] = '\0';
				write(STDOUT_FILENO, "Invalid Command Number\n", strlen("Invalid Command Number\n"));
			}
		}
	}

	if(buff[0]!='\0'){
		command_arr(buff);
		count++;
	}

	token = strtok(buff," ");
	if(token == NULL)
		tokens[0] = "\0";
	
	
	while (token != NULL){
		tokens[position] = token;		
		token = strtok(NULL," ");
		position ++;
	}
	
	tokens[position] = '\0';
	return position;
}

void read_command(char *buff, char *tokens[], _Bool *in_background)
{
	*in_background = false;

	// Read input, read make printf not working
	int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);	

	if ( (length < 0)&& (errno !=EINTR))
	{			
		perror("Unable to read command. Terminating.\n");
		exit(-1);
	}

	// Null terminate and strip \n.
	buff[length] = '\0';
	if (buff[strlen(buff) - 1] == '\n') 
	{
		buff[strlen(buff) - 1] = '\0';
	}

	// Tokenize (saving original command string)
	int token_count = tokenize_command(buff, tokens);
	if (token_count == 0) { 		
		return;
	}

	// Extract if running in background:
	if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
		*in_background = true;
		tokens[token_count - 1] = 0;
	}
}


int main(int argc, char* argv[])
{
	

	char input_buffer[COMMAND_LENGTH]; 
	char *tokens[NUM_TOKENS];
	char dir[100];
	char *ptr = dir;
	

	while (true) {
		getcwd(ptr, 100);
		for(int i=0;i<100 && dir[i]!='\0';i++)
			write (STDOUT_FILENO, &dir[i], 1);
	

		write (STDOUT_FILENO, "> ", strlen("> "));

		
		_Bool in_background = false;

		read_command(input_buffer, tokens, &in_background);			

		signal(SIGINT, print_history);

		
		if(tokens[0] == '\0'){			
			continue;
		}	

		if(strcmp(tokens[0], "exit")==0){		
			return 0;
		}		

		if(strcmp(tokens[0], "cd")==0){
			chdir(tokens[1]);
			continue;
		}

		if(strcmp(tokens[0], "pwd")==0){
			getcwd(ptr, 100);
			for(int i=0;i<100 && dir[i]!='\0';i++)
				write (STDOUT_FILENO, &dir[i], 1);
			write (STDOUT_FILENO, "\n", strlen("\n"));
			continue;
		}

		if(strcmp(tokens[0], "history")==0){
			print_history();
			continue;
		}

		pid_t pid = fork();
		if (pid < 0){
			perror("fork failed :( \n");
		}
		else if (pid == 0){					
			if(execvp(tokens[0], tokens)==-1){				
				write (STDOUT_FILENO, "execvp failed: unknown command\n", 
							strlen("execvp failed: unknown command\n"));
				exit(-1);
			}
		}

		int status;
		
		if(in_background == false)
			waitpid(pid, &status, 0);
		
		else
			write (STDOUT_FILENO, "parent's not waiting", strlen("parent's not waiting"));

		while (waitpid(-1, NULL, WNOHANG) > 0) ;
	}
	return 0;
}