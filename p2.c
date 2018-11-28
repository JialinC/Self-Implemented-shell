/*Jialin Cui CS570 Carroll 02/28/2018
p2.c contain a main function,a parse function, and a signal handler function.

parse() parse the command line argument store every word in a storage array and use a array of char pointers to 
point to every command and argument that going to run in main, parse set global flags to indicate all kinds of 
metacharacters, parse also return a word conunts of the input sentence as its return value.

main() function fork child(one or more in p2) to execute different command with its arguments.p2 can also take command line argument as file name to run. main() also have specified
behavior to different metacharacters. main() exit on receiving EOF,otherwise run a infinite loop to get command and execute
them.

myhandler() function catch the SIGTERM for main, protect it from kill by the signal.
*/


#include "p2.h"
int global_flag_out; /*global flag for >*/
int global_flag_in; /*global flag for <*/
int global_flag_back; /*global flag for &*/
int global_flag_pound; /*global flag for #*/
int command_flag; /*global command line argument flag*/
int pipe_number; /*global tracking of number of pipes*/
int pipeposition[10]; /*index of pipeline in argument pointer array*/
static int pipe_type[10]; /*initialize all to 0, which means they are all just normal pipe instead of |&*/

char *input_file; /*redirection input file name*/
char *output_file; /*redirection output file name*/
char s[STORAGE*MAXITEM]; /*storage string*/
char *newargv[MAXITEM]; /*argument pointer array*/

int parse();
void myhandler(int signum);

