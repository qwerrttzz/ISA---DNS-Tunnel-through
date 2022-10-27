//Serverová aplikace bude naslouchat na implicitním portu pro DNS komunikaci. -Done
//Příchozí datové přenosy bude ukládat na disk ve formě souborů.              -Done
//Komunikační protokol mezi klientem a serverem je implementační detail.      
//
//dns_receiver {BASE_HOST} {DST_FILEPATH}
//$ dns_receiver example.com ./data
//
//{BASE_HOST} slouží k nastavení bázové domény k příjmu dat                   -not Done
//{DST_FILEPATH} cesta pod kterou se budou všechny příchozí data/soubory ukládat (cesta specifikovaná klientem bude vytvořena pod tímto adresářem)

#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct arguments {
    char* BASE_HOST;
    char* DST_FILEPATH;
} Arguments;

int parseArguments(int argc, char** argv, Arguments* myArguments){
    if (argc == 3){
        myArguments->BASE_HOST= argv[1];
        myArguments->DST_FILEPATH = argv[2];
        return 0;
    }
    else{
        return 1;
    }
    
}

int main(int argc, char *argv[])
{   
    //parsovanie argumentov 
    Arguments myArguments = {};
    if(parseArguments(argc, argv, &myArguments) != 0){
        fprintf(stderr,"wrong arguments");
    }
    printf("BASE_HOST:%s\n",myArguments.BASE_HOST);
    printf("DST_FILEPATH:%s\n",myArguments.DST_FILEPATH);
    fflush(stdout);

    int family = PF_INET;
    int type = SOCK_STREAM;
    int protocol = IPPROTO_TCP;
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

    FILE* file = fopen(myArguments.DST_FILEPATH, "w");
    char buffer[1024] = { 0 };
    int size;
    while((size = recv(s, buffer, 1024, 0)) != 0){
        //printf("%s\n", buffer);
        for (int i = 0; i < 1024; i++){
            printf("%hX:",buffer[i]);
            fputc(buffer[i],file);
            if ((i%40) == 0)
            {
                fputc('\n',file);
            }
            
        }
        printf("\n\n####################################################################################################\n");
    }
    
    
    
    
    close(socketId);
    shutdown(statusListen, SHUT_RDWR);
    return 0;
}
