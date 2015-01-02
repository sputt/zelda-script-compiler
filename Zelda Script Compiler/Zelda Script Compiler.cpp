// Zelda Script Compiler.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define SCRIPT_LIMIT	256

#define next_char skip_whitespace
#define passReplacement 0
#define passLabel       1
#define passAssemble    2

const char strict_white_space[] = " \t\n\r(),[]";
char commands[256][32];
char arg_structure[256][64];  
unsigned char script[SCRIPT_LIMIT];         //finished script
int scr_i = 0;
char source[100][256];
int source_offset[100];
int line_num = 0;
int string_offset;
int total_commands = 0;
int total_lines = 0;
char labelprefix[64];
void do_pass(char str[][256], int pass, int total_lines);

char *skip_whitespace(char *);


/*
 * Removes surrounding quotation marks (if necessary)
 * and reduces control characters
 */

char* reduce_string (char* input) {
    char *output = input;
    int i = 0;
    if (input[0] == '"') i = 1;
    
    for (; input[i] != '\0' && input[i] != '"'; i++, output++) {
        if (input[i] == '\\') {
            switch (toupper(input[++i]))
			{
			case 'N':
				*output = '\n';
				break;
			case 'T':
				*output = '\t';
				break;
			default:
				*output = input[i];
				break;
			}
        } else {
            *output = input[i];
        }
    }
    *output = '\0';
    return input;
}

void add_string(char *str) {
	static int cnt = 0;
	// allocate 12 possible lines each with 16 possible characters
	char split[12][16];
	FILE *file;
	int i, j, line;
	char *p = reduce_string(str);
	int len = 0;

	file = fopen("strings.inc", "a");

	for (j = 0; j < lstrlen(p); j++);
	p[j] = '\0';
	
	for (i = 0; i < 12 && p[0]; i++) {
		for (j = 0; j < 15 && p[j]; j++) {
			if (p[j] == '\n') {
				strncpy(split[i], p, j);
				split[i][j] = '\0';
				j++;
				break;
			}
		}
		if (p[j] == '\0') {
			strcpy(split[i], p);
		} else if (j == 15) {
			// We got to the end without splitting the string
			
			// remove trailing spaces
			int k = 14;
			while (p[k] == ' ') k--;
			
			if (k < 14) {
				strncpy(split[i], p, k+1);
				split[i][k+1] = '\0';
			} else {
				j--;
				while (p[j] != ' ' && j > 0) j--;
				if (j == 0) {
					printf("Error: string could not fit on line: %s\n", str);
					return;
				} else {
					strncpy(split[i], p, j);
					split[i][j] = '\0';
					
					while (p[j] && p[j] == ' ') j++;
				}
			}
		}
		
		p += j;
	}
	
	for (line = 0; line < i; line++) {
		if (line == i - 1) {
			len += strlen(split[line]) + 1;
		} else {
			len += strlen(split[line]);
			if (strlen(split[line]) == 15) ;
			else {
				len ++;	
			}
		}
	}
	
	fprintf(file, ";; offset %d (%d)\n", string_offset, len);
	
	
	for (line = 0; line < i; line++) {
		fprintf(file, ".db \"%s\"", split[line]);
		if (line == i - 1) {
			fprintf(file, ", 0\n");
			string_offset += strlen(split[line]) + 1;
		} else {
			string_offset += strlen(split[line]);
			
			if (strlen(split[line]) == 15) fputc('\n', file);
			else {
				fprintf(file, ", -1\n");
				string_offset ++;	
			}
		}
	}
	
	fclose(file);

}

void read_commands() {
	FILE *scriptfile;
	int i, op_i;
	char buffer[sizeof(arg_structure[0])];

    if (!(scriptfile = fopen("scriptdef.asm","r"))) {
        puts("Error opening script definition file");
        exit(1);
    }

	i = 0;
	while (fgets(buffer, sizeof(buffer), scriptfile)) {
		
		if (buffer[0] == ';') 
			break;

		if (buffer[0] == '\t') {
			for (op_i = 0; buffer[op_i] != '\n' && buffer[op_i]; op_i++);
			buffer[op_i] = 0;

            if (buffer[2] != ';') {
                //handle command list
				strcpy(commands[i], buffer + 5);
            } else {
                //handle argument definitions
                strcpy(arg_structure[i++], buffer + 3);
            }
        }
    }
	total_commands = i;
}

