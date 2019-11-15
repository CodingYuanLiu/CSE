#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h> 

int main(){
    int fd = open("./yfs1/test-file",O_RDWR | O_CREAT,0777);
    char* buf = "123";
    char* readout;
    readout = (char *)malloc(3);
    int writesize = write(fd,buf,3);
    int readsize = read(fd,readout,3);
    printf("%s,%d\n",buf,writesize);
    printf("%s,%d\n",readout,readsize);
    close(fd);
}