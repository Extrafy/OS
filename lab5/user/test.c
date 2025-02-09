#include <lib.h>

int main() {
    int fdnum = open("/newmotd", O_RDWR);
    if (fdnum < 0) {
        user_panic("open failed");
    }
    seek(fdnum, 114514);
    int pid = fork();
    if (pid == 0) {
        struct Fd *fd;
        if (fd_lookup(fdnum, &fd) < 0) {
            debugf("NO\n");
        } else {
            debugf("YES, THE CHILD: fd->fd_offset = %d\n", fd->fd_offset);
        }
    }
}