int read_string_offset() {
	char buffer[256];
    FILE *strings = fopen("strings.inc", "r");
	int offset = 0, len = 0;

	strings = fopen("strings.inc", "r");
	if (strings == NULL) {
		return 0;
	}

	while (fgets(buffer, sizeof(buffer), strings)) {
		sscanf(buffer, ";; offset %d (%d)", &offset, &len);
	}

	fclose(strings);
    
	return offset+len;
}

char* get_span(char* str, char* buffer, char limit) {
    int i;
    bool in_quote = false;
	int paren_level = 0;
    for (i = 0; *str != 0; str++) {
        if (*str == limit && !in_quote && paren_level == 0) {
            if (i) break;
        } else {
			if (*str == '"')
				in_quote = !in_quote;
			else if (*str == '(')
				paren_level++;
			else if (*str == ')')
				paren_level--;
            buffer[i++] = *str;
        }
    }
    buffer[i] = 0;
    if (*str == 0 && !i) {
        return NULL;
    }
    return str;
}

char *get_word(char* str, char* buffer) {
    int i, op_i;
    for (i = 0; *str != 0; str++) {
        for (op_i = 0; op_i < 9 && (*str != strict_white_space[op_i]); op_i++);
        if (op_i < 9) {
            if (i) break;
        } else {
            buffer[i++] = *str;
        }
    }
    buffer[i] = 0;
    if (*str == 0 && !i) {
        return NULL;
    }
    return str;
}

void labelize_script(char *name) {
	char *p;

	p = strchr(name, '.');
	if (p)
		*p = '\0';

	p = strchr(name, '-');
	while (p != NULL) {
		*p = '_';
		p = strchr(p + 1, '-');
	}

	p = strchr(name, ' ');
	while (p != NULL) {
		*p = '_';
		p = strchr(p + 1, ' ');
	}

	p = strrchr(name, '\\');
	if (p != NULL)
		strcpy(name, p + 1);
}

void read_code(char *filename) {
	char buffer[256];
    int row_index = 0, arg_i = 0, i = 0;
    char *ptr = NULL;
	FILE *scriptFile;

	scriptFile = fopen(filename, "r");

    while (fgets(buffer, sizeof(buffer), scriptFile)) {
        if (*next_char(buffer) == '\n' || *next_char(buffer) == '\r' || *next_char(buffer) == ';')
			continue;

        //ptr = next_char(buffer);
        //pack_string(ptr);
        if (i) ptr = source[row_index]+i; // continue a previous line
        else ptr = source[row_index];
            
        for (i = 0; buffer[i]!='\n' && !(buffer[i]==';' || buffer[i]==':'); i++);
        if (buffer[i] == ';' || buffer[i]==':') { //we hit a ';'
            buffer[i] = 0;
            i = 0;
            if (ptr > source[row_index]) strcpy(ptr,next_char(buffer));
            else strcpy(ptr,buffer);
//            puts(source[row_index]);
            row_index++;
        } else { //if there was a line end without ';'
            char *tptr;
            buffer[i] = 0;
            if (ptr > source[row_index]) tptr = next_char(buffer);
            else tptr = buffer;
            strcpy(ptr,tptr);
            i = (ptr - source[row_index]) + strlen(tptr); //clip off terminating zero
        }
    }

	total_lines = row_index;
}

FILE *outfile;

int _tmain(int argc, _TCHAR* argv[])
{
	int i;
	char buffer[MAX_PATH];

	if (argc < 2) {
        printf("Usage: %s <input file>", argv[0]);
        return 0;
    }

	read_commands();
	string_offset = read_string_offset();
	read_code(argv[1]);

	strcpy(labelprefix, argv[1]);
	labelize_script(labelprefix);
	strcat(labelprefix, "_");

	strcpy(buffer, argv[1]);
	for (i = 0; buffer[i] != '.'; i++);
	strcpy(buffer+i+1,"ccr");

	outfile = fopen(buffer, "w");

	fprintf(outfile, "_: ;%s", argv[1]);
	do_pass(source, passReplacement, total_lines);
	do_pass(source, passAssemble, total_lines);

	fclose(outfile);
	return 0;
}

