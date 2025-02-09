#include <lib.h>

int flag[256];
void createRec(char *p) {
	char *t = p;
	int r;
	if (*p == '/') t = p + 1;
	while (*t) {
		while (*t && *t != '/') t++;
		char save = *t;
        *t = '\0';
		r = user_create(p, 1);
		if (r != -E_FILE_EXISTS && r!=0) user_panic("damn");
		*t = save;
		if (*t == '/') t++;		
	}
}

void mkdir(char* path) {
	int r;
	// debugf("%d\n",flag['p']);
	if (flag['p']) createRec(path);
	else {
		r = user_create(path, 1);
		if (r == -E_FILE_EXISTS) user_panic("mkdir: cannot create directory '%s': File exists\n", path);
		else if (r < 0) user_panic("mkdir: cannot create directory '%s': No such file or directory\n", path);
	}
}

void usage(void) {
	debugf("usage: mkdir [-p] [file...]\n");
	my_exit(1);
}

int main(int argc, char **argv) {
	ARGBEGIN{
	default:
		usage();
	case 'p':
		flag[(u_char)ARGC()]++;
		break;
	} ARGEND
	if (argc == 0) debugf("please input a directory name\n");
	else {
		for (int i = 0; i < argc; i++) mkdir(argv[i]);
	}
	return 0;
}
