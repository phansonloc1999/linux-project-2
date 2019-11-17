#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
    int fd = open("test.txt", O_WRONLY, S_IRUSR);
    printf("fd = %d\n", fd);
    write(fd, "Hello Nguyen Phuong Vy!!!\n", 36);
    return 0;
}
