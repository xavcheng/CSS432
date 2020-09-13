//
//  FTP.cpp
//  FINAL
//
//  Created by Xavier on 6/9/19.
//  Copyright © 2019 Xavier. All rights reserved.
//

#include "FTP.h"
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

using namespace std;

FTP::FTP() {
    clientSd = -1;
    pasvSd = -1;
}

FTP::FTP() {

}

void FTP::checkLogIn()
{
    if(!loggedIn)
    {
        cout << "Login with \"open\" command" << endl;
    }
}

//Reads response from server into char[] buffer
void FTP::getServerResponse() {
    //Clear the buffer
    bzero(buff, sizeof(buff));
    //Read data from server into buffer
    read(clientSd, buff, sizeof(buff));
    string error = "421";
    if(strstr(buff, error.c_str())) {
        cerr << "There was a problem with the server." << endl;
        close();
        exit(0);
    }
}

//Socket polling function after user enters correct password
int FTP::pollSocket()
{
    struct pollfd ufds;
    ufds.fd = clientSd;               // a socket descriptor to exmaine for read
    ufds.events = POLLIN;             // check if this sd is ready to read
    ufds.revents = 0;                 // simply zero-initialized
    return poll( &ufds, 1, 1000 );       // poll this socket for 1000msec (=1sec)
}

