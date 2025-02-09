#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()#"

/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes ('\0'), so that the
 *   returned token is a null-terminated string.
 */
int _gettoken(char *s, char **p1, char **p2) {
	*p1 = 0;
	*p2 = 0;
	if (s == 0) {
		return 0;
	}

	while (strchr(WHITESPACE, *s)) {
		*s++ = 0;
	}
	if (*s == 0) {
		return 0;
	}

	if (strchr(SYMBOLS, *s)) {
		int t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		return t;
	}

	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s)) {
		s++;
	}
	*p2 = s;
	return 'w';
}

int gettoken(char *s, char **p1, int flag) {
	static int c, nc;
	static char *np1, *np2;
	if(flag == 0){
		if (s) {
			nc = _gettoken(s, &np1, &np2);
			return 0;
		}
		c = nc;
		*p1 = np1;
		nc = _gettoken(np2, &np1, &np2);
		return c;
	}
	else{
		c = nc;
		*p1 = np1;
		np2 = np1 - 3;
		np1 -= 4;
		return c;
	}
}

#define MAXARGS 128

int flag = 0;
int flag_double = 0;
int note = 0;
int parsecmd(char **argv, int *rightpipe) {
	int argc = 0;
	int last_status = -1;
	int pid = 0;
	while (1) {
		char *t;
		int fd, r;
		int c = gettoken(0, &t, 0);
		if( t != 0){
			if(*t == '`') flag = 1;
			if(*t == '"') flag_double = 1;
		}
		switch (c) {
		case 0:
			return argc;
		case 'w':
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				my_exit(1);;
			}
			argv[argc++] = t;
			break;
		case '<':
			if(flag || flag_double){
				char *temp = "<";
				argv[argc++] = temp;
				break;
			}
			if (gettoken(0, &t, 0) != 'w') {
				debugf("syntax error: < not followed by word\n");
				my_exit(1);;
			}
			// Open 't' for reading, dup it onto fd 0, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (1/3) */
			if((r = open(t, O_RDONLY)) < 0){
				debugf("< open error\n");
				my_exit(1);;
			}
			fd = r;
			dup(fd, 0);
			close(fd);
			break;
		case '>':
			if(flag || flag_double){
				char *temp = ">";
				argv[argc++] = temp;
				break;
			}
			char ch = gettoken(0, &t, 0);
			if (ch == '>'){
				if (gettoken(0, &t, 0) != 'w') {
					debugf("syntax error: >> not followed by word\n");
					my_exit(1);
				}
				if ((r = open(t, O_WRONLY | O_CREAT)) < 0) {
					debugf(">> open error\n");
					my_exit(1);
				}
				fd = r;
				struct Fd* fd2;
				struct Filefd* ffd2;
				fd_lookup(fd , &fd2);
				ffd2 = (struct Filefd*) fd2;
				seek(fd, ffd2->f_file.f_size);
				
				dup(fd, 1);
				close(fd);
				break;
			}
			else if (ch != 'w') {
				debugf("syntax error: > not followed by word\n");
				my_exit(1);
			}
			// Open 't' for writing, create it if not exist and trunc it if exist, dup
			// it onto fd 1, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (2/3) */
			if ((r = open(t, O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
				debugf("< open error\n");
				my_exit(1);
			}
			fd = r;
			dup(fd, 1);
			close(fd);
			break;
		case '&':
			if(flag || flag_double){
				char *temp = "&";
				argv[argc++] = temp;
				break;
			}
            if (gettoken(0, &t, 0) == '&' && argc > 0) {
				if(argc != 0) last_status = fork_and_run(argv, argc);
				if (last_status != 0) {
					skip_next_command();
				}
				argc = 0;
				break;
            }
			else {
                debugf("syntax error: & not followed by &\n");
                my_exit(1);;
            }
            break;
		case '#':
			if(flag || flag_double){
				char *temp = "#";
				argv[argc++] = temp;
				break;
			}
			note = 1;
			break;

		case ';':
			pid = fork();
			if (pid == 0) {
				return argc;
			}
			else {
				int whom;
				int rev = ipc_recv(&whom, 0, 0);
				*rightpipe = 0;
				return parsecmd(argv, rightpipe);
			}
			break;
		case '|':;
			/*
			 * First, allocate a pipe.
			 * Then fork, set '*rightpipe' to the returned child envid or zero.
			 * The child runs the right side of the pipe:
			 * - dup the read end of the pipe onto 0
			 * - close the read end of the pipe
			 * - close the write end of the pipe
			 * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of the
			 *   command line.
			 * The parent runs the left side of the pipe:
			 * - dup the write end of the pipe onto 1
			 * - close the write end of the pipe
			 * - close the read end of the pipe
			 * - and 'return argc', to execute the left of the pipeline.
			 */
			if(flag || flag_double){
				char *temp = "|";
				argv[argc++] = temp;
				break;
			}
			if (gettoken(0, &t, 0) == '|' && argc > 0) {
				if(argc != 0) last_status = fork_and_run(argv, argc);
				if (last_status == 0) {
					skip_next_command();
				}
				argc = 0;
				break;
            }
			else {
				int p[2];
				/* Exercise 6.5: Your code here. (3/3) */
				pipe(p);
				*rightpipe = fork();
				if(*rightpipe == 0){
					dup(p[0], 0);
					close(p[0]);
					close(p[1]);
					return parsecmd(argv, rightpipe);
				}
				else{
					dup(p[1], 1);
					close(p[1]);
					close(p[0]);
					return argc;
				}
            }
			break;
		}
		if(note) return argc; 
	}

	return argc;
}

void skip_next_command() {
    char *t;
    int c;
    while ((c = gettoken(0, &t, 0)) != 0) {
		if(c == '&'){
			c = gettoken(0, &t, 0);
			if(c == 0 || c == '&') break;
		}
		else if(c == '|'){
			c = gettoken(0, &t, 0);
			if(c == 0 || c == '|') break;
		}
        // 只消耗标记直到下一个 && 或 ||
    }
}

int fork_and_run(char **argv, int argc) {
    int child = fork();
    if (child == 0) {
        argv[argc] = 0;
        int r = spawn(argv[0], argv);
		int t;
		int status = ipc_recv(&t, 0, 0);
		if(status == 1) my_exit(1);
		close_all();
		my_exit(0);
    }
	else {
		int t;
		int r = ipc_recv(&t, 0, 0);
		// debugf("recv: %d\n", r);
		return r;
    }
}

void runcmd(char *s) {
	gettoken(s, 0, 0);

	char *argv[MAXARGS];
	int rightpipe = 0;
	int argc = parsecmd(argv, &rightpipe);
	if (argc == 0) {
		return;
	}
	argv[argc] = 0;
	char *history = "history";
	if(strcmp(history,argv[0]) == 0){
		// debugf("success\n");
		run_history(argc, argv);
		my_exit(0);
	}
	else{
		int child = spawn(argv[0], argv);
		close_all();
		int rev = 0;
		if (child >= 0) {
			// wait(child);
			int whom;
			rev = ipc_recv(&whom, 0, 0);
		} else {
			debugf("spawn %s: %d\n", argv[0], child);
			my_exit(1);
		}
		if (rightpipe) {
			// wait(rightpipe);
			int whom;
			rev = ipc_recv(&whom, 0, 0);
		}
		my_exit(rev);
	}
}

void readline(char *buf, u_int n) {
    int r;
    int repeat;

    for (int i = 0; i < n; i++) {
        if ((r = read(0, buf + i, 1)) != 1) {
            if (r < 0) {
                debugf("read error: %d\n", r);
            }
            my_exit(1);
        }

        if (buf[i] == '\b' || buf[i] == 0x7f) {
            if (i > 0) {
                i -= 2;
            } else {
                i = -1;
            }
            printf("\b \b");
            continue;
        }

        if (buf[i] == '\x1b') {  // Handle escape sequences
            i++;
            if ((r = read(0, buf + i, 1)) != 1) {
                if (r < 0) {
                    debugf("read error: %d\n", r);
                }
                my_exit(1);
            }
            if (buf[i] != '[') {
                debugf("unexpected escape sequence\n");
                my_exit(1);
            }
            i++;
            if ((r = read(0, buf + i, 1)) != 1) {
                if (r < 0) {
                    debugf("read error: %d\n", r);
                }
                my_exit(1);
            }

            if (buf[i] == 'A') {  // Up arrow key
                debugf("\x1b[1B\x1b[2K");
                get_history_cmd(buf, 0, 1, &repeat);
                debugf("%s", buf);
            } else if (buf[i] == 'B') {  // Down arrow key
                debugf("\x1b[%dD\x1b[K", i);
                get_history_cmd(buf, 0, 0, &repeat);
                debugf("%s", buf);
            }
            i = strlen(buf) - 1;
            continue;
        }

        if (buf[i] == '\r' || buf[i] == '\n') {
            buf[i] = 0;
            get_history_cmd(buf, 0, -1, &repeat);
            if (!repeat) {
                save_history_cmd(buf);
            }
            return;
        }
    }
    
    debugf("line too long\n");
    while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n') {
        ;
    }
    buf[0] = 0;
}

