//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//              This class processes an FTP transactions.
// 
//****************************************************************************

#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>
#include <random>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include <sys/stat.h> 
#include <iostream>
#include <dirent.h>

#include "common.h"

#include "FTPServer.h"
#include "ClientConnection.h"

ClientConnection::ClientConnection(int s) {
  control_socket = s;
  // Check the Linux man pages to know what fdopen does.
  fd = fdopen(s, "a+");
  if (fd == NULL) {
	  std::cout << "Connection closed" << std::endl;
	  fclose(fd);
	  close(control_socket);
	  ok = false;
	  return ;
  }
  ok = true;
  data_socket = -1;
  parar = false;
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
}

void ParsePortCommand(std::string& raw, std::string& address, int& port) {
  int address_counter{0};
  //std::cout << "RAW: "<< raw << std::endl;
  // IP
  while (true) {
    char ch{raw[0]};
    if (ch == ',') {
      if (address_counter == 3) break;
      address.push_back('.');
      ++address_counter;
    } else {
      address.push_back(ch);
    }
    raw.erase(0, 1);
  }
  //std::cout << "IP: " << address << std::endl;
  // PORT
  std::string port_temp{};
  int mult{0};
  for (long i{(long)raw.length() - 1}; i >= 0; --i) {
    if (raw[i] == ',') {
      if (mult == 0) port += std::stoi(port_temp);
      else port += std::stoi(port_temp) * mult * 256;
      ++mult;
      port_temp.clear();
    } else {
      port_temp.insert(0, 1, raw[i]);
    }
  }
  //std::cout << "PORT: " << port << std::endl;
}

int connect_TCP(char* address, uint16_t port) {
   // Implement your code to define a socket here
  struct sockaddr_in sin;
  struct hostent* hent;
  int s;
  
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  
  hent = gethostbyname(address);
  if (hent) {
    memcpy(&sin.sin_addr, hent->h_addr, hent->h_length);
  } else if ((sin.sin_addr.s_addr = inet_addr(address)) == INADDR_NONE) {
    errexit("No se puede resolver el nombre \"%d\"\n", address);
  }
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    errexit("No puedo crear el socket: %s\n", strerror(errno));
  }
  if (connect(s, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
    errexit("No se puede conectar con %d: %s\n", address, strerror(errno));
  }
  return s; // You must return the socket descriptor.
}

void ClientConnection::stop() {
  close(data_socket);
  close(control_socket);
  parar = true;
}

#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.
void ClientConnection::WaitForRequests() {
  if (!ok) return;
  fprintf(fd, "220 Service ready\n");
  while (!parar) {
    fscanf(fd, "%s", command);
    if (COMMAND("USER")) {
	    fscanf(fd, "%s", arg);
	    fprintf(fd, "331 User name ok, need password\n");
    } else if (COMMAND("PWD")) {
      char wd[MAX_BUFF];
      getcwd(wd, MAX_BUFF);
      fprintf(fd, "257 \"%s\" is the current directory.\n", wd);
    } else if (COMMAND("CWD")) {
      fscanf(fd, "%s", arg);
      if (chdir(arg) == -1) fprintf(fd, "550 Failed to change directory to %s: No such file or directory.\n", arg);
      else fprintf(fd, "250 Directory successfully changed to %s\n", arg);
    } else if (COMMAND("PASS")) {
      fscanf(fd, "%s", arg);
      if (strcmp(arg,"1234") == 0) {
        fprintf(fd, "230 User logged in\n");
      } else { 
        fprintf(fd, "530 Not logged in.\n");
        parar = true;
      }
    } else if (COMMAND("PORT")) {
	    // To be implemented by students
      fscanf(fd, "%s", arg);
      std::string raw{arg}, address{};
      int port{};
      ParsePortCommand(raw, address, port);
      data_socket = connect_TCP((char*)address.c_str(), port);
      fprintf(fd, "200 OK\n");
    } else if (COMMAND("SIZE")) {
      fscanf(fd, "%s", arg);
      struct stat file;
      if (stat(arg, &file) != -1) fprintf(fd, "%s %ld %s", "213", file.st_size, "\n");
      else fprintf(fd, "%s", "550 File does not exist.\n");
    } else if (COMMAND("PASV")) {
	    // To be implemented by students
      srand(time(NULL));
      int port = (rand() % 48000) + 2000; // Random port between 2000 and 50000;
      int p1{port / 256}, p2{port % 256};
      int listening_socket = define_socket_TCP(port);
      listen(listening_socket, 1);
      fprintf(fd, "227 Entering Passive Mode (127,0,0,1,%d,%d).\n", p1, p2);
      fflush(fd);
      struct sockaddr_in fsin;
      socklen_t alen = sizeof(fsin);
      data_socket = accept(listening_socket, (struct sockaddr*) &fsin, &alen); // Waits the client to connect to the port sent before (30000).
      close(listening_socket);
      shutdown(listening_socket, SHUT_RDWR);
    } else if (COMMAND("STOR")) {
	    // To be implemented by students
      fscanf(fd, "%s", arg);
      int file = open(arg, O_CREAT | O_RDWR | O_TRUNC, 0666);
      fprintf(fd, "150 Ready to get data.\n");
      fflush(fd);
      int bytes_read{};
      while ((bytes_read = recv(data_socket, &data_buff, MAX_BUFF, 0)) > 0) {
        write(file, data_buff, bytes_read);
      }
      fprintf(fd, "226 Closing data connnection\n");
      close(data_socket);
      close(file);
      data_socket = -1;
    } else if (COMMAND("RETR")) {
	    // To be implemented by students
      fscanf(fd, "%s", arg);
      int file = open(arg, O_RDONLY);
      if (file == -1) {
        fprintf(fd, "%s", "550 File does not exist.\n");
        continue;
      }
      fprintf(fd, "150 File status okey; about to open data connection\n");
      fflush(fd);
      size_t bytes_read{};
      while ((bytes_read = read(file, &data_buff, MAX_BUFF)) > 0) {
        send(data_socket, &data_buff, bytes_read, 0);
      }
      fprintf(fd, "226 Closing data connnection\n");
      close(data_socket);
      close(file);
      data_socket = -1;
    } else if (COMMAND("LIST")) {
	    // To be implemented by students
      fprintf(fd, "150 File status okey; about to open data connection\n");
      fflush(fd);
      int file = fileno(popen("ls -l", "r"));
      size_t bytes_read{};
      while ((bytes_read = read(file, &data_buff, MAX_BUFF)) > 0) {
        send(data_socket, &data_buff, bytes_read, 0);
      }
      fprintf(fd, "226 Closing data connnection\n");
      close(data_socket);
      close(file);
      data_socket = -1;
    } else if (COMMAND("SYST")) {
      fprintf(fd, "215 UNIX Type: L8.\n");   
    } else if (COMMAND("TYPE")) {
	    fscanf(fd, "%s", arg);
	    fprintf(fd, "200 OK\n");   
    } else if (COMMAND("QUIT")) {
      fprintf(fd, "221 Service closing control connection. Logged out if appropriate.\n");
      close(data_socket);	
      parar = true;
      break;
    } else {
	    fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
	    printf("Comando : %s %s\n", command, arg);
	    printf("Error interno del servidor\n");
    }
  }
  fclose(fd);
  return;
};
