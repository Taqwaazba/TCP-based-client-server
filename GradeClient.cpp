#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <string.h>
#include <iostream>

#define MAX 1500

using namespace std;

typedef struct sockaddr SA;

bool loggedIn = false;
bool studentMode = true;
string loggedID;

void SendToServer(int sockfd, char buff[MAX])
{
    // send to server
    write(sockfd, buff, sizeof(char) * MAX);

    // server response
    bzero(buff, sizeof(char) * MAX);
    read(sockfd, buff, sizeof(char) * MAX);
}

void Login(int sockfd, string input)
{
    char *id = strtok(nullptr, " ");
    char *password = strtok(nullptr, "");
    if (id == nullptr || password == nullptr || string(password).find(' ') != string::npos) 
    {
        cout << "Wrong Input" << endl;
        return;
    }
    if (loggedIn)
    {
        cout << "There is a user already logged in from this client." << endl;
        return;
    }

    // in case of success, should return a message in the format: "success <TA/Student> <Message>"
    // in case of failure, should return a message in the format: "failure failure <Message>"
    char buff[MAX];
    bzero(buff, sizeof(char) * MAX);
    strcpy(buff, input.c_str());
    SendToServer(sockfd, buff);
    string result = string(strtok(buff, " "));
    string mode = string(strtok(nullptr, " "));
    string msg = string(strtok(nullptr, ""));
    if (result == "success")
    {
        cout << msg << endl;
        studentMode = mode == "TA" ? false : true;
        loggedID = id;
        loggedIn = true;
    }
    else
    {
        cout << msg << endl;
    }
}

void ReadGrade(int sockfd, string input)
{
    char *id = strtok(nullptr, "");
    if (id == nullptr)
    {
        if (!studentMode)
        {
            cout << "Missing argument" << endl;
            return;
        }
        string command = "ReadGrade " + loggedID;
        char buff[MAX];
        bzero(buff, sizeof(char) * MAX);
        strcpy(buff, command.c_str());
        // in case of success, should return a message in the format: "<Grade>"
        SendToServer(sockfd, buff);
        cout << buff << endl;
    }
    else if (id != nullptr && string(id).find(' ') == string::npos)
    {
        if (studentMode)
        {
            cout << "Action not allowed" << endl;
            return;
        }
        char buff[MAX];
        bzero(buff, sizeof(char) * MAX);
        strcpy(buff, input.c_str());
        // in case of success, should return a message in the format: "<Grade>"
        SendToServer(sockfd, buff);
        cout << buff << endl;
    }
    else
    {
        cout << "Wrong Input" << endl;
        return;
    }
}

void GradeList(int sockfd, string input)
{
    if (strtok(nullptr, "") != nullptr)
    {
        cout << "Wrong Input" << endl;
        return;
    }
    if (studentMode)
    {
        cout << "Action not allowed" << endl;
        return;
    }
    char buff[MAX];
    bzero(buff, sizeof(char) * MAX);
    strcpy(buff, input.c_str());
    // in case of success, should return a message in the format: "<id>: <Grade>\n<id>: <Grade>\n ... <id>: <Grade>"
    SendToServer(sockfd, buff);
    cout << buff << endl;
}

void UpdateGrade(int sockfd, string input)
{
    char *id = strtok(nullptr, " ");
    char *grade = strtok(nullptr, "");
    if (id == nullptr || grade == nullptr || string(grade).find(' ') != string::npos) 
    {
        cout << "Wrong Input" << endl;
        return;
    }
    if (studentMode)
    {
        cout << "Action not allowed" << endl;
        return;
    }

    // in case of success, should not return a response message
    char buff[MAX];
    bzero(buff, sizeof(char) * MAX);
    strcpy(buff, input.c_str());
    SendToServer(sockfd, buff);
}

void Logout()
{
    if (strtok(nullptr, "") != nullptr)
    {
        cout << "Wrong Input" << endl;
        return;
    }
    
    cout << "Good bye " << loggedID << endl;
    loggedIn = false;
}

int main(int argc, char *argv[])
{
    char *hostname;
    int port;
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <host> <port> \n", argv[0]);
        exit(1);
    }
    hostname = argv[1];
    port = atoi(argv[2]);

    while (1)
    {
        // get input from user
        cout << "> ";
        string input;
        getline(cin, input);
        // trim string
        size_t end = input.find_last_not_of(" ");
        input = (end == std::string::npos) ? "" : input.substr(0, end + 1);
        size_t start = input.find_first_not_of(" ");
        input = (start == std::string::npos) ? "" : input.substr(start);
        size_t doubleSpace = input.find("  ");
        while (doubleSpace != string::npos)
        {
            input.erase(doubleSpace, 1);
            doubleSpace = input.find("  ");
        }
        // tokenize string
        char cmd[MAX];
        bzero(cmd, sizeof(char) * MAX);
        strcpy(cmd, input.c_str());
        char *cCommand = strtok(cmd, " ");
        if (cCommand == nullptr) continue;
        string command(cCommand);

        if (command == "Exit") return 0;

        // connect to the server
        int clientfd;
        struct hostent *hp;
        struct sockaddr_in serveraddr;

        if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            return -1; // check errno for cause of error

        if ((hp = gethostbyname(hostname)) == nullptr) // Fill in the server's IP address and port
            return -2; // check h_errno for cause of error
        bzero((char *) &serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        bcopy((char *)hp->h_addr, 
            (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
        serveraddr.sin_port = htons(port);

        if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0) // Establish a connection with the server
            return -1;


        // process command
        bool needsLogin = (command == "ReadGrade" || command == "GradeList" || command == "UpdateGrade" || command == "Logout");
        if (command == "Login") Login(clientfd, input);
        else if (needsLogin && !loggedIn) cout << "Not logged in" << endl;
        else if (command == "ReadGrade") ReadGrade(clientfd, input);
        else if (command == "GradeList") GradeList(clientfd, input);
        else if (command == "UpdateGrade") UpdateGrade(clientfd, input);
        else if (command == "Logout") Logout();
        else cout << "Wrong Input" << endl;

        // close the socket
        close(clientfd);
    }
}