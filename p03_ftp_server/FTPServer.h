#if !defined FTPServer_H
#define FTPServer_H

#include <list>

#include "ClientConnection.h"

class FTPServer {
public:
  FTPServer(int port = 21);
  void run();
  void stop();
  ~FTPServer() { stop(); }
private:
  int port;
  int msock;
  std::list<ClientConnection*> connection_list;
};

int define_socket_TCP(int port);

#endif