#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

static volatile sig_atomic_t running = 1;

static void sig_handler(int sig)
{
    (void)sig;
    running = 0;
}

int main(int argc, char *argv[])
{
    int fd;
    char *filename;
    unsigned short databuf[3];
    unsigned short ir, als, ps;
    int ret;

    if (argc != 2) {
        printf("Error Usage: %s <device>\n", argv[0]);
        return -1;
    }

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    filename = argv[1];
    fd = open(filename, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    while (running) {
        ret = read(fd, databuf, sizeof(databuf));
        if (ret > 0) {
            ir  = databuf[0];
            als = databuf[1];
            ps  = databuf[2];
            printf("ir = %d, als = %d, ps = %d\n", ir, als, ps);
        } else if (ret < 0) {
            perror("read");
            break;
        }
        usleep(200000);
    }

    close(fd);
    return 0;
}