//Handles logging in user to ftp server
void FTP::logIn() {
    clientSd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSd < 0) {
        cerr << "Error creating socket to server" << endl;
        exit(0);
    }
    
    host = gethostbyname(url);
    struct sockaddr_in addr;
    bzero((char*)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr =
    inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    addr.sin_port = htons(CTRL_PORT);
    
    //Attempt to connect socket to ftp server
    if(connect(clientSd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "Cannot connect to ftp server." << endl;
        exit(0);
    }
    
    //Print welcome message from server
    getServerResponse();
    cout << buff;
    
    //Log in user to server
    
    //Request username
    cout << "Name(" << url << ":" << getenv("USER") << "): ";
    char name[BUF_SIZE];
    cin >> name;
    char userCmd[BUF_SIZE];
    strcpy(userCmd, "USER ");
    strcat(userCmd, name);
    strcat(userCmd, "\r\n"); //CRLF
    
    //Send username to server and recieve the acknowledgement
    write(clientSd, (char*)&userCmd, strlen(userCmd));
    getServerResponse();
    cout << buff;
    
    //Enter password
    while(true)
    {
        cout << "Password: ";
        char password[BUF_SIZE];
        cin >> password;
        char passwordCmd[BUF_SIZE];
        strcpy(passwordCmd, "PASS ");
        strcat(passwordCmd, password);
        strcat(passwordCmd, "\r\n"); //CRLF
        
        //Send password to server
        write(clientSd, (char *)&passwordCmd, strlen(passwordCmd));
        //print response
        getServerResponse();
        cout << buff;
        
        string error = "501";
        
        //If it is a valid password break out of loop
        if(!strstr(buff, error.c_str()))
        {
            char syst[BUF_SIZE];
            strcpy(syst, "SYST ");
            strcat(syst,password);
            strcat(syst, "\r\n");
            
            write(clientSd, (char *)&syst, strlen(syst));
            getServerResponse();
            cout << buff;
            
            break;
        }
        
    }
    //Clear buffer of newline chars so getline() can work properly
    cin.ignore();
    
    //Poll the server
    while(pollSocket() == 1) {
        loggedIn = true;
        
        getServerResponse();
        cout << buff;
    }
    if(pollSocket() == -1) {
        cout << "Can't poll socket" << endl;
    }
    
    //loggedIn = true;
}

void FTP::close() {
    char close[BUF_SIZE];
    strcpy(close, "QUIT");
    strcat(close, "\r\n");
    write(clientSd, (char*)&close, strlen(close));
    getServerResponse();
    cout << buff;
    //shutdown(clientSd, SHUT_WR); //close socket
    
}

//Passive mode connection with FTP server returns false if there is an error
bool FTP::pasv() {
    char passiveCmd[BUF_SIZE];
    strcpy(passiveCmd, "PASV");
    strcat(passiveCmd, "\r\n");
    write(clientSd, (char*)&passiveCmd, strlen(passiveCmd));
    getServerResponse();
    cout << buff;
    
    sscanf(buff, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
           &a1, &a2, &a3, &a4, &p1, &p2);
    
    int serverPortNum = (p1 * 256) + p2;
    struct sockaddr_in serverAddress;
    bzero((char*) &serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPortNum);
    serverAddress.sin_addr.s_addr =
    inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    
    pasvSd = socket(AF_INET, SOCK_STREAM, 0);
    if(pasvSd < 0) {
        cerr << "Error creating passive socket" << endl;
        return false;
    }
    
    //passive connection to ftp server
    if(connect(pasvSd, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Can't establish passive connection" << endl;
        return false;
    }
    return true;
}

//Sets data sending type to binary for file retrieval
void FTP::setTypeToI() {
    char typeCmd[BUF_SIZE];
    strcpy(typeCmd, "TYPE I");
    strcat(typeCmd, "\r\n");
    write(clientSd, (char*) &typeCmd, strlen(typeCmd));
    getServerResponse();
    cout << buff;
}

//Get command implementation
void FTP::get(string filename) {
    //Set type to 'I' (IMAGE aka binary)
    setTypeToI();
    //establish pasv connection
    if(!pasv()) {
        return;
    }
    bool error = false;
    //int file;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    if((pid = fork()) < 0)
    {
        cerr << "Get: fork failed" << endl;
        return;
    }
    else if(pid > 0)
    {
        //parent process
        char retrieveCmd[BUF_SIZE];
        strcpy(retrieveCmd, "RETR ");
        strcat(retrieveCmd, filename.c_str());
        strcat(retrieveCmd, "\r\n");
        write(clientSd, (char*) &retrieveCmd, strlen(retrieveCmd));
        sleep(1); //wait for child
    }
    else
    {
        //Check for error
        while(pollSocket() == 1)
        { //Check if there is a response
            getServerResponse();
            cout << buff;
            
            string errorCode = "550";
            if(strstr(buff, errorCode.c_str()))
            {
                error = true;
                exit(0);
                return;
            }
        }
        //child process
        int file = open(filename.c_str(), O_WRONLY | O_CREAT, mode);
        bzero(buff, sizeof(buff));
        
        while(true)
        {
            int n = read(pasvSd, buff, sizeof(buff));
            if(n == 0 || n == -1) {
                break;
            }
            write(file, buff, n);
        }
        close(file);
        close(pasvSd);
        exit(0);
    }
    close(pasvSd);
}

bool fileExists(string &filename) {
    ifstream ifile(filename.c_str());
    //return ifile;
    return (access(filename.c_str(), F_OK) != -1);
}

void FTP::put() {
    //get filename and newFilename from user
    
    string filename;
    string filename2;
    
    cout << "(local file) ";
    getline(cin, filename);
    cout << "(remote file) ";
    getline(cin, filename2);
    
    if(!fileExists(filename)) {
        cout << filename  << " No such file." << endl;
        return;
    }
    
    //Set type to 'I' (IMAGE aka binary)
    setTypeToI();
    //establish pasv connection
    if(!pasv()) {
        return;
    }
    
    if((pid = fork()) < 0) {
        cerr << "put: fork failed" << endl;
        return;
    }
    else if(pid > 0)
    {
        //parent process
        char putCmd[BUF_SIZE];
        strcpy(putCmd, "STOR ");
        strcat(putCmd, filename2.c_str());
        strcat(putCmd, "\r\n");
        write(clientSd, (char *) &putCmd, strlen(putCmd));
        wait(NULL); //wait for child
    }
    else
    {
        //child process
        int file = open(filename.c_str(), O_RDONLY);
        while(true) {
            int n = read(file, buff, sizeof(buff));
            if(n == 0){
                break;
            }
            write(pasvSd, buff, n);
        }
        close(file);
        exit(0);
    }
    close(pasvSd);
    
    getServerResponse();
    cout << buff;
}