char *skip_whitespace (char *ptr) {
	while (isspace (*ptr) && *ptr != '\n' && *ptr != '\r')
		ptr++;

	return ptr;
}

int parse_f(char *value) {
	return 0;
}

void add_label(char *name, int value) {
	printf("Setting %s to %d\n");
}

void *search_labels(char *value) {
	printf("Searching labels\n");
	return NULL;
}

void *add_define(char *name, void *pv) {
	return NULL;
}

void set_define(void *define, char *name, int i, BOOL b) {
	return;
}


void arg_replace(char *str, char *arg, char *replace, BOOL bSurround) {
	char *dest, *src;
	int len;

	char *p = strstr(str, arg);
	if (p == NULL)
		return;

	dest = p + strlen(replace) + (bSurround ? 2 : 0);
	src = p + strlen(arg);
	len = strlen(p) - strlen(arg) + 1;
	memmove(dest, src, len);
	if (bSurround)
		*p++ = '(';
	strncpy(p, replace, strlen(replace));
	if (bSurround)
		p[strlen(replace)] = ')';
}


void do_pass(char str[][256], int pass, int total_lines) {
    int script_offset = 0,i;
    unsigned int result;
    char parsebuf[128],*ptr;
    char namebuf[16],resultbuf[128];

    line_num = 0;
    
    for (; line_num < total_lines; line_num++) {
		//printf("Scriptoffset: %d - %s\n", script_offset, str[line_num]);
        if (pass == passReplacement) {
			int in_string = 0;
			char *label;

            for (i = 0; str[line_num][i] && !(str[line_num][i] == '.' && in_string == 0); i++) {
				if (str[line_num][i] == '"') in_string = !in_string;
            }
            if (str[line_num][i]) {
                char indexbuf[256];
                for (i = 0; str[line_num][i] && str[line_num][i]!='['; i++);
                if (!str[line_num][i]) {
                    printf("Bad access: %s\n",str[line_num]);
                    return;
                }
                ptr = get_word(str[line_num],namebuf);
                ptr = get_word(ptr,indexbuf);
                ptr += 2;
                //puts(parsebuf);
                //printf("parsing: %s\n", parsebuf);
                ptr = get_word(ptr,parsebuf);
                ptr = next_char(ptr);
                ptr = next_char(ptr + 1);
                //puts(ptr);
                //result2 = parse_f(ptr);
                //puts("what pointer currently has");
                //puts(ptr);
                strcpy(resultbuf,namebuf);
                strcat(resultbuf,"_");
                strcat(resultbuf,parsebuf);
				strcat(resultbuf,"_abs");
                strcpy(parsebuf,ptr);
                //printf("Starting search cycle with %s\n",resultbuf);
                //printf("Searching with %s...\n",resultbuf);
				// printf(" SET_%s_ATTR8(%d,%s,%s)\n",namebuf,result,resultbuf,parsebuf);
				strupr(namebuf);
				if (strstr(strupr(resultbuf),"PTR") != NULL)
					sprintf(str[line_num]," SET_%s_ATTR16(%s,%s,%s)\n",namebuf,indexbuf,resultbuf,parsebuf);                  
				else
					sprintf(str[line_num]," SET_%s_ATTR8(%s,%s,%s)\n",namebuf,indexbuf,resultbuf,parsebuf);
            }

			label = strchr(str[line_num], '@');
			if (label != NULL) {
				memmove(label + strlen(labelprefix), label + 1, strlen(label) - 1);
				strncpy(label, labelprefix, strlen(labelprefix));

				if (label == str[line_num]) {
					char *label_end = strchr(label, ':');
					if (label_end == NULL)
						label_end = label + strlen(label);
					strcpy(label_end, " = $ - -_");
				}
			}

        } else if (pass == passLabel || pass == passAssemble) {
        	
			fputc('\n', outfile);

           // if (pass == passAssemble) printf("%d: %s", line_num, str[line_num]);
            if (pass == passLabel) source_offset[line_num] = script_offset;
            //printf("Offset: %d: %s\n",script_offset,str[line_num]);
            if (next_char(str[line_num]) > str[line_num]) { // if the first character is spaced forward, it's a command
                ptr = get_word(str[line_num],parsebuf);
                for (i = 0; i < total_commands && stricmp(parsebuf,commands[i]); i++);
                
                if (i < total_commands) {
                    char *argptr;
                    int argcount = 0,ci;
					char expandedarg[256];
					char transition[16] = ", ";
                    
                    script_offset++; //add one for the "opcode"
                    
                    if (pass == passAssemble)
						fprintf(outfile, ".db $%02x", i);
                    
                    ptr = next_char(ptr);
                    if (ptr && *ptr == '(') ptr++;
                    else {
                        //if (pass == passAssemble) script[scr_i++] = i;
                        continue;
                        // it's a singler.
                        // output the byte.
                    }
                    /* parse values of arguments */
                    if (pass == passAssemble) {
                        strcpy(namebuf,"argX");
                        ci = 0;
						strcpy(expandedarg, arg_structure[i]);
						arg_replace(expandedarg, ",*", "\\ .dw ", FALSE);
						if (expandedarg[0] == '*') {
							arg_replace(expandedarg, "*", "\\ .dw ", FALSE);
							strcpy(transition, " ");
						}
                        for (ptr = get_span(ptr,resultbuf,','); ptr; ptr = get_span(ptr,resultbuf,',')) {
							if (ci == 9) ci = -1;
                            namebuf[3] = '1'+ci;
                            //puts(namebuf);
                            
                            if (!*ptr) get_span(resultbuf,resultbuf,')');

							char test_word[256];
							get_word(resultbuf, test_word);
                            if (strcmp(test_word,"*") != 0) {
								if (*(skip_whitespace(resultbuf)) == '"') {
									int old_offset = string_offset;
									//string situations
									add_string(skip_whitespace(resultbuf));
									char buf[256];
									strcpy(buf, "SCRIPT_STRINGS + ");
									char numbuf[256];
									sprintf(numbuf, "%d", old_offset);
									strcat(buf, numbuf);
									set_define(add_define(namebuf, NULL), _strdup(buf), -1, false);
									arg_replace(expandedarg, namebuf, buf, TRUE);
								} else {
									set_define(add_define(namebuf, NULL), _strdup(resultbuf), -1, false);
									//printf("%s assigned to: %s\n",namebuf,resultbuf);
									arg_replace(expandedarg, namebuf, resultbuf, TRUE);
								}
                            } else {
                                //printf("Found label thing: Setting %s to %d\n",namebuf,source_offset[line_num+1]);
                                char buf[256];
                                sprintf(buf, "%d", source_offset[line_num+1]);
                                set_define(add_define(namebuf, NULL), _strdup(buf), -1, false);
								arg_replace(expandedarg, namebuf, "@&next", TRUE);
                            }
                            //puts(ptr);
                            
                            ci++;               
                        }
						
                    }

                    for (argptr = get_span(arg_structure[i],parsebuf,','); argptr; argptr = get_span(argptr,parsebuf,',')) {
                        if (*parsebuf=='*') {
                            script_offset+=2;
                            if (pass == passAssemble) {
                                ci = parse_f(skip_whitespace(parsebuf)+1);
                                script[scr_i++] = ci & 0xFF;
                                script[scr_i++] = ci >> 8;
                            }
                        } else {
                            script_offset++;
                            if (pass == passAssemble) {
								script[scr_i++] = parse_f(parsebuf) & 0xFF;
                            }
                        }
                        argcount++;
                    }

					if (pass == passAssemble) {
						char stroffset[256];
						sprintf(stroffset, "%d", script_offset);
						arg_replace(expandedarg, "@&next", stroffset, TRUE);
						fprintf(outfile, "%s%s", transition, expandedarg);
					}
                        
                } else {
                    fprintf(stderr, "Invalid command %s\n.", str[line_num]);
                    return;
                }
            } else if (pass == passLabel || pass == passAssemble) {
                fprintf(outfile, "%s", str[line_num]);
            }
        }
    }
}