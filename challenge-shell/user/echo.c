#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()#"
char buf[1024];
char cmd[1024];
int k = 0;
void execute_command(){
	int r;
	runcmd(cmd);
}

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

int my_gettoken(char *s, char **p1, char **p2) {
	*p1 = 0;
	*p2 = 0;
	if (s == 0) {
		return 0;
	}

	while (strchr(WHITESPACE, *s)) {
		s++;
	}
	if (*s == 0) {
		return 0;
	}

	if (strchr(SYMBOLS, *s)) {
		int t = *s;
		*p1 = s;
		s++;
		*p2 = s;
		return t;
	}

	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s)) {
		s++;
	}
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
			if (gettoken(0, &t, 0) != 'w') {
				debugf("syntax error: > not followed by word\n");
				my_exit(1);;
			}
			if ((r = open(t, O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
				debugf("< open error\n");
				my_exit(1);;
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
		case '|':;
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
int main(int argc, char **argv) {
    int nflag = 0;
    // char *output;

    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        nflag = 1;
        argc--;
        argv++;
    }

	for(int i=1 ;i < argc; i++){
		int len = strlen(argv[i]);
		for(int j = 0; j < len; j++){
			buf[k++] = argv[i][j];
		}
		buf[k++] = ' ';
	}
	if(buf[0] == '"'){
		buf[k-2] = 0;
		printf("%s\n",buf+1);
		return 0;
	}
	if(buf[0] == '`'){
		char *end = strchr(buf+1, '`');
		if(end != NULL) {
			int cmd_len = end - buf - 1;
			for(int j = 0; j < cmd_len; j++){
				cmd[j] = buf[j+1];
			}
			cmd[cmd_len] = '\0';
			execute_command();
        }
		else printf("%s", buf);
	}
	else printf("%s", buf);
    if (!nflag) printf("\n");
    return 0;
}