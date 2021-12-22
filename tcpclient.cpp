#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <thread>

using namespace std;
const int backLog = 3;
const int maxDataSize = 1460;

int clientSocketFd;
thread th_send, th_recv;
struct sockaddr_in serverSockAddressInfo;

void send_message(int clientSocketFd){
   int flags = 0;
   int dataSent = 0;
   char sendDataBuf[maxDataSize];
   string sendDataStr;
   //cout<<"You can type your message whenever you want!"<<endl;
   while(1){
      //receiving the message from the terminal
      memset(&sendDataBuf, 0, maxDataSize);
      cin.clear();
      getline(cin, sendDataStr);
            
      //sending the data from client to server
      dataSent = send(clientSocketFd, sendDataStr.c_str(), sendDataStr.length(), flags);
     
      //closing the client if the client says "bye"
      if(!strcmp(sendDataStr.c_str(), "bye"))
      {
         th_send.detach();
         th_recv.detach();
         close(clientSocketFd);
         exit(0);
      }
   }

}

void recv_message(int clientSocketFd){
   int flags = 0;
   int dataRecvd = 0;
   char rcvDataBuf[maxDataSize];
   string rcvDataStr;
   //cout<<"You will receive you messages as soon as they arrive!"<<endl;
   while(1){
      //receiving the message from the server
      memset(&rcvDataBuf, 0, maxDataSize);
      dataRecvd = recv(clientSocketFd, &rcvDataBuf, maxDataSize, flags);

      //printing the message on the terminal
      if(dataRecvd>0){
         rcvDataStr = rcvDataBuf;
         cout<<rcvDataStr.c_str()<<endl;
      }
   }
}

int main()
{

   uint16_t serverPort=3002;
   string serverIpAddr = "127.0.0.1";

   cout<<"Enter the ip address and port number of server"<<endl;
   cin>>serverIpAddr;
   cin>>serverPort;

   //creating the client socket
   clientSocketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(!clientSocketFd)
   {
      cout<<"Error creating socket"<<endl;
      exit(1);
   }

   serverSockAddressInfo.sin_family = AF_INET;
   serverSockAddressInfo.sin_port = htons(serverPort);
   inet_pton(AF_INET, serverIpAddr.c_str(), &(serverSockAddressInfo.sin_addr));

   memset(&(serverSockAddressInfo.sin_zero), '\0', 8);

   socklen_t sinSize = sizeof(struct sockaddr_in);
   int flags = 0;
   int dataRecvd = 0, dataSent = 0;
   struct sockaddr_in clientAddressInfo;
   char rcvDataBuf[maxDataSize], sendDataBuf[maxDataSize];
   string sendDataStr, rcvDataStr;

   //connecting to the server
   int ret = connect(clientSocketFd, (struct sockaddr *)&serverSockAddressInfo, sizeof(struct sockaddr));
   if (ret<0)
   {
      cout<<"Error with server connection "<<endl;
      close(clientSocketFd);
      exit(1);
   }

   //receiving the name from the terminal
   cout<<"Enter you name"<<endl;
   memset(&sendDataBuf, 0, maxDataSize);
   cin.clear();
   getline(cin, sendDataStr);

   //sending the name to the server
   dataSent = send(clientSocketFd, sendDataStr.c_str(), sendDataStr.length(), flags);


   //ceating threads for sending and receiving messages
   thread t1(send_message, clientSocketFd);
   thread t2(recv_message, clientSocketFd);

   th_send = move(t1);
   th_recv = move(t2);

   if(th_send.joinable()){
      th_send.join();
   }

   if(th_recv.joinable()){
      th_recv.join();
   }
}