char buf[1024];

void usage(void) {
	printf("usage: sh [-ix] [script-file]\n");
	my_exit(1);;
}

int main(int argc, char **argv) {
	int r;
	int interactive = iscons(0);
	int echocmds = 0;
	printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	printf("::                                                         ::\n");
	printf("::                     MOS Shell 2024                      ::\n");
	printf("::                                                         ::\n");
	printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	ARGBEGIN {
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}
	ARGEND

	if (argc > 1) {
		usage();
	}
	if (argc == 1) {
		close(0);
		if ((r = open(argv[0], O_RDONLY)) < 0) {
			user_panic("open %s: %d", argv[0], r);
		}
		user_assert(r == 0);
	}
	for (;;) {
		if (interactive) {
			printf("\n$ ");
		}
		get_history_cmd(0, 1, 0, 0);
		readline(buf, sizeof buf);

		if (buf[0] == '#') {
			continue;
		}
		if (echocmds) {
			printf("# %s\n", buf);
		}
		if ((r = fork()) < 0) {
			user_panic("fork: %d", r);
		}
		if (r == 0) {
			runcmd(buf);
			my_exit(0);
		} else {
			int whom;
			int rev = ipc_recv(&whom, 0, 0);
			// wait(r);
		}
	}
	return 0;
}

