#if !defined ClientConnection_H
#define ClientConnection_H

#include <pthread.h>

#include <cstdio>
#include <cstdint>

constexpr int MAX_BUFF = 1000;

class ClientConnection {
  public:
    ClientConnection(int s);
    void WaitForRequests();
    void stop();
    ~ClientConnection();
  private:  
    bool ok;  // This variable is a flag that avois that the
  	        // server listens if initialization errors occured.
     
    
    FILE *fd;	 // C file descriptor. We use it to buffer the
  		     // control connection of the socket and it allows to
  		     // manage it as a C file using fprintf, fscanf, etc.
      
    char command[MAX_BUFF];  // Buffer for saving the command.
    char arg[MAX_BUFF];      // Buffer for saving the arguments. 
      
    char data_buff[MAX_BUFF]; // Buffer for saving data.
  
    int data_socket;         // Data socket descriptor;
    int control_socket;      // Control socket descriptor;
    bool parar;
};

#endif
