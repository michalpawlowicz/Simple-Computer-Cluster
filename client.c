#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/un.h>
#include <signal.h>

#define MAX_NAME_SIZE 256
#define MESSAGE_SIZE 256

int PORT;
int option;
char name[MAX_NAME_SIZE];
char server_ip[15];
char server_path[MAX_NAME_SIZE];
int  master_socket;

struct sockaddr_un un_address;
struct sockaddr_in in_address;

void printf_usage(void){
     printf("Name, Option - 1, Socket path\n");
     printf("Name, Option - 2, IP, PORT\n");
}

struct message{
     int type;
     int A;
     int B;
     char data[MESSAGE_SIZE - 12];
};

void handler(int signum){
     struct message msg;
     msg.type = 8;
     strcpy(msg.data, name);
     if(send(master_socket, &msg, sizeof(msg), 0) == -1 ){
	  perror("send name error");
     }
     printf("Closing ... \n");
     exit(0);
}

int main(int argc, char** argv){

     signal(SIGINT, handler);
     
     if(argc < 4){
	  printf_usage();
	  return 0;
     } else {
	  strcpy(name, argv[1]);
	  if(atoi(argv[2]) == 1){
	       option = 0;
	  } else option = 1;
     }
     
     struct sockaddr_in address;
     socklen_t addr_size;

     if(option == 0){
	  // unix socket
	  strcpy(server_path, argv[3]);
     } else if(option == 1) {
	  // network
	  if(argc < 5){
	       printf("!!!");
	       printf_usage();
	       return 0;
	  }
	  strcpy(server_ip, argv[3]);
	  PORT = (int)strtol(argv[4], NULL, 10);
     } else {
	  printf_usage();
	  return 0;
     }

     if(option == 1){
	  // newtwork
	  master_socket = socket(AF_INET, SOCK_STREAM, 0);
     } else {
	  // local, unix socket
	  master_socket = socket(AF_UNIX, SOCK_STREAM, 0);
     }

     if(master_socket == -1){
	  perror("socket error");
	  return 0;
     }

     if(option == 0){
	  // unix socket
	  un_address.sun_family = AF_UNIX;
	  strcpy(un_address.sun_path, server_path);
	  // cos jeszcze
     } else {
	  in_address.sin_family = AF_INET;
	  in_address.sin_port = htons((uint16_t) PORT);
	  inet_pton(AF_INET, server_ip, &in_address.sin_addr.s_addr); // !!! co to
	  // cos jeszcze
     }



     if(option == 0){
	  // unix socket
	  if(connect(master_socket, (struct sockaddr*) &un_address, sizeof(un_address)) == -1){
	       perror("un connect error");
	       exit(0);
	  }
     } else {
	  // network
	  if(connect(master_socket, (struct sockaddr *) &in_address, sizeof(in_address)) == -1){
	       perror("connect error");
	       exit(0);
	  }
     }

     // send my name

     struct message msg;
     msg.type = 1;
     strcpy(msg.data, name);
     
     if(send(master_socket, &msg, sizeof(msg), 0) == -1 ){
	  perror("send name error");
	  return 0;
     }              


     fd_set set;
     int max_sd;
     int valread;
     int activity;     
     // receive

    
     while(1){
	  FD_ZERO(&set);
	  FD_SET(master_socket, &set);
	  max_sd = master_socket;

	  activity = select( max_sd + 1 , &set , NULL , NULL , NULL);

	  if ((activity < 0) && (errno!=EINTR)){
	       printf("select error");
	  }
	  
	  if (FD_ISSET(master_socket, &set)){
	       valread = read(master_socket, (void*) &msg, 1024);
	       if(valread > 0){
		    if(((struct message)msg).type == 0){
			 printf("This name already exists!\n");
			 exit(0);
		    } else if(((struct message)msg).type == 2){
			 printf("Calculating %d + %d\n", ((struct message)msg).A, ((struct message)msg).B);
			 int res = ((struct message)msg).A + ((struct message)msg).B;
			 msg.type = 6;
			 msg.A = res;
			 if(send(master_socket, &msg, sizeof(msg), 0) == -1 ){
			      perror("send name error");
			      return 0;
			 }   
 		    } else if(((struct message)msg).type == 3){
			 printf("Calculating %d - %d\n", ((struct message)msg).A, ((struct message)msg).B);
			 int res = ((struct message)msg).A - ((struct message)msg).B;
			 msg.type = 6;
			 msg.A = res;
			 if(send(master_socket, &msg, sizeof(msg), 0) == -1 ){
			      perror("send name error");
			      return 0;
			 }   
		    } else if(((struct message)msg).type == 4){
			 printf("Calculating %d * %d\n", ((struct message)msg).A, ((struct message)msg).B);
			 int res = ((struct message)msg).A * ((struct message)msg).B;
			 msg.type = 6;
			 msg.A = res;
			 if(send(master_socket, &msg, sizeof(msg), 0) == -1 ){
			      perror("send name error");
			      return 0;
			 }   
		    } else if(((struct message)msg).type == 5){
			 printf("Calculating %d / %d\n", ((struct message)msg).A, ((struct message)msg).B);
			 int res = ((struct message)msg).A / ((struct message)msg).B;
			 msg.type = 6;
			 msg.A = res;
			 if(send(master_socket, &msg, sizeof(msg), 0) == -1 ){
			      perror("send name error");
			      return 0;
			 }   
		    } else if(((struct message)msg).type == 7){
			 printf("My ID: %d\n", (((struct message)msg).A));  
		    }  else if(((struct message)msg).type == 9){
			 printf("Ping ... \n");
		    }
	       }
	  }

     }
     
     close(master_socket);
     
     return 0;
}