char history_flag[256];

char history_buf[8192];

int to_num(char *s) {
	int ans = 0;
	while(*s) {
		ans = ans * 10 + (*s) - '0';
		s++;
	}
	return ans;
}

void history(int arg) {
	int fd, n;
	int r;
	int i;
	int line;
	int fast = -1;
	int allLine = 0;
	if (history_flag['c']) {
    	if ((fd = open("/.mosh_history", O_RDONLY)) < 0)
        	user_panic("open %d: error in histroy\n",  fd);
        if ((r = seek(fd, 0)) < 0) {
        	user_panic("error in seek history\n");
		}
		r = ftruncate(fd, 0);
		if (r) user_panic("error in ftruncate in history\n");
		return;
	}
	if (arg == -1) {
    	if ((fd = open("/.mosh_history", O_RDONLY)) < 0)
        	user_panic("open %d: error in histroy\n",  fd);
        if ((r = seek(fd, 0)) < 0) {
        	user_panic("error in seek history\n");
		}
		n=read(fd, history_buf, (long)sizeof history_buf);
		line = 1;
		for (i = 0; i < n; i++) {
			debugf("%c", *(history_buf + i));
			if (*(history_buf + i) == '\n') {
				if (!history_buf[i + 1]) break;
			}
		}
	} else {
		if ((fd = open("/.mosh_history", O_RDWR | O_APPEND)) < 0)
        	user_panic("open %d: error in histroy\n",  fd);
	    if ((r = seek(fd, 0)) < 0) {
	        user_panic("error in seek history\n");
	    }
    	n = read(fd, history_buf, (long)sizeof history_buf);
    	for (i = 0; i < n; i++) {
    		if (*(history_buf + i) == '\n') {
	            allLine++;
	        }
		}
		for (i = n - 1; i >= 0; i--) {
	        if (*(history_buf + i) == '\n') {
	            fast++;
	            if (fast == arg) {
	            	i++;
	            	break;
				}
	        }
			if (i == 0) {
				fast++;
				break;
			}
    	}
		line = ( allLine - arg + 1 ) > 0 ? ( allLine - arg + 1 ) : 1;
		for (; i < n; i++) {
			debugf("%c", *(history_buf + i));
			if (*(history_buf + i) == '\n') {
				if (!history_buf[i + 1]) break;
			}
		}
	}
}


