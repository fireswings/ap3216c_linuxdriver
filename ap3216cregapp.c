#include <stdio.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h> 

static struct input_event inputevent;

int main(int argc, char *argv[])
{
    int fd;
    char *filename;
    unsigned short databuf[3];
    unsigned short ir, als, ps;
    int ret = 0;

    if(argc != 2) {
        printf("Error Usage!\n");
        return -1;
    }
    
    filename = argv[1];
    fd = open(filename, O_RDWR);
    if(fd < 0) {
        printf("can't open file %s\n", filename);
        return -1;
    }

    while (1)
    {
        ret = read(fd, databuf, sizeof(databuf));
        if(ret == 0) {
            ir = databuf[0];
            als = databuf[1];
            ps = databuf[2];
            printf("ir = %d, als = %d, ps = %d\n", ir, als, ps);
        }
        usleep(200000);
    }
    close(fd);

    return 0;    
}