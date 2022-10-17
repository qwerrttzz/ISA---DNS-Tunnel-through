//Serverová aplikace bude naslouchat na implicitním portu pro DNS komunikaci. -Done
//Příchozí datové přenosy bude ukládat na disk ve formě souborů.              -not Done
//Komunikační protokol mezi klientem a serverem je implementační detail.      -not Done
//
//dns_receiver {BASE_HOST} {DST_FILEPATH}
//$ dns_receiver example.com ./data
//
//{BASE_HOST} slouží k nastavení bázové domény k příjmu dat
//{DST_FILEPATH} cesta pod kterou se budou všechny příchozí data/soubory ukládat (cesta specifikovaná klientem bude vytvořena pod tímto adresářem)

#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char const *argv[])
{    
    int family = PF_INET;
    int type = SOCK_STREAM;
    int protocol = 0;
    int socketId = socket(family, type, protocol);

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(8000);
    int serverAddressLen = sizeof(serverAddress);
    int statusBind = bind(socketId, (struct sockaddr *) &serverAddress, serverAddressLen);
    
    int queueLimit = 1000;
    int statusListen = listen(socketId, queueLimit);
    
    struct sockaddr_in clientAddress;
    unsigned int clientAddressLen = sizeof(clientAddress);
    int s = accept(socketId, (struct sockaddr *) &clientAddress, &clientAddressLen);

    char buffer[1024] = { 0 };
    int valread = recv(s, buffer, 1024, 0);
    //printf("%s\n", buffer);
    for (int i = 0; i < 1024; i++){
        printf("%d:",buffer[i]);
    }
    
    //close(s);
    shutdown(statusListen, SHUT_RDWR);
    return 0;
}
