#include <lib.h>

void touch(char* path){
	int r;
	r = user_create(path, 0);
	if (r == -E_FILE_EXISTS) return;
	else if (r < 0) user_panic("touch: cannot touch '%s': No such file or directory\n", path);
}

void usage(void){
    debugf("usage: touch [] [file...]\n");
    my_exit(1);
}

int main(int argc, char **argv){
	ARGBEGIN {
		default:
			usage();
	}ARGEND
	for(int i = 0; i < argc; i++) touch(argv[i]);
	return 0;
}
