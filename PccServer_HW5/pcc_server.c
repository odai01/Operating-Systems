#define _POSIX_C_SOURCE 200809
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>

/*in the assignment file it is said that the printable chars are byte chars with value
ranges between 32 and 126, which means there are 95 possible values for such a
printable char */
#define PRINTABLE_CHAR_RANGE 95
/*so our data structure pcc-total will be an array of size PRINTABLE_CHAR_RANGE=95
such that pcc_total[i] represents the total count for the byte char with the value: i+32*/
unsigned short pcc_total[PRINTABLE_CHAR_RANGE];

int accept_more_clients=1,new_socket=-1;

void print_pcc_total(){
    int i;
    for(i=0;i<PRINTABLE_CHAR_RANGE;i++){
        printf("char '%c' : %hu times\n",i+32,pcc_total[i]);
    }
}

void sigint_handler(int sig){
    if( new_socket<0 ){
        /*i also used new_socket to be negative when not processing a client*/
        print_pcc_total();
        exit(0);
    }
    accept_more_clients=0;
}



int main(int argc, char *argv[]){
    if(argc!=2){
        fprintf(stderr,"Incorrect number of passed arguments.\n");
        exit(1);
    }
    unsigned short temp_pcc_total[PRINTABLE_CHAR_RANGE];
    unsigned short port=atoi(argv[1]);

    memset(pcc_total,0,sizeof(pcc_total));
    memset(temp_pcc_total,0,sizeof(temp_pcc_total));
    
    struct sigaction new_sig;
    sigemptyset(&new_sig.sa_mask);
    new_sig.sa_handler=sigint_handler;
    new_sig.sa_flags=SA_RESTART;
    if(sigaction(SIGINT,&new_sig,NULL)){
        perror("error changing the handling of SIGINT.\n");
        exit(1);
    }

    /*signal(SIGINT,sigint_handler);*/
    int server_socket;
    server_socket=socket(AF_INET,SOCK_STREAM,0);
    if(server_socket<0){
        perror("An error has occured creating a tcp scocket.\n");
        exit(1);
    }

    int opt_value=1;
    if(setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&opt_value,sizeof(opt_value))){
        perror("An error has occured in set socket option.\n");
        close(server_socket);
        exit(1);
    }

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr)); 
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_port=htons(port);

    if(bind(server_socket,(struct sockaddr *) &addr,sizeof(addr))<0){
        perror("An error has occured in bind.");
        exit(1); 
    }

    if(listen(server_socket,10)<0){
        perror("An error has occured in listen.");
        exit(1); 
    }
    int addr_len=sizeof(addr);
    int continue_to_next_while;


    while(accept_more_clients==1){
        new_socket=-1;
        continue_to_next_while=0;
        for(int i=0;i<PRINTABLE_CHAR_RANGE;i++){
            temp_pcc_total[i]=0;
        }
        
        new_socket=accept(server_socket,(struct sockaddr *) &addr,(socklen_t*) &addr_len);
        if(new_socket<0){
            perror("An error has occured accepting the socket.");
            exit(1);
        }
        /*getting N from the client*/
        unsigned short N,file_size;
        unsigned char *file_size_pointer=(unsigned char *)&file_size;
        ssize_t bytes_recieved=0;
        while(bytes_recieved<sizeof(file_size)){
            ssize_t curr_recieved=read(new_socket,file_size_pointer+bytes_recieved,sizeof(file_size)-bytes_recieved);
            if(curr_recieved==0 || errno==ETIMEDOUT || errno ==ECONNRESET || errno==EPIPE){
                
                continue_to_next_while=1;
                close(new_socket);
                new_socket=-1;
                break;
            }
            
            if(curr_recieved<0){
                
                perror("An error has occured in reading the file size from the client.");
                close(new_socket);
                exit(1);
            }
            
            bytes_recieved+=curr_recieved;
        }
        
        if(continue_to_next_while==1){
            continue;
        }

        N=ntohs(file_size);
        
        /*reading the file content from the client*/
        char buffer[N];
        ssize_t total_read=0;
        while(total_read<N){
            ssize_t bytes_read=read(new_socket,buffer+total_read,N-total_read);
            if(bytes_read==0 ||  errno==ETIMEDOUT || errno ==ECONNRESET || errno==EPIPE){
                perror("An error has occured in reading the file content from the client-but an allowed error.");
                close(new_socket);
                new_socket=-1;
                continue_to_next_while=1;
                break;
            }
            if(bytes_read<0){
                
                perror("An error has occured in reading the file content from the client.\n");
                close(new_socket);
                exit(1);
            }
    
            total_read+=bytes_read;
        }

        if(continue_to_next_while==1){
            continue;
        }

        unsigned short C=0;
        for(int i=0;i<N;i++){
            if(buffer[i]>=32 && buffer[i]<=126){
                C+=1;
                temp_pcc_total[buffer[i]-32]+=1;
            }
        }
        /*sending to the client C:printable_char_count*/
        unsigned short printable_char_count=htons(C);
        unsigned char *printable_char_count_pointer=(unsigned char *)&printable_char_count;
        ssize_t bytes_sent=0;
        while(bytes_sent<sizeof(printable_char_count)){
            ssize_t curr_sent=write(new_socket,printable_char_count_pointer+bytes_sent,sizeof(printable_char_count)-bytes_sent);
            if(curr_sent==0 ||  errno==ETIMEDOUT || errno ==ECONNRESET || errno==EPIPE){
                
                perror("An error has occured in reading the file content from the client-but an allowed error.");
                close(new_socket);
                new_socket=-1;
                continue_to_next_while=1;
                break;
            }
            if(curr_sent<0){
                
                perror("An error has occured in sending C to the client.");
                close(new_socket);
                exit(1);
                
            }
            bytes_sent+=curr_sent;
    
        }
        if(continue_to_next_while==1){
            continue;
        }
        for(int i=0;i<PRINTABLE_CHAR_RANGE;i++){
            pcc_total[i]+=temp_pcc_total[i];
            temp_pcc_total[i]=0;
        }
        close(new_socket);
        new_socket=-1;
    
    }

    close(server_socket);
    print_pcc_total();
    exit(0);
}