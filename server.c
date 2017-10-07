#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/un.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS_NUMBER 10
#define MESSAGE_SIZE 256
#define MAX_NAME_SIZE 256


#define True 1
#define False 0

int port_num;
char* socket_path;
int c_socket[MAX_CLIENTS_NUMBER];

char** names_ar;

int cli_num = 0;

int un_socket;
int in_socket;

struct sockaddr_un un_address;
struct sockaddr_in in_address;


pthread_t monitor_th;
pthread_t ping_th;


int answer[MAX_CLIENTS_NUMBER];

int contains(char* s){
     for(int i=0; i<MAX_CLIENTS_NUMBER; i++){
	  if(strcmp(s, names_ar[i]) == 0){
	       return True;
	  }
     }
     return False;
}

void rem_cli(char* name){
     for(int i=0; i<MAX_CLIENTS_NUMBER; i++){
	  if(strcmp(name, names_ar[i]) == 0){
	       strcpy(names_ar[i], "\0");
	  }
     }
}

void add_cli(char* name){
     for(int i=0; i<MAX_CLIENTS_NUMBER; i++){
	  if(strcmp(names_ar[i], "\0") == 0){
	       strcpy(names_ar[i], name);
	  }
     } 
}

void rem_soc(int soc){
     for(int i=0; i<MAX_CLIENTS_NUMBER; i++){
	  if(c_socket[i] == soc){
	       c_socket[i] = 0;
	  }
     } 
}

void print_usage(void){
     printf("TCP port number\nSocket path\n");
}

int type(char* buf){
     return (unsigned int) buf;
}

struct message{
     int type;
     int A;
     int B;
     char data[MESSAGE_SIZE - 12];
};

void* monitor(void* args){

     pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

     unlink(socket_path);


     for(int i=0; i<MAX_CLIENTS_NUMBER; i++){
	  c_socket[i] = 0;
     }

     
     if((un_socket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1 ){
	  perror("unix socket error");
	  exit(0);
     }

     int option = 1;
     setsockopt(un_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
     
     if((in_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1){
	  perror("network socket error");
	  exit(0);
     }

     setsockopt(in_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
     
     
     un_address.sun_family = AF_UNIX;
     strcpy(un_address.sun_path, socket_path);

     in_address.sin_family = AF_INET;
     in_address.sin_port = htons((uint16_t) port_num);
     in_address.sin_addr.s_addr = htonl(INADDR_ANY);
     

     
     if(bind(un_socket, (struct sockaddr*) &un_address, sizeof(un_address)) != 0){
	  perror("un bind error");
	  exit(0);
     }

     if(bind(in_socket, (struct sockaddr*) &in_address, sizeof(in_address)) != 0){
	  perror("in bind error");
	  exit(0);
     }
     
     if(listen(in_socket, MAX_CLIENTS_NUMBER) == -1){
	  perror("in listen error");
	  exit(0);
     }

     if(listen(un_socket, MAX_CLIENTS_NUMBER) == -1){
	  perror("un listen error");
	  exit(0);
     }


     
     int activity, max_socket = 0;
     


     struct message msg;
     
     int valread;

     fd_set set;

     int debug = 0;
     
     while(1){

	  FD_ZERO(&set); // clear set
	  FD_SET(in_socket, &set); // add in socket
	  FD_SET(un_socket, &set); // add un socket
	  
	  max_socket = (in_socket > un_socket) ? in_socket : un_socket; // searching for max socket

	  for(int i=0; i<MAX_CLIENTS_NUMBER; i++){ // add all clients do set
	       if(c_socket[i] > 0)
		    FD_SET(c_socket[i], &set);
	       
	       if(max_socket < c_socket[i])
		    max_socket = c_socket[i];
	  }

	  activity = select(max_socket + 1, &set, NULL, NULL, NULL);

	  if ((activity < 0) && (errno!=EINTR)){
	       printf("select error");
	  }

	  if(FD_ISSET(in_socket, &set)){ // activity on in_socket
	       int new_socket;
	       int addrlen = sizeof(in_address);
	       if((new_socket = accept(in_socket, (struct sockaddr *) &in_address, &addrlen)) < 0){
		    perror("in_socket accept error");
		    exit(0);
	       }

	       for(int i=0; i<MAX_CLIENTS_NUMBER; i++){
		    if(c_socket[i] == 0){
			 c_socket[i] = new_socket;
			 break;
		    }
	       }
	       printf("[%d] New connection: %d\n", in_socket, new_socket);
	  }

	  if(FD_ISSET(un_socket, &set)){
	       int new_socket;
	       int addrlen = sizeof(un_address);
	       if((new_socket = accept(un_socket, (struct sockaddr *) &un_address, (socklen_t*) &addrlen)) < 0){
		    perror("un accept error");
		    exit(0);
	       }

	       for(int i=0; i<MAX_CLIENTS_NUMBER; i++){
		    if(c_socket[i] == 0){
			 c_socket[i] = new_socket;
			 break;
		    }
	       }
	       printf("[%d] New connection: %d\n", un_socket, new_socket);
	  }

	  
	  for(int i=0; i<MAX_CLIENTS_NUMBER; i++){
	       int sd = c_socket[i];
	       if(FD_ISSET(sd, &set)){
		    // take message
       
		    valread = read(sd, (void*) &msg, 1024);

		    if(valread > 0){
			 if(((struct message)msg).type == 1){
			      if(contains(((struct message)msg).data) == 1){
				   // sent to client that this name already exist
				   msg.type = 0;
				   if(send(sd, (void*) &msg, sizeof(msg), 0) == -1 ){
					perror("send name error");
					exit(0);
				   }
				   rem_soc(sd);
			      } else {
				   msg.type = 7;
				   msg.A = sd;
				   if(send(sd, (void*) &msg, sizeof(msg), 0) == -1 ){
					perror("send name error");
					exit(0);
				   }				   
				   add_cli(((struct message)msg).data);
			      }
			 } else if(((struct message)msg).type == 6){
			      printf("Result from [%d]: %d\n", sd, (((struct message)msg).A));
			 } else if(((struct message)msg).type == 8){
			      rem_soc(sd);
			      rem_cli(((struct message)msg).data);
			 }		 
		    }
		    
	       }
	  }
	  
	  
	  
     }
     return (void*) 0;
}




void* pinging(void* args){

     pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

     struct message msg;
     msg.type = 9; // ping
     
     while(1){
	  for(int i=0; i<MAX_CLIENTS_NUMBER; i++){
	       
	       if(c_socket[i] > 0 && send(c_socket[i], (void*) &msg, sizeof(msg), 0) == -1 ){
		    perror("send ping error");
		    exit(0);
	       }
	       
	       usleep(50000);
	  }
     }
     
     return (void*) 0;
}

int rand_socket(){
     int sd = 0;
     int index;
     while(sd == 0){
	  index = rand() % MAX_CLIENTS_NUMBER;
	  if(c_socket[index] > 0){
	       sd = c_socket[index];
	  }
     }
     return sd;
}


int main(int argc, char** argv){

     srand(time(NULL));
     
     if(argc != 3){
	  print_usage();
	  return -1;
     } else {
	  port_num = atoi(argv[1]);
	  socket_path = malloc(sizeof(char) * strlen(argv[2]));
	  strcpy(socket_path, argv[2]);
     }


     names_ar = malloc(sizeof(char*) * MAX_CLIENTS_NUMBER);
     for(int i=0; i<MAX_CLIENTS_NUMBER; i++){
	  names_ar[i] = malloc(sizeof(char) *  MAX_NAME_SIZE);
	  //memset(names_ar[i], 0, sizeof(names_ar[i]));
	  strcpy(names_ar[i], "\0"); 
     }


     if(pthread_create(&monitor_th, NULL, &monitor, NULL) != 0){
	  perror(NULL);
	  return 0;  
     }

     if(pthread_create(&ping_th, NULL, &pinging, NULL) != 0){
	  perror(NULL);
	  return 0;  
     }

     for(int i=0; i<MAX_CLIENTS_NUMBER; i++){
	  answer[i] = False;
     }

     
     int opt = 1;
     int a, b;
     int sd;
     struct message msg;
     
     while(opt != 0){
	  printf("Quit - 0\na + b - 1\na - b - 2\na * b - 3\na / b - 4\n");
	  scanf("%d", &opt);
	  
	  switch (opt){
	  case 1: // +
	       printf("Specify A and B: ");
	       scanf("%d %d", &a, &b);
	       msg.A = a;
	       msg.B = b;
	       msg.type = 2;
	       sd = rand_socket();
	       if(send(sd, (void*) &msg, sizeof(msg), 0) == -1 ){
		    perror("send name error");
		    exit(0);
	       }
	       break;
	  case 2: // -
	       printf("Specify A and B: ");
	       scanf("%d %d", &a, &b);
	       msg.A = a;
	       msg.B = b;
	       msg.type = 3;
	       sd = rand_socket();
	       if(send(sd, (void*) &msg, sizeof(msg), 0) == -1 ){
		    perror("send name error");
		    exit(0);
	       }
	       break;
	  case 3: // *
	       printf("Specify A and B: ");
	       scanf("%d %d", &a, &b);
	       msg.A = a;
	       msg.B = b;
	       msg.type = 4;
	       sd = rand_socket();
	       if(send(sd, (void*) &msg, sizeof(msg), 0) == -1 ){
		    perror("send name error");
		    exit(0);
	       }
	       break;
	  case 4: // /
	       printf("Specify A and B: ");
	       scanf("%d %d", &a, &b);
	       msg.A = a;
	       msg.B = b;
	       msg.type = 5;
	       sd = rand_socket();
	       if(send(sd, (void*) &msg, sizeof(msg), 0) == -1 ){
		    perror("send name error");
		    exit(0);
	       }
	       break;
	  }
     }

     pthread_cancel(monitor_th);
     pthread_cancel(ping_th);
     
     free(socket_path);
     return 0;

}
