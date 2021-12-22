#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

using namespace std;
const int backLog = 3;
const int maxDataSize = 1460;

int main()
{

   uint16_t serverPort=3002;
   string serverIpAddr = "127.0.0.1";

   int n=4;
   cout<<"Enter the number of members in the chatroom"<<endl;
   cin>> n;

   int clientsockets[n];
   for(int i=0; i<n; i++){
      clientsockets[i] = 0;
   }

   string clientnames[n];
   for(int i=0; i<n; i++){
      clientnames[i] = "";
   }

   cout<<"Enter the ip address and port number to listen the connections for"<<endl;
   cin>>serverIpAddr;
   cin>>serverPort;

   //creating server socket
   int serverSocketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(!serverSocketFd)
   {
      cout<<"Error creating socket"<<endl;
      exit(1);
   }

   struct sockaddr_in serverSockAddressInfo;
   serverSockAddressInfo.sin_family = AF_INET;
   serverSockAddressInfo.sin_port = htons(serverPort);

   inet_pton(AF_INET, serverIpAddr.c_str(), &(serverSockAddressInfo.sin_addr));
   memset(&(serverSockAddressInfo.sin_zero), '\0', 8);
   printf("Server listening on IP %x:PORT %d \n",serverSockAddressInfo.sin_addr.s_addr, serverPort);

   //binding server socket
   int ret = bind(serverSocketFd, (struct sockaddr *)&serverSockAddressInfo, sizeof(struct sockaddr)); 
   if(ret<0)
   {
      cout<<"Error binding socket"<<endl;
      close(serverSocketFd);
      exit(1);
   }

   //listening on the server socket
   ret = listen(serverSocketFd, backLog);
   if(!serverSocketFd)
   {
      cout<<"Error listening socket"<<endl;
      close(serverSocketFd);
      exit(1);
   }

   socklen_t sinSize = sizeof(struct sockaddr_in);
   int flags = 0;
   int dataRecvd = 0, dataSent = 0;
   struct sockaddr_in clientAddressInfo;
   char rcvDataBuf[maxDataSize], sendDataBuf[maxDataSize];
   string sendDataStr, recvDataStr;

   fd_set socketfds;

   while(1)
   {
      //adding server socket to the fd_set for monitoring
      FD_ZERO(&socketfds);
      FD_SET(serverSocketFd, &socketfds);

      //adding all the active clients to the fd_set for monitoring
      int highestfd = serverSocketFd;
      for(int i=0; i<n; i++){
         if(clientsockets[i]>0){
            FD_SET(clientsockets[i], &socketfds);
         }
         if(clientsockets[i] > highestfd){
            highestfd = clientsockets[i];
         }
      }

      //blocking for any of the file descriptors to be ready
      int readready = select(highestfd+1, &socketfds, nullptr, nullptr, nullptr);
      

      //checking if a new connection has arrived
      if FD_ISSET(serverSocketFd, &socketfds){

         //accepting a connection from a new client
         memset(&clientAddressInfo, 0, sizeof(struct sockaddr_in));
         memset(&rcvDataBuf, 0, maxDataSize);
         int newClientFd = accept(serverSocketFd, (struct sockaddr *)&clientAddressInfo, &sinSize);
         if (!newClientFd)
         {
            cout<<"Error with new client connection "<<endl;
            exit(1);
         }

         //receiving the name of the newly joined client
         memset(&rcvDataBuf, 0, maxDataSize);
         dataRecvd = recv(newClientFd, &rcvDataBuf, maxDataSize, flags);
         recvDataStr = rcvDataBuf;
         string joinmsg = recvDataStr.c_str() + string(" has joined the chat");
         string clientname = recvDataStr.c_str();

         //sending welcome message to the newly joined client
         string greeting = "Welcome to the chat server!";
         int datasent = send(newClientFd, greeting.c_str(), greeting.size() + 1, 0);

         //adding the newly joined client to our list of active clients
         for(int i=0; i<n; i++){
            if(clientsockets[i] == 0){
               clientsockets[i] = newClientFd;
               clientnames[i] = clientname;
               cout << clientname << " has been added to the chat"<<endl;
               break;
            }
         }

         //notifying all the active clients about the arrival of the new client
         for(int i=0; i<n; i++){
            int outClientFd = clientsockets[i];
            if((outClientFd!=newClientFd) && (outClientFd!=serverSocketFd) && (outClientFd!=0)){
               dataSent = send(outClientFd, joinmsg.c_str(), joinmsg.size() + 1, 0);
               cout<<clientname<<" joining message broadcasted to "<<clientnames[i]<<endl;
            }
         }
      }
      for(int i=0; i<n; i++){
         int newClientFd = clientsockets[i];

         //checking for messages from the clients
         if FD_ISSET(newClientFd, &socketfds){

            //receiving the message from the client
            memset(&rcvDataBuf, 0, maxDataSize);
            dataRecvd = recv(newClientFd, &rcvDataBuf, maxDataSize, flags);
            recvDataStr = rcvDataBuf;
            cout<<"Data received from "<<clientnames[i]<<": "<<recvDataStr.c_str()<<endl;

            //constructing the message that needs to be broadcasted
            string outmsg = clientnames[i] + ">" + recvDataStr.c_str();
            if(!strcmp(recvDataStr.c_str(), "bye"))
            {
               clientsockets[i] = 0;
               outmsg = clientnames[i] + " has left the chat.";
               clientnames[i] = "";
            }

            //broadcasting the received message to all the clients
            for(int j=0; j<n; j++){
               int outClientFd = clientsockets[j];
               if((outClientFd!=newClientFd) && (outClientFd!=serverSocketFd) && (outClientFd!=0)){
                  dataSent = send(outClientFd, outmsg.c_str(), outmsg.size() + 1, 0);
                  cout<<"Broadcast done to "<<clientnames[j]<<endl;
               }
            }
         }
      }
   }

   cout<<"All done closing server socket now"<<endl;
   close(serverSocketFd);
   
}
