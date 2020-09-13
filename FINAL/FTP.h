//
//  FTP.hpp
//  FINAL
//
//  Created by Xavier on 6/9/19.
//  Copyright © 2019 Xavier. All rights reserved.
//

#ifndef FTP_h
#define FTP_h

#include <stdio.h>
#include <arpa/inet.h>      // inet_ntoa
#include <netinet/in.h>     // htonl, htons, inet_ntoa
#include <netinet/tcp.h>    // TCP_NODELAY
#include <sys/socket.h>     // socket, bind, listen, inet_ntoa
#include <sys/stat.h>
#include <sys/types.h>      // socket, bind
#include <sys/uio.h>        // writev
#include <sys/wait.h>       // for wait
#include <fcntl.h>          // fcntl
#include <netdb.h>          // gethostbyname
#include <sys/poll.h>
#include <signal.h>         // sigaction
#include <stdio.h>          // for NULL, perror
#include <string.h>
#include <unistd.h>         // read, write, close
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#define BUF_SIZE = 10000

using namespace std;

class FTP {
public:
    FTP();
    FTP(char*);
    ~FTP();
    
    void getServerResponse();
    void logIn();
    void checkLogIn();
    void close();
    bool pasv();
    void setTypeToI();
    void get(string filename);
    void put();
    bool fileExists(string& filename);
    bool loggedIn = false;
    


private:
    char buff[BUF_SIZE];

    
    char * url;
    int clientSd;
    int pasvSd;
    int clientFd;
    const int CTRL_PORT = 21; //Port for FTP communication
    //const int BUF_SIZE = 10000;
    struct hostent *host;
    //variables to store server response data when going into passive mode
    int a1, a2, a3, a4, p1, p2;
    int pid; //for fork()
};


#endif /* FTP_hpp */