main(int argc, char *argv[]){
	
	int command_fd;
	int num_arg; /*number of words returned by parse*/
	int cd_correct; /*indicator of cd correctlly*/
	int input_fd; /*input file descriptor*/
	int output_fd; /*output file descriptor*/
	int saved_stderr;
	pid_t childpid; /*pid for single child case without pipe*/
	(void) signal(SIGTERM,myhandler);
	setpgid(0,0); /*set grounp id to separate p2 from running shell*/
	command_flag = 0;
	saved_stderr = dup(2);
	
	if(argc > 1){
	
		if(argv[2] != NULL){/*check argv[2], must be empty*/
		
			fprintf(stderr,"Invalid number od command line arguments.\n");
			fflush(stderr);
			exit(10);
		
		}
	
		if(argv[1] != NULL){
		
			command_flag = 1; /*set the command line argument flag to 1*/
			if((command_fd = open(argv[1], O_RDONLY)) < 0){ /*open the input file*/
				
				char str[255];
				strcpy(str,"Open failed! Command line argument file invalid.");
				strcat(str,argv[1]);
				perror(str);
				exit(11);
					
			}
			dup2(command_fd, STDIN_FILENO); /*redirect the stdin to the file*/
			close(command_fd); /*close the input_fd file descriptor*/	
		}
	}

	for(;;){
		pipe_number = 0; /*initialize pipeline number*/
		global_flag_out = 0; /*reset output file flag as needed*/
		global_flag_in = 0; /*reset input file flag as needed*/
		global_flag_back = 0; /*reset background flag as needed*/
		
		if(command_flag != 1){
			printf("p2: "); /*issue prompt*/
		}
		num_arg = parse(); /*parse the command line and return the words number*/
		
		if(global_flag_out > 1 || global_flag_in > 1){ /*syntax error multiple redirect not allowed*/
			fprintf(stderr,"Ambiguous redirect.\n");
			fflush(stderr);
			continue;
		}
		
		if(num_arg == -2){ /*syntax error handle redirect without file name*/
			fprintf(stderr,"Redirection error.\n");
			fflush(stderr);
			continue;
		}
		
		
		if(num_arg == -1){ /*first word is EOF*/
			break;
		}
		
		if(num_arg == 0){ /*no execuable command the line is empty*/
			continue;
		}
		
		if((num_arg - 2*global_flag_in - 2*global_flag_out - pipe_number) == 0){ /*this line calculate the actual arguments number of input*/
			fprintf(stderr,"Invalide command.\n");					 /*print error if the actual argument number is 0*/   
			fflush(stderr);
			continue;
		}
		
		if(num_arg == -3){
			fprintf(stderr,"Invalide command.\n");
			fflush(stderr);
			continue;
		}
		
		if(strcmp(newargv[0],"cd") == 0){ /*handle buildin command cd*/
			
			if(num_arg > 2){ /*handle cd argument more than 1 error*/
				fprintf(stderr,"chdir: Too many arguments.\n");
				fflush(stderr);
				continue;
			}
			
			if(newargv[1] == NULL){ /*with no parameters cd $HOME*/
				cd_correct = chdir(getenv("HOME"));
				
				if(cd_correct != 0){ /*fail to go home chdir failed*/
					perror("chdir: cannot go back to HOME ditrctory.");
					continue;
				}
				
				continue;
			}
			
			else{ /*cd to given path*/
				cd_correct = chdir(newargv[1]);
				
				if(cd_correct != 0){ /*fail to go to given path*/
					perror("path name error.");
					continue;
				}
				continue;
			}
			
		}
		
		if(strcmp(newargv[0],"MV") == 0){ /*handle buildin command MV*/
			int num_argv_err = 0;
			char path_name[STORAGE]; /*directory path name*/
			int target_file_index = -1; /*index of target file name to be moved in the argurment array*/ 
			int destination_file_index = -1; /*index of destination file name or directory in the argument array*/
			int flag_index = 0; /*set up the final flag index*/
			int i; /*for loop to run through argument array index*/
			char *file_name; /*char pointer to the name of the file we want to move*/
			struct stat buffer; /*structue to store the file information*/
			int status; /*store stat() return value*/
			
			
			for(i = 1; i < num_arg; i++){ /*this loop get the arguments and the flag*/
				
				if(newargv[i][0] != '-'){/* not a flag then must be a file name or path*/
					
					if(target_file_index!=-1&&destination_file_index!=-1){ /* there are more than 2 file or path name, error*/
						num_argv_err = 1;
					}
					
					if(target_file_index == -1){ /*first file name has not been set, then set it*/
						target_file_index = i;
					}
					
					else{ /*second file name*/
						destination_file_index = i;
					}
				}
				
				if(newargv[i][0] == '-'){ /*when the argument is a flag, then mark the position in the argument array*/
					flag_index = i;
				}
			} /*after this loop, we should get the index of target file, desination file, and the last flag, if no flag, flag index is 0*/
			if(num_argv_err){
				fprintf(stderr,"MV failed! Too many MV arguments.\n");
				fflush(stderr);
				continue;
			}
			
			if(target_file_index == -1||destination_file_index == -1){
				fprintf(stderr,"MV failed! Not enough number of MV arguments.\n");
				fflush(stderr);
				continue;
			}
			
			status = stat(newargv[destination_file_index], &buffer); /*stroe the second argument file information in buffer*/
			
			if(S_ISDIR(buffer.st_mode)&&status==0){ /*test if the second argument is a path name, then assemble the final path name*/
				
				i = 0;
				file_name = newargv[target_file_index]; /*set it point to the start point first*/
				
				while(newargv[target_file_index][i] != '\0'){ /*this loop gets the name of the file that we want to move*/
			
					if(newargv[target_file_index][i] == '/'){
						file_name = newargv[target_file_index]+i+1;
					}
					i = i + 1;
				}
				
				strcpy(path_name, newargv[destination_file_index]); /*create the final path name use for link*/
				strcat(path_name, "/");
				strcat(path_name, file_name);
				strcat(path_name, "\0");
			}
			/*printf(" the status: %d \n",status);
			printf(" the destinaton: %s \n",newargv[destination_file_index]);
			printf(" the target: %s \n",newargv[target_file_index]);
			printf(" the flag: %s \n",newargv[flag_index]);
			printf(" the final path name: %s \n",path_name);
			printf(" the file name: %s \n",file_name);*/
			
			if(strcmp(newargv[flag_index],"-f") == 0){/*forcibly does the move*/
				/*printf("forced move \n");*/
				if(access(newargv[target_file_index],F_OK) != 0){ /*should check if old file exists before unlink new file*/
					
					char str[255];
					strcpy(str,"MV failed!File does not exist!");
					strcat(str,newargv[target_file_index]);
					perror(str);
					continue;
					
				}
				
				unlink(newargv[destination_file_index]); /*unlink new file name*/
				/*unlink(path_name);*/
				
				if(S_ISDIR(buffer.st_mode)&&status==0){ /*name of the file MV is not given*/
				
					if(link(newargv[target_file_index],path_name) != 0){ /*link old name to new name*/
					
					char str[255];
					strcpy(str,"MV failed! Invalid path name: ");
					strcat(str,path_name);
					perror(str);
					continue;
					
					}
					
				}
				
				else{ /*name of the file after MV already given*/
					if(link(newargv[target_file_index],newargv[destination_file_index]) != 0){ /*link old name to new name*/
						
						char str[255];
						strcpy(str,"MV failed! Invalid path name: ");
						strcat(str,newargv[destination_file_index]);
						perror(str);
						continue;
					}
				}
				
				unlink(newargv[target_file_index]); /*unlink old file name*/
				continue;
			}
					
			else{/*normal move*/
				/*printf("normal move \n");*/
				
				if(access(newargv[target_file_index],F_OK) != 0){ /*should check if old file exists before unlink new file*/
					
					char str[255];
					strcpy(str,"MV failed!File does not exist!");
					strcat(str,newargv[target_file_index]);
					perror(str);
					continue;
					
				}
				
				if(S_ISDIR(buffer.st_mode)&&status==0){/*the second is a path*/
					/*printf("move to a directory \n");*/
					
					if(link(newargv[target_file_index],path_name) != 0){ /*link old name to new name*/
						char str[255];
						strcpy(str,"MV failed! Invalid path name: ");
						strcat(str,path_name);
						perror(str);
						continue;
					}
				}
				
				else{ /*normal file not directory*/
					
					if(link(newargv[target_file_index],newargv[destination_file_index]) != 0){
					char str[255];
					strcpy(str,"MV failed! Invalid path name: ");
					strcat(str,newargv[destination_file_index]);
					perror(str);
					continue;
					}
					
				}
				
				unlink(newargv[target_file_index]); /*unlink old file name*/
				continue;
			}
		}
		
				
		if(pipe_number != 0){ /*check for pipe line*/
			
			pid_t right; /*the pid of the rightmost child, the one do the last command, p2 only know this one*/
			fflush(stdout);
			fflush(stderr); /*flush stderr and stdout before fork first child*/
			right = fork(); /*parent p2 get the rightmost child pid so that can do a wait*/
				
				if(right < 0){ /*check if fork successful*/
					perror("fork failed.");
					continue;
				}
			
				if(0 == right){ /*the first(the rightmost) forked child*/
					
					int i;
					pid_t middle_child; /*middle child pid*/
					int fildes[pipe_number*2]; /*file desk int array for the pipe(), must have pipe_number*2 slots*/
					pipe(fildes); /*rightmost child creates the last pipe*/
					middle_child = fork(); /*fork first middle child to handle multiple pipe*/
					
					if(middle_child != 0){ /*rightmost child run the below code*/
					
						dup2(fildes[0],STDIN_FILENO); /*change the standard input of the rightmost child*/
						close(fildes[0]); /*close file descriptor*/
						close(fildes[1]); /*close file descriptor*/
					
						if(global_flag_out == 1){ /*handle output redirection*/
							mode_t mode = S_IRWXU | S_IRWXG | S_IROTH;
					
							output_fd = open(output_file, O_WRONLY | O_CREAT | O_EXCL, mode); 
							/*open the output file descriptor, if*/ 
							/*the file name already exist then open*/
							/*failed with EEXIST, otherwise create*/ 
							/*the output file with given mode*/
					
							dup2(output_fd, STDOUT_FILENO); /*redirect the stdout to the file*/
							close(output_fd);  /*close the output_fd file descriptor*/
						}
					
						if(execvp(newargv[pipeposition[pipe_number - 1] + 1],
						   newargv + pipeposition[pipe_number - 1] + 1) < 0){
							char str[80];
							strcpy(str,"Execute failed!: ");
							strcat(str,newargv[pipeposition[pipe_number - 1] + 1]);
							perror(str); 
							/*if execute failed print error and exit*/
							_exit(13);
						}
						/*rightmost child run the last command*/
						/*so that the parent can wait for all*/
						/*children*/
					} /*rightmost child code end here*/
					
					
					
					for(i = 1;i < pipe_number; i++){ /*this loop create the number of middle child*/
					
						pipe(fildes + 2*i); /*open file desk array for input and pass to next middle child*/
						
						dup2(fildes[2*i - 1],STDOUT_FILENO); /*redirect stdout to the pipe inherent from parent process*/
				        
						
						if(pipe_type[pipe_number - i] == 1){
							dup2(fildes[2*i - 1],STDERR_FILENO); /*redirect the stderr to the pipe*/
						}
						
						if(pipe_type[pipe_number - i] == 0){
							dup2(saved_stderr,STDERR_FILENO); /*redirect the stderr to the saved stderr*/
						}
					
						close(fildes[2*i - 2]); /*close the pipe inherent from parent process before further fork*/
						close(fildes[2*i - 1]); /*so that child will not see that pipe, avoid to many close*/ 
						middle_child = fork(); /*fork next middle child*/
					
						if(middle_child != 0){ /*the one who create middle child*/
						
							dup2(fildes[2*i],STDIN_FILENO); /*redirect its stdin to the pipe it creates*/
							close(fildes[2*i]); /*close the pipe it creates*/
							close(fildes[2*i + 1]);
							
							if(execvp(newargv[pipeposition[pipe_number - i - 1] + 1],
						   	   newargv + pipeposition[pipe_number - i - 1] + 1) < 0){
						   		
								char str[80];
						   		strcpy(str,"Execute failed!: ");
						   		strcat(str,newargv[pipeposition[pipe_number - i - 1] + 1]);
						   		perror(str);
						   		/*if execute failed print error and exit*/
						   		_exit(14);
						   	}
						
						}
					} /*middle children code end here, below is the leftmost child code*/
					
					if(global_flag_back == 1){ /*this is a background job should redirect stdin to dev/null*/
						int dev_null = open("/dev/null",O_WRONLY); /*open dev/null file descriptor*/
						dup2(dev_null, STDIN_FILENO);
					}
		
					if(global_flag_in == 1){ /*handle input redirection*/
						input_fd = open(input_file, O_RDONLY); /*open the input file*/
						dup2(input_fd, STDIN_FILENO); /*redirect the stdin to the file*/
						close(input_fd); /*close the input_fd file descriptor*/
					}
					
					
					
					dup2(fildes[pipe_number*2 - 1],STDOUT_FILENO); /*redirect its stdout to pipe it inherent from parent*/
					
					if(pipe_type[0] == 0){
						dup2(saved_stderr,STDERR_FILENO); /*redirect the stderr to the saved stderr*/ 
					}
					
					if(pipe_type[0] == 1){
						dup2(fildes[pipe_number*2 - 1],STDERR_FILENO); 
					}
					
					
					close(fildes[pipe_number*2 - 1]); /*close file descriptor it inherents from parent*/
					close(fildes[pipe_number*2 - 2]); /*close file descriptor*/
					
					if(execvp(newargv[0],newargv) < 0){
						char str[80];
						strcpy(str,"Execute failed!: ");
						strcat(str,newargv[0]);
						perror(str);  /*if execute failed print error and exit*/
						_exit(15);
					} 
					/*leftmost child always run the first command*/
					/*last child run the first command so that parent can wait*/ 
					/*for the first child, since the parent does not know about */
					/*the middle and last children*/
				} /*lestmost child code block end*/
				
			if(global_flag_back == 1){ 
				/*if the child's job is a back ground job, then parent does not wait for it and reissue prompt again*/
				printf("%s [%d]",newargv[pipeposition[pipe_number - 1] + 1],right); /*print the command name and the child's pid*/
				continue;
			}
			
			for(;;){ /*parent wait for the first child finish its job*/
				pid_t pid;
				pid  = wait(NULL);
				
				if(pid == right){
				break;
				}
			}
			continue;
		} /*handle the multiple pipe line ends here*/
		
		/*below is the situation without pipeline*/
		
		fflush(stdout);
		fflush(stderr); /*always flush before fork*/
		childpid = fork();
		
		if(childpid < 0){ /*fork failed*/
			fprintf(stderr,"fork failed.\n");
			fflush(stderr);
			continue;
		}
		
		if(childpid == 0){ /*fork off a child to do jobs*/
		
			if(global_flag_back == 1){ /*this is a background job should redirect stdin to dev/null*/
				int dev_null = open("/dev/null",O_WRONLY); /*open dev/null file descriptor*/
				dup2(dev_null, STDIN_FILENO);
			}
		
			if(global_flag_in == 1){ /*handle input redirection*/
				
				if((input_fd = open(input_file, O_RDONLY)) < 0){ /*open the input file*/
					perror("Open failed!");
					_exit(16);	
				}
				
				if(dup2(input_fd, STDIN_FILENO) < 0){ /*redirect the stdin to the file*/
					perror("Redirection failed!");
					_exit(17);
				}
				
				if(close(input_fd) < 0){ /*close the input_fd file descriptor*/
					perror("Close failed!");
					_exit(18);
				}
			}
				
			if(global_flag_out == 1){ /*handle output redirection*/
				mode_t mode = S_IRWXU | S_IRWXG | S_IROTH;
				
				if((output_fd = open(output_file, O_WRONLY | O_CREAT | O_EXCL, mode)) < 0){ /*open the output file descriptor, if*/ 
					perror("Open failed!");						   /*the file name already exist then open*/
					_exit(19);							   /*failed with EEXIST, otherwise create*/ 
				}									   /*the output file with given mode*/
					
				if(dup2(output_fd, STDOUT_FILENO) < 0){ /*redirect the stdout to the file*/
					perror("Redirection failed!");
					_exit(20);
				}
				
				if(close(output_fd) < 0){ /*close the output_fd file descriptor*/
					perror("Close failed!");
					_exit(21);
				}
			}
			
			if(execvp(newargv[0],newargv) < 0){ /*child execute the command*/
				perror("Execute failed!");  /*if execute failed print error and exit*/
				_exit(22);
			}
			
			_exit(0); /*exit normally*/	
				
		} /*child code block ends here*/
		
		if(global_flag_back == 1){ /*if the child's job is a back ground job, then parent does not wait for it and reissue prompt again*/
			printf("%s [%d]\n",newargv[0],childpid); /*print the command name and the child's pid*/
			continue;
		}
		
		for(;;){ /*parent wait for child finish its job*/
			pid_t pid;
			pid  = wait(NULL);
				
			if(pid == childpid){ /*wait till the child exit*/
				break;
			}
		}
		
	} /*deteced the EOF symbol, get ready to exit*/
	
	killpg(getpgrp(),SIGTERM);
	printf("p2 terminated.\n");
	exit(0);	
}/*end of p2 main*/



