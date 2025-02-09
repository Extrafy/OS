#include <lib.h>

int flag[256];

void rm(char* path){
	
	if(!flag['r']) rm_file(path);
	else if(!flag['f']){
		int r;
		r = remove(path);
		if (r < 0) user_panic("rm: cannot remove '%s': No such file or directory\n", path);
	}
	else{
		int r;
		r = remove(path);
	}
}

void rm_file(char * path){
	int r;
	struct Stat st;
	if ((r = stat(path, &st)) < 0){
		user_panic("rm: cannot remove '%s': No such file or directory\n", path);
		return;
	}
	if (st.st_isdir){
		user_panic("rm: cannot remove '%s': Is a directory\n", path);
		return;
	}
	r = remove(path);
	if (r < 0) user_panic("rm: cannot remove '%s': No such file or directory\n", path);
}

void usage(void){
    debugf("usage: rm [] [file...]\n");
    my_exit(1);
}

int main(int argc, char **argv){
	ARGBEGIN {
		default:
			usage();
		case 'r':
			flag[(u_char)ARGC()]++;
			break;
		case 'f':
			flag[(u_char)ARGC()]++;
			break;
	}ARGEND
	// debugf("%d   %d\n",flag['r'],flag['f']);
	for(int i = 0; i < argc; i++) rm(argv[i]);
	return 0;
}
