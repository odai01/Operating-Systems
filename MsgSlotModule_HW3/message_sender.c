#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAJOR_NUM 235
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM,0,unsigned int)

int main(int argc, char *argv[]){

    if(argc!=4){
        perror("Invalid number of passed arguments.");
        exit(1);
    }

    int fd=open(argv[1],O_WRONLY);
    if(fd<0){
        perror("An error has occured opening the device file.");
        exit(1);
    }

    unsigned long channel_id=strtoul(argv[2],NULL,10);

    if(ioctl(fd,MSG_SLOT_CHANNEL,channel_id)<0){
        perror("An error has occured setting the channel id.");
        close(fd);
        exit(1);
    }
    if(write(fd,argv[3],strlen(argv[3]))<0){
        perror("An error has occured writing the message.");
        close(fd);
        exit(1);
    }

    close(fd);
    return 0; 


}