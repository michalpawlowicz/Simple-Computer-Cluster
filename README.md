# Simple-Computer-Cluster

Simple computer clouster with stream pipes                                      
Central sever which listens on local and network socket with connected clients.                 
                                                                                
## Description                                                                   
Calculations are written in server's terminal and contain basic operations.     
Next, instructions are send to on of unused clients, where are calculated and send     
back to the server which shows them on terminal.

## Usage
* Use cmake to compile.

* Arguments                                                                     
  * Server, port number TCP or UDP and unix socket path.                                    
  * Client, client name, server connection type (network or local unix sockets) and server  
address (address IPv4 and port number or local unix socket path)                
                                                                                
* Client can be unloged from cluster by using Ctrl + C shotcut.              
                                                                   