void save_history_cmd(char *buf) {
	int fd, n;
	int r;
    if ((fd = open("/.mosh_history", O_RDWR)) < 0)
        user_panic("open %d error in save_history_cmd\n", fd);
	struct Fd* fd2;
	struct Filefd* ffd2;
	fd_lookup(fd , &fd2);
	ffd2 = (struct Filefd*) fd2;
	seek(fd, ffd2->f_file.f_size);
    r = write(fd, buf, strlen(buf));
    if (r < 0) {
    	user_panic("error in save_history_cmd of write Buf\n");
	}
	r = write(fd, "\n", 1);
	if (r < 0) {
    	user_panic("error in save_history_cmd of write enter\n");
	}
	close(fd);
}

void get_history_cmd(char* command, int clear, int upOrDown, int* repeat){
	static int curLine = 0;
	int fd, n;
	int r;
	int i;
	char buf[8192];
	char cmp[256];
	int j;
	//upOrDown 1 is up, 0 is down
	if (clear) {
		curLine = 0;
		return;
	}
	if ((fd = open("/.mosh_history", O_RDONLY)) < 0)
    	user_panic("open %s: error  in histroy\n", fd);
    if ((r = seek(fd, 0)) < 0) {
        user_panic("error in seek history\n");
    }
    n = read(fd, buf, (long)sizeof buf);
	if (upOrDown == 1) {
		curLine++;
	} else if (upOrDown == 0){
		curLine--;
	} else if (upOrDown == -1) {
	    for (i = n - 2; i > 0; i--) {
    	    if (buf[i] == '\n') {
        	    i++;
            	break;
        	}
   		}
   		for (j = 0; i < n; i++, j++){
        	cmp[j] = buf[i];
        	if (buf[i] == '\n') {
            	cmp[j] = 0;
            	break;
        	}
    	}
    	if (strcmp(cmp, command) == 0) {
        	*repeat = 1;
    	} else {
        	*repeat = 0;
    	}
		close(fd);
		return;
	} 
	int fast = -1;
    for (i = n - 1; i >= 0; i--) {
        if (*(buf + i) == '\n') {
            fast++;
            if (fast >= curLine) {
            	i++;
            	break;
			}
        }
		if (i == 0) {
			fast++;
			break;
		}
    }
    char* tmp = command;
    for (; i < n; i++, tmp++){
    	*tmp = buf[i];
    	if (buf[i] == '\n') {
			*tmp = 0;
    		break;
		}
	}
	for (i = n - 2; i > 0; i--) {
		if (buf[i] == '\n') {
			i++;
			break;
		}
	}
	for (j = 0; i < n; i++, j++){
		cmp[j] = buf[i];
		if (buf[i] == '\n') {
			cmp[j] = 0;
			break;
		}
	}
	if (strcmp(cmp, command) == 0) {
		*repeat = 1;
	} else {
		*repeat = 0;
	}
    if (fast <= curLine) {
        curLine = fast;
    }
    if (curLine <= 0) {
        curLine = 0;
        command[0] = 0;
    }
	close(fd);
}

int run_history(int argc, char **argv){
    int i;

    ARGBEGIN{
    default:
        usage();
    case 'c':
        history_flag[(u_char)ARGC()]++;
        break;
    }ARGEND
    if (argc == 1) {
    	history(to_num(argv[0]));
	} else {
		history(-1);
	}
	return 0;
}

