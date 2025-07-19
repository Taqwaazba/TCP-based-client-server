
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <string>
#include <list>
#include <set>
#include <iostream>
#include <fstream>

#define N 5
#define MAX 1500
#define LISTENQ  1024

using namespace std;

typedef struct sockaddr SA;

typedef struct Account
{
    string id;
    string password;
} Account;

class Student
{
public:
    int key;
    string id;
    mutable int grade;

    Student(string id, int grade = 0) : id(id), grade(grade)
    {
        // reverse the id string, convert to integer, and save in key for ascending id ordering
        string reversedID = id;
        int len = reversedID.length();
        int n = len-1;
        int i = 0;
        while (i <= n)
        {
            swap(reversedID[i],reversedID[n]);
            n = n-1;
            i = i+1;
        }
        key = stoi(reversedID);
    }
    bool operator<(const Student other) const
    {
        return key < other.key;
    }
    bool operator==(const Student other) const
    {
        return key == other.key;
    }
};

list<Account> TAAccounts;
list<Account> StudentAccounts;
set<Student> grades;

pthread_cond_t request_available;
pthread_mutex_t queue_lock;
list<int> requestQueue;

void Init()
{
    // import fata from text files
    ifstream fStudents("students.txt");
    ifstream fTA("assistants.txt");
    
    string line;
    char cLine[100];

    while (getline(fStudents, line))
    {
        strcpy(cLine, line.c_str());
        Account account;
        account.id = string(strtok(cLine, ":"));
        account.password = string(strtok(nullptr, ""));
        StudentAccounts.push_back(account);
        grades.insert(Student(account.id));
    }

    while (getline(fTA, line))
    {
        strcpy(cLine, line.c_str());
        Account account;
        account.id = string(strtok(cLine, ":"));
        account.password = string(strtok(nullptr, ""));
        TAAccounts.push_back(account);
    }

    fStudents.close();
    fTA.close();

    // initialize thread handling
    pthread_cond_init(&request_available, NULL);
	pthread_mutex_init(&queue_lock, NULL);
}

string Login(char buff[MAX])
{
    string id = string(strtok(nullptr, " "));
    string password = string(strtok(nullptr, ""));

    int found = 0;
    bool studentMode;

    for (Account account : StudentAccounts)
    {
        if (id == account.id && password == account.password)
        {
            found = 1;
            studentMode = true;
            break;
        }
    }

    for (Account account : TAAccounts)
    {
        if (id == account.id && password == account.password)
        {
            found = 1;
            studentMode = false;
            break;
        }
    }

    if(found)
    {
        string response = "success ";
        if (studentMode) response += "Student Welcome Student " + string(id);
        else response += "TA Welcome TA " + string(id);
        return response;
    }
    else
    {
        return string("failure failure Wrong user information");
    }
}

string ReadGrade(char buff[MAX])
{
    string id = string(strtok(nullptr, ""));

    Student student(id);
    auto p = grades.find(student);
    return to_string((*p).grade);
}

string GradeList()
{
    bool empty = true;
    string response;
    for (auto student : grades)
    {
        empty = false;
        response += student.id + ": " + to_string(student.grade) + "\n";
    }
    if (!empty) response.pop_back(); // remove last "\n"
    return response;
}

string UpdateGrade(char buff[MAX])
{
    string id = string(strtok(nullptr, " "));
    int grade = atoi(strtok(nullptr, ""));

    Student student(id, grade);
    auto p = grades.find(student);
    if (p == grades.end()) // student not found in database, create a new student
    {
        grades.insert(student);
    }
    else // student found in database, update existing student
    {
        (*p).grade = grade;
    }
    
    return string();
}

void HandleRequest(int connfd)
{
    // read the message from client and copy it in buffer
    char buff[MAX];
    bzero(buff, sizeof(char) * MAX);
    if (read(connfd, buff, sizeof(char) * MAX) < 1)
    {
        close(connfd);
        return;
    }

    string command = strtok(buff, " ");
    string response;
    if (command == "Login") response = Login(buff);
    else if (command == "ReadGrade") response = ReadGrade(buff);
    else if (command == "GradeList") response = GradeList();
    else if (command == "UpdateGrade") response = UpdateGrade(buff);

    bzero(buff, sizeof(char) * MAX);
    strcpy(buff, response.c_str());
    
    // and send that buffer to client
    write(connfd, buff, sizeof(char) * MAX);

    // After chatting close the socket
    close(connfd);
}

void* WaitForRequests(void* args)
{
    while (1)
	{
        // get request from the global request queue
		pthread_mutex_lock(&queue_lock);
        while (requestQueue.size() == 0)
            pthread_cond_wait(&request_available, &queue_lock);
        int connfd = requestQueue.front();
        requestQueue.pop_front();
        pthread_mutex_unlock(&queue_lock);

		HandleRequest(connfd);
	}
	return NULL;
}

void ListenForConnections(int sockfd)
{
    while (1)
    {
        struct sockaddr_in clientaddr;
        int clientlen = sizeof(clientaddr);
        int connfd = accept(sockfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        if (connfd < 0) return;

        // queue request
        pthread_mutex_lock(&queue_lock);
        requestQueue.push_back(connfd);
        pthread_cond_signal(&request_available);
        pthread_mutex_unlock(&queue_lock);
    }
}

int main(int argc, char **argv)
{
    Init();
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);
    pthread_t threads[N];
    for (int i = 0; i < N; i++) pthread_create(&threads[i], NULL, WaitForRequests, NULL);

    /*    create server    */
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
    // Create a socket descriptor
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
    // Eliminates "Address already in use" error from bind.
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) < 0) return -1;
    // Listenfd will be an endpoint for all requests to port on any IP address for this host
    bzero((char*) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (SA*)&serveraddr, sizeof(serveraddr)) < 0) return -1;
    // Make it a listening socket ready to accept connection requests
    if (listen(listenfd, LISTENQ) < 0) return -1;
   
    ListenForConnections(listenfd);
}