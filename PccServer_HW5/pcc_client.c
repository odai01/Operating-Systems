#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char *argv[]){
    /*i will try to provide a note above each step as desribed in the assignment file*/
    /*validating the number of passed arguments*/
    if(argc!=4){
        fprintf(stderr,"Incorrect number of passed arguments.\n");
        exit(1);
    }

    char *server_ip=argv[1];
    unsigned short server_port=(unsigned short) strtoul(argv[2],NULL,10);
    char *file_path=argv[3];

    /*opening the file*/
    int file=open(file_path,O_RDONLY);
    if(file<0){
        perror("An error has occured opening the file.");
        exit(1);
    }

    /*creating a tcp socket*/
    int client_socket=socket(AF_INET,SOCK_STREAM,0);
    if(client_socket<0){
        perror("An error has occured creating a tcp scocket.");
        exit(1);
    }
    
    
    struct sockaddr_in server_addr;
    /*according to the internet it's a good practice to
     do memset to clear server_addr before using it*/
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(server_port);
    
    if(inet_pton(AF_INET,server_ip,&server_addr.sin_addr)<=0){
        perror("An error has occured dealing with the IP address, maybe it's invalid.");
        close(file);
        close(client_socket);
        exit(1);
    }

    /*connecting to the server*/
    if(connect(client_socket,(struct sockaddr *) &server_addr,sizeof(server_addr))<0){
        perror("An error has occured connecting to the server.");
        close(file);
        close(client_socket);
        exit(1);
    }

    /*sending N-the file size to the server*/
    off_t file_size=lseek(file,0,SEEK_END);
    lseek(file,0,SEEK_SET);
    unsigned short N=(unsigned short) htons((unsigned short)file_size);
    unsigned char *N_pointer=(unsigned char *)&N;
    ssize_t bytes_sent=0;
    while(bytes_sent<sizeof(N)){
        ssize_t curr_sent=write(client_socket,N_pointer+bytes_sent,sizeof(N)-bytes_sent);
        if(curr_sent<=0){
            perror("An error has occured sending the size of the file to the server.");
            close(file);
            close(client_socket);
            exit(1);
        }
        bytes_sent+=curr_sent;

    }

    /*sending the file content to the server*/
    char buffer[1024];
    ssize_t bytes_read_from_file,bytes_sent_to_server;
    lseek(file,0,SEEK_SET);/*to make sure the the file buffer is at the start*/
    while((bytes_read_from_file=read(file,buffer,sizeof(buffer)))>0){
        ssize_t total_sent=0;
        while(total_sent<bytes_read_from_file){
            bytes_sent_to_server=write(client_socket,buffer+total_sent,bytes_read_from_file-total_sent);
            if(bytes_sent_to_server<=0){
                perror("An error has occured sending the content of the file to the server.");
                close(file);
                close(client_socket);
                exit(1); 
            }  
            total_sent+=bytes_sent_to_server; 
        }
    }

    /*receiving C from the server*/
    unsigned short printable_char_count;
    unsigned char *printable_char_count_pointer=(unsigned char *)&printable_char_count;
    ssize_t bytes_recieved=0;
    while(bytes_recieved<sizeof(printable_char_count)){
            ssize_t curr_recieved=read(client_socket,printable_char_count_pointer+bytes_recieved,sizeof(printable_char_count)-bytes_recieved);
            if(curr_recieved<=0){
                perror("An error has occured in recieving C from the server.");
                close(client_socket);
                exit(1);
            }
            bytes_recieved+=curr_recieved;
    
    }

    unsigned short C=(unsigned short ) ntohs(printable_char_count);
    printf("# of printable characters: %hu\n",C);


    close(file);
    close(client_socket);
    exit(0);
    
}