/*Jialin Cui CS570 Carroll 02/25/2018
getword.c is the c file for getword() function
getword() function get every word from the commmand input and put them into storage array one by one with a \0 at the end of each word. getword() return -1 on matacharacters, -10 in newline, 0 for EOF, and -2 on some special combations.
The reason why you use ungetc() is that you need to put that char back you buffer so that you can get it again
since the last char you get have some special meaning but you used it up to detect the special behavior, but you
do not want to lose it, so you have to put it back.
*/
#include "getword.h"
int getword (char *w){
	int input;
	int flag_backslash = 0;
	/*mark that the previous input is '\'*/
	int flag_pipe = 0;
	/*mark that the prvious input is '|'*/
	int count=0;
	/*count the number of characters in a word*/
	
	while((input = getchar()) != EOF){
	/*if input is eof then stop the while loop*/
	
		if(count == 254){
			ungetc(input,stdin);/*when the string is full but you need to unget the last char otherwise you lose it*/
			break;
		}
	
		if(input != '&' && flag_pipe == 1){
		/*if the current input is not & then clear the flag2*/
		*(w + count) = '\0';
		ungetc(input,stdin);
		flag_pipe = 0;
		return -1;
		}
	
		if(flag_backslash == 1){
		/*when previous input character is '\'*/
	
			if(input == '\n'){
			/*current input is newline character*/
	
				if(count != 0){
				/*newline character end the current word*/
				*(w + count) = '\0';
				ungetc(input,stdin);
				return count;
				}
	
				else{
				/*only newline character*/
				*w='\0';
				return -10;
				}
			}
		
			else{
			/*current input is not newline character*/
			*(w + count) = input;
			count++;
			}
			flag_backslash = 0; /*reset back slash flag*/
		}
	
		else{
		/*when previous input character is not '\'*/
	
			if(input == '\\'){
			/*when current input is '\'*/
				flag_backslash = 1; /*set backslash flag*/
				continue;
			}
	
			if(input == ' ' && count != 0){
			/*blank space end a word*/
				*(w + count) = '\0';
				return count;
			}
	
			if(input == '<' || input == '>' || input == '\n'){
			/*current is '<','>',or '\n'*/
			
				if(count != 0){
				/*end the previous word*/
					*(w + count) = '\0';
					ungetc(input,stdin);
					return count;
				}
			
				else{
				/*count is 0*/
			
					if(input == '\n'){
						/*input is '\n'*/
						*w = '\0';
						return -10;
					}
				
					else{
					/*input is '<' or '>'*/
						*w = input;
						*(w + 1)='\0';
						return -1;
					}
				}
			}
	
			if(input =='#' && count == 0){
			/*when '#' start a new word*/
				*w = input;
				*(w + 1) = '\0';
				return -1;
			}
	
			if(input == '|'){
			/*current input is '|'*/
		
				if(count != 0){
				/*end the previous word*/
					*(w + count) = '\0';
					ungetc(input,stdin);
					return count;
				}
			
				else{
				/*'|' start a new word*/
					flag_pipe = 1;
					*w = input;
					count = count + 1;
					continue;
				}
			}
	
			if(input == '&'){
			/*current input is '&'*/
		
				if(count == 0){
				/*'&' start a new word*/
					*w = input;
					*(w + 1) = '\0';
					return -1;
				}
			
				else{
				/*'&' end a word*/
			
					if(flag_pipe == 1){
					/*previous input is '|'*/
						*(w + count) = '&';
						*(w + count + 1) = '\0';
						flag_pipe = 0; /*reset pipe flag*/
						return -2;
					}
				
					else{
					/*just end previous word*/
						*(w + count) = '\0';
						ungetc(input,stdin);
						return count;
					}
				}
			}
	
			if(input != ' '){
			/*ignore the leading blank space*/
				*(w + count) = input;
				count=count+1;
			}
		}
	}/*end of while loop*/
	
	if(count != 0 && flag_pipe != 1){
	/*when eof is entered to end a word*/
		*(w + count)='\0';
		return count;
	}
	
	else if(flag_pipe == 1){
		*(w + count) = '\0';
		return -1;
	}
	else{
	/*when eof is entered to end the input file*/
	*w='\0';
	return 0;
	}
}
