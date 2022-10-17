
//Klientská aplikace bude odesílat data souboru/ze STDIN.                                       -Done
//V případě, že program načítá data ze STDIN je činnost aplikace ukončena přijetím EOF.         -Done
//Program bude možné spustit a ovládat pomocí následujícího předpisu:                           -Done
//
//dns_sender [-u UPSTREAM_DNS_IP] {BASE_HOST} {DST_FILEPATH} [SRC_FILEPATH]
//
//$ dns_sender -u 127.0.0.1 example.com data.txt ./data.txt
//$ echo "abc" | dns_sender -u 127.0.0.1 example.com data.txt
//
//-u slouží k vynucení vzdáleného DNS serveru
//    pokud není specifikováno, program využije výchozí DNS server nastavený v systému          -Done
//
//{BASE_HOST} slouží k nastavení bázové domény všech přenosů                                    -not Done
//    tzn. dotazy budou odesílány na adresy *.{BASE_HOST}, tedy např. edcba.32.1.example.com
//{DST_FILEPATH} cesta pod kterou se data uloží na serveru                                      -not Done
//[SRC_FILEPATH] cesta k souboru který bude odesílán                                            -Done
//    pokud není myspecifikovano pak program čte data ze STDIN

#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h> 

#define CHUNK_SIZE 10
int findSystemDnsServer(char* dnsIP){
    printf("resolv.conf output\n");
    
    FILE* file;
    file = fopen("/etc/resolv.conf", "r");
    char ch;
    char word[100];
    int nextWordImportant = 0;
    while ((ch = fgetc(file)) != EOF){
        if (ch == ' ' || ch == '\n' || ch == EOF){
            //printf(":%s:",word);
            if (nextWordImportant == 1){
                nextWordImportant = 2;
                break;
            }
            else if (strcmp(word, "nameserver") == 0){
                strcpy(word, "");
                nextWordImportant = 1;
            }
            else{
                strcpy(word, "");
            }
        }
        else{
            strncat(word, &ch, 1);   
        }
        
        
    }
    
    if (nextWordImportant == 2){
        strcpy(dnsIP, word);
        printf("################################################################%s\n",dnsIP);
        return 0;
    }
    else{
        return 1;
    }
}

typedef struct arguments {
    short int dnsIpExists;
    char* UPSTREAM_DNS_IP;
    char* BASE_HOST;
    char* DST_FILEPATH;
    short int srcFilePathExists;
    char* SRC_FILEPATH;
} Arguments;

int argument_parser(char** argv, int argc, Arguments* myArguments){
    int opt;
    while((opt = getopt(argc, argv, "u:")) != -1)
    {
        switch(opt) 
        {
            case 'u': 
                myArguments->dnsIpExists = 1;
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
            myArguments->srcFilePathExists = 1;
            myArguments->SRC_FILEPATH = argv[optind];
            break;
        default:
            break;
        }
        printf("extra arguments: %d %s\n",optind, argv[optind]); 
        argument_number++;
    }

    return 0;
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
    Arguments myArguments;
    if(argument_parser(argv,argc,&myArguments) != 0){
        fprintf(stderr, "wrong arguments");
    }
    printf("BASE_HOST: %s\n", myArguments.BASE_HOST);
    printf("DST_FILEPATH: %s\n", myArguments.DST_FILEPATH);
    printf("SRC_FILEPATH: %s\n", myArguments.SRC_FILEPATH);
    printf("UPSTREAM_DNS_IP: %s\n", myArguments.UPSTREAM_DNS_IP);
    
    //////////////////////////////////////////////////////////////////
    int family = PF_INET;
    int type = SOCK_STREAM;
    int protocol = 0;
    int queueLimit = 1000;
    int socketId = socket(family, type, protocol);
    
    //nastavenie adresy podla -u
    char dnsServerAdress[100];
    if (myArguments.dnsIpExists == 1){
        //TODO chack ci nie je addressa moc velka 
        strcpy(dnsServerAdress, myArguments.UPSTREAM_DNS_IP);
    }
    else{
        findSystemDnsServer(dnsServerAdress);
    }
    
    //nameserver 127.0.0.53
    struct sockaddr_in foreignAddress;
    foreignAddress.sin_family = AF_INET;
    foreignAddress.sin_addr.s_addr = inet_addr(dnsServerAdress);
    foreignAddress.sin_port = htons(8000);                      //TODO nastav na 53
    int foreignAddressSize = sizeof(foreignAddress);
    int status = connect(socketId, (struct sockaddr *) &foreignAddress, foreignAddressSize);
    
    
    char data[CHUNK_SIZE+1] = {0};
    struct dnsPacket packet;
    char sendpacket[sizeof(struct dnsPacket)];
    char character;
    int counter = 0;

    //vybranie medzy suborom a stdin
    FILE* file;
    if(myArguments.srcFilePathExists == 1){
        fflush(stdout);
        file = fopen(myArguments.SRC_FILEPATH, "r");
    }
    else{
        file = stdin;
    }
        
    while ((character = fgetc(file)) != EOF){
        strncat(data, &character, 1);
        if (counter == CHUNK_SIZE){
            
            packet = createDnsPacket(data);
            memcpy(sendpacket, &packet, sizeof(packet));
            send(socketId, sendpacket, sizeof(sendpacket), 0);
            
            strcpy(data,"\0");
            counter = 0;
        }
        
       
        counter++;
    }
    printf("filepath:%s\n", myArguments.SRC_FILEPATH);
    
    
    char buffer[1024] = { 0 };
    int valread = recv(socketId, buffer, 1024, 0);
    printf("\n\nanswer of server\n");
    for (int i = 0; i < 1024; i++){
            printf("%d:",buffer[i]);
    }
        
    return 0;
}




