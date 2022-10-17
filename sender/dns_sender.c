
//Klientská aplikace bude odesílat data souboru/ze STDIN.                                       -not Done
//V případě, že program načítá data ze STDIN je činnost aplikace ukončena přijetím EOF.         -not Done
//Program bude možné spustit a ovládat pomocí následujícího předpisu:                           -not Done
//
//dns_sender [-u UPSTREAM_DNS_IP] {BASE_HOST} {DST_FILEPATH} [SRC_FILEPATH]
//
//$ dns_sender -u 127.0.0.1 example.com data.txt ./data.txt
//$ echo "abc" | dns_sender -u 127.0.0.1 example.com data.txt
//
//-u slouží k vynucení vzdáleného DNS serveru
//    pokud není specifikováno, program využije výchozí DNS server nastavený v systému          -not Done
//
//{BASE_HOST} slouží k nastavení bázové domény všech přenosů
//    tzn. dotazy budou odesílány na adresy *.{BASE_HOST}, tedy např. edcba.32.1.example.com
//{DST_FILEPATH} cesta pod kterou se data uloží na serveru
//[SRC_FILEPATH] cesta k souboru který bude odesílán
//    pokud není myspecifikovano pak program čte data ze STDIN

#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h> 

struct arguments{
    short int dnsIpExists;
    char* UPSTREAM_DNS_IP;
    char* BASE_HOST;
    char* DST_FILEPATH;
    char* SRC_FILEPATH;
};

struct arguments* argument_parser(char** argv, int argc){
    int opt;
    struct arguments* myArguments;
    while((opt = getopt(argc, argv, "u:")) != -1) 
    { 
        switch(opt) 
        {
            case 'u': 
                myArguments->dnsIpExists=0;
                myArguments->UPSTREAM_DNS_IP = optarg;
                printf("%s \n", optarg);
                break; 
        }
    } 
    
    int argument_number=1;
    for(; optind < argc; optind++){
        switch (argument_number)
        {
        case 1:
            myArguments->BASE_HOST = argv[optind];
            break;
        case 2:
            myArguments->DST_FILEPATH = argv[optind];
            break;
        case 3:
            myArguments->SRC_FILEPATH = argv[optind];
            break;
        default:
            break;
        }
        printf("extra arguments: %d %s\n",optind, argv[optind]); 
        argument_number++;
    }
    return myArguments;
}

struct dnsPacket{
    char ident;
    char ident2;
    unsigned short int numberOfQuestion;
    unsigned short int numberOfAnswer;
    unsigned short int numberOfAuthority;
    unsigned short int numberOfRRs;
    char dirtyData[500];
};

struct dnsPacket createDnsPacket(char* data){
    struct dnsPacket packet;
    packet.ident = 0;
    packet.ident2 = 0;
    packet.numberOfQuestion = 0;
    packet.numberOfAnswer = 0;
    packet.numberOfAuthority = 0;
    packet.numberOfRRs = 0;
    strcpy(packet.dirtyData, data);

    return packet;
}

int main(int argc, char *argv[])
{
    struct arguments* myArguments = argument_parser(argv,argc);
    printf("BASE_HOST: %s\n", myArguments->BASE_HOST);
    printf("DST_FILEPATH: %s\n", myArguments->DST_FILEPATH);
    printf("SRC_FILEPATH: %s\n", myArguments->SRC_FILEPATH);
    printf("UPSTREAM_DNS_IP: %s\n", myArguments->UPSTREAM_DNS_IP);
    
    //////////////////////////////////////////////////////////////////

    int family = PF_INET;
    int type = SOCK_STREAM;
    int protocol = 0;
    int queueLimit = 1000;
    int socketId = socket(family, type, protocol);

    //nameserver 127.0.0.53
    struct sockaddr_in foreignAddress;
    foreignAddress.sin_family = AF_INET;
    foreignAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    foreignAddress.sin_port = htons(8000);
    int foreignAddressSize = sizeof(foreignAddress);
    int status = connect(socketId, (struct sockaddr *) &foreignAddress, foreignAddressSize);
    
    char sendpacket[sizeof(struct dnsPacket)];
    char string[]="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    struct dnsPacket packet = createDnsPacket(string);
    memcpy(sendpacket, &packet, sizeof(packet));

    char character;
    while ((character = fgetc(stdin)) != EOF)
    {
        printf("%c \n",character);
    }
    
    send(socketId, sendpacket, sizeof(sendpacket), 0);
    
    char buffer[1024] = { 0 };
    char cbuffer[1000] = { 0 };
    int valread = recv(socketId, buffer, 1024, 0);
    
        
    return 0;
}




