//
//  main.cpp
//  FINAL
//
//  Created by Xavier on 6/9/19.
//  Copyright © 2019 Xavier. All rights reserved.
//

#include "FTP.h"

using namespace std;

FTP* ftp;


//Utility function for splitting strings by spaces
vector<string> split(string str) {
    vector<string> splitArr;
    char * token = strtok((char *) str.c_str(), " ");
    while(token != NULL) {
        splitArr.push_back(token);
        token = strtok(NULL, " ");
    }
    return splitArr;
}

int main(int argc, char *argv[]) {
    //Check for valid argument count
    if(argc != 2) {
        cerr << "Insfficient Argument!" << endl;
        cerr << endl;
        exit(0);
    }
    
    char* url = argv[1];
    //Command input loop
    while(true) {
        //Prompt user for command
        cout << "ftp> ";
        string command; //string containing command
        getline(cin, command);
        if(command.empty()) {
            continue;
        }
        //vector to store each part of command
        vector<string> splitCommand = split(command);
        string commandType = splitCommand[0];
        
        if(commandType == "open")
        {
            if(!ftp->loggedIn)
            {
                logIn();
            }
            else
            {
                cout << "You are already logged in." << endl;
            }
        }
        else if(commandType == "ls")
        {
            checkLogIn();
            if(!loggedIn)
            {
                continue;
            }
            if(!pasv()) {
                continue;
            }
            
            string directoryList;
            if((pid = fork()) < 0)
            {
                cerr << "Fork failed." << endl;
                continue;
            }
            else if (pid > 0)
            {
                //parent process
                
                char listCmd[BUF_SIZE];
                strcpy(listCmd, "LIST");
                strcat(listCmd, "\r\n");
                write(clientSd, (char *) &listCmd, strlen(listCmd));
                sleep(1);
                
            } else {
                bzero(buff, sizeof(buff));
                //list of directories and files sent through passive connection
                while(read(pasvSd, (char *)&buff, sizeof(buff)) > 0)
                {
                    directoryList.append(buff);
                }
                
                getServerResponse();
                cout << buff;
                close(pasvSd);
                cout << directoryList << endl;
                getServerResponse();
                cout << buff;
                return 0;
                close(pasvSd);
            }
        }
        
        else if(commandType == "cd")
        {
            checkLogIn();
            if(splitCommand.size() != 2)
            {
                cout << "Usage: cd subdir" << endl;
                continue;
            }
            if(!loggedIn)
            {
                continue;
            }
            char cdCommand[BUF_SIZE];
            strcpy(cdCommand, "CWD ");
            strcat(cdCommand, splitCommand[1].c_str());
            strcat(cdCommand, "\r\n");
            
            //send command to server
            write(clientSd, (char *)&cdCommand, strlen(cdCommand));
            getServerResponse();
            cout << buff;
            continue;
            
        }
        else if( commandType == "get")
        {
            checkLogIn();
            if(!loggedIn)
            {
                continue;
            }
            if(splitCommand.size() != 2)
            {
                cout << "Usage: requires 1 argument, the file to get." << endl;
                continue;
            }
            cout << "testing";
            get(splitCommand[1]);
        }
        else if(commandType == "put")
        {
            checkLogIn();
            if(!loggedIn)
            {
                continue;
            }
            if(splitCommand.size() != 1)
            {
                cout << "Usage: just type put" << endl;
                continue;
            }
            put();
        }
        else if(commandType == "close")
        {
            //Close connection to server but keep ftp client running
            //Check if connection is open first
            checkLogIn();
            if(loggedIn)
            {
                close();
                shutdown(clientSd, SHUT_WR); //close socket
                
                loggedIn = false;
            }
            
            continue;
        }
        else if(commandType == "quit")
        {
            if(!loggedIn)
            {
                close();
                shutdown(clientSd, SHUT_WR); //close socket
                loggedIn = false;
            }
            else {
                close(); //close connection with ftp server
            }
            
            break;   //Exit program
            //close()
        }
        else
        {
            cout << "Invalid command" << endl;
        }
    }
    
    return 0;
}
