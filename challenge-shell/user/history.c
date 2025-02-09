#include <lib.h>

char history_flag[256];

char history_buf[8192];

int toNum(char *s) {
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
	//	seek(fd, 0);	
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
	//	seek(fd, 0);
		n=read(fd, history_buf, (long)sizeof history_buf);
		line = 1;
		debugf(" %d ", line);
		for (i = 0; i < n; i++) {
			debugf("%c", *(history_buf + i));
			if (*(history_buf + i) == '\n') {
				if (history_buf[i + 1]) {
					debugf(" %d ", ++line);
				} else {
					break;
				}
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
		debugf(" %d ", line);
		for (; i < n; i++) {
			debugf("%c", *(history_buf + i));
			if (*(history_buf + i) == '\n') {
				if (history_buf[i + 1]) {
					debugf(" %d ", ++line);
				} else {
					break;
				}
			}
		}
	}
}

void usage(void){
    debugf("usage: touch [] [file...]\n");
    exit();
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
    	history(toNum(argv[0]));
	} else {
		history(-1);
	}
	return 0;
}