int parse(){//start of parse

	int reValue; /*return value from getword()*/
	int len = 0; /*count chars in the storage array*/
	int word_number = 0; /*actual words number in the input*/
	int argc = 0; /*argument number*/
	int local_flag_in = 0; /*local flag for <*/
	int local_flag_out = 0; /*local flag for >*/
	int local_flag_pound = 0; /*local flag for #*/
	int local_pipe = 0; /*local flag for pipe*/

	for(;;){
		reValue = getword(s + len);
		
		if(local_flag_pound == 1){ /*handle # start a whole command*/
			
			if(command_flag == 1){ /*mark that this is from command line*/
			
				if(reValue == 0){ /*EOF*/ 
					command_flag == 0;
					newargv[argc] = NULL; /*set null for argument pointer array*/
					return -1;
				}
				
				if(reValue == -10){ /*when input is newline character*/
					newargv[argc] = NULL; /*set null for argument pointer array*/
					return word_number;
				}
				continue;
				
			}
			
			if(reValue == 0){ /*EOF*/ 
				return -1;
			}
			
			if(reValue == -10){ /*newline*/
				return 0;
			}
			
			continue;
		}
	
		if(local_flag_in == 1){ /*handle input redirection*/
			
			if(reValue < 0){ /*invalide input file name*/
				return -2;
			}
			
			input_file = (s + len); /*input file name for redirection*/
			len += reValue + 1; /*increase len*/
			local_flag_in = 0; /*reset the < flag*/
			word_number += 1;
			continue;
		}
	
		if(local_flag_out == 1){ /*handle output redirection*/
			
			if(reValue < 0){ /*invalide output file name*/
				return -2;
			}
			
			output_file = (s + len); /*output file name for redirection*/
			len += reValue + 1; /*increase len*/
			local_flag_out = 0; /*reset the > flag*/
			word_number += 1;
			continue;
		}
		
		if(local_pipe == 1){ /*handle pipe argument*/
			
			if(reValue == 0||reValue == -10){ /*invalide output file name*/
				return -3;
			}
			local_pipe = 0;	
		}
	
		if(reValue > 0){ /*when input word is just normal word without special meaning*/
			newargv[argc] = (s + len); /*point the argument pointer array to correct argument*/
			len += reValue + 1; /*increase len*/
			argc += 1; /*increase argument counter*/
			word_number += 1; /*increase word number counter*/
			continue;
		}
	
		if(reValue == 0){ /*when input is EOF */
			newargv[argc] = NULL; /*set null for argument pointer array*/
			return -1;
		}
	
		if(reValue == -10){ /*when input is newline character*/
			newargv[argc] = NULL; /*set null for argument pointer array*/
			return word_number;
		}
	
		if(reValue == -1){ /*handle all metacharacters*/
		
			if(*(s + len) == '<'){ /*input is input redirection symbol*/
				local_flag_in = 1; /*set local flag*/
				global_flag_in += 1; /*set global flag*/
				len += 2; /*increase len*/
				word_number += 1;
				continue;
			}
		
			if(*(s + len) == '>'){ /*input is output redirection symbol*/
				local_flag_out = 1; /*set local flag*/
				global_flag_out += 1; /*set global flag*/
				len += 2; /*increase len*/
				word_number += 1;
				continue;
			}
		
			if(*(s + len) == '#'){ /*input is # symbol*/
				
				if(command_flag == 1){
					local_flag_pound = 1; /*set local flag*/
					global_flag_pound = 1; /*set global flag*/
					word_number += 1;
					continue;
				}
				
				if(word_number == 0){ /*when # is the first word of a sentence*/
					local_flag_pound = 1; /*set local flag*/
					global_flag_pound = 1; /*set global flag*/
					word_number += 1;
					continue;
				}
			
				newargv[argc] = (s + len); /*put # into argument array*/
				len += 2;                  /*increase len*/
				argc += 1;
				word_number += 1;
				continue;
			}
		
			if(*(s + len) == '|'){ /*input is |*/
				local_pipe = 1;
				pipeposition[pipe_number] = (argc); /*pipe index in argument pointer array*/
				pipe_type[pipe_number] = 0; /*just reset*/
				pipe_number += 1;   /*keep traking number of pipe line*/
				newargv[argc] = NULL; /*set null in argument pointer array*/
				len += 2;             /*increase len*/
				argc += 1;            /*increase argument counter*/
				word_number += 1;
				continue;
			}
		
			if(*(s + len) == '&'){ /*input is &*/
				newargv[argc] = NULL; /*set null in argument pointer array*/
				global_flag_back = 1;    /*set global flag*/
				return word_number;
			}
		
		} /*end of handle metacharacters*/
		
		if(reValue == -2){ /*handle pipe redirect stderr*/
			local_pipe = 1;
			pipeposition[pipe_number] = (argc); /*pipe index in argument pointer array*/
			pipe_type[pipe_number] = 1; /*mark that pipe to redirect the stderr*/
			pipe_number += 1;   /*keep traking number of pipe line*/
			newargv[argc] = NULL; /*set null in argument pointer array*/
			len += 3;             /*increase len*/
			argc += 1;            /*increase argument counter*/
			word_number += 1;
			continue;
		}
	} /*end of for loop*/
} /*end of parse*/

void myhandler(int signum){ /*only catch the signal but do nothing*/
}
	
