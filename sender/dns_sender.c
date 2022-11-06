
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

#include <stdlib.h> //TODO do i need that?
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h> 

#define CHUNK_SIZE 5

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length) {

    *output_length = (4 * ((input_length + 2) / 3) + 1);//for appending .

    char *encoded_data = malloc(*output_length);
    if (encoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    char ending = '.';
    return encoded_data;//strncat(encoded_data, &ending, 1);
}

int stringToInt8(char* string, int8_t** array, int strlen){
    *array = (int8_t*) malloc(strlen);
    
    for (int i = 0; i < strlen; i++)
    {
        (*array)[i] = (int8_t) string[i];
        printf("char:%c int:%d\n",(char)string[i],(*array)[i]);
    }
    return 0;
}

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
                printf("wtf %s",argv[optind]);
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

struct PacketLenght{
    uint16_t len;
};

struct DnsPacketHeader{
    uint16_t ident;
    uint16_t flags;
    uint16_t numberOfQuestion;
    uint16_t numberOfAnswer;
    uint16_t numberOfAuthority;
    uint16_t numberOfRRs;
};

struct DnsQuestion{
    //uint8_t* name;
    //uint8_t* whatever;
    uint16_t qtype;
    uint16_t qclass;
};

struct DnsPacket{
    struct PacketLenght packetLen;
    struct DnsPacketHeader header;
    struct DnsQuestion question;
};

short int createDnsFlags(){
    return 256;
}

int prepareDomainName(char* domainName){
    int domainLen = strlen(domainName);
    char* tmp = malloc(sizeof(char)* (domainLen+1));
    
    int wordLen = 0;
    for (int i = domainLen-1; i >= 0; i--){
        if (domainName[i] == '.'){
            tmp[i+1] = (char) wordLen;
            wordLen=0;
        }
        else{
            tmp[i+1] = domainName[i];
            wordLen++;
        }

        if(i == 0){
            tmp[i] = (char) wordLen;
        }
    }
    printf("new domainName %s\n",tmp);
    
    strcpy(domainName,tmp);
    free(tmp);
    return 0;
}

struct DnsPacket createDnsPacket(char* data){ 
    struct PacketLenght packetLen;
    packetLen.len = htons(100);

    struct DnsPacketHeader packetHeader;
    packetHeader.ident = htons(0x5678);//1
    packetHeader.flags = htons(createDnsFlags());
    packetHeader.numberOfQuestion = htons(1);//1
    packetHeader.numberOfAnswer = htons(0);
    packetHeader.numberOfAuthority = htons(0);
    packetHeader.numberOfRRs = htons(0);

    
    struct DnsQuestion packetQuestion;
    char hostname[] = "\03www\03sme\02sk";
    int8_t* int_hostname;
    //stringToInt8(hostname, &int_hostname, strlen(hostname));
    packetQuestion.qtype = htons(1);//2
    packetQuestion.qclass = htons(1);//1
    
    struct DnsPacket packet;
    packet.packetLen = packetLen;
    packet.header = packetHeader;
    packet.question = packetQuestion;
    
    return packet;
}

int8_t* prepare_dns_packet(char* domainName, char* dataToHide, long unsigned int* encodedSize){
    struct DnsPacket packet = createDnsPacket(dataToHide);
    printf("header flags: %hX",packet.header.ident);

    //sizeof(char)*(strlen(dataToHide)+(strlen(dataToHide)%3));
    char* data = "helloo";
    char* encodedStr = base64_encode(dataToHide, strlen(dataToHide), encodedSize);                                      //TODO neviem preco+ 1 CHUNK_SIZE+1
    printf("string:%s\nencodedStr:$%s$\n domainName:$%s$",dataToHide, encodedStr, domainName);
    
    int8_t* super_buffer = (int8_t*)malloc(sizeof(int8_t)*1000);
    int16_t packet_size =  htons(16 + strlen(domainName)+*encodedSize+1);//16 + domain_size
    int position=0;
    memcpy(&super_buffer[position], &packet_size, 1 * sizeof( short int ));
    memcpy(&super_buffer[position+=2], &packet.header.ident, 1 * sizeof( short int ));
    memcpy(&super_buffer[position+=2], &packet.header.flags, 1 * sizeof( short int ));
    memcpy(&super_buffer[position+=2], &packet.header.numberOfQuestion, 1 * sizeof( short int ));
    memcpy(&super_buffer[position+=2], &packet.header.numberOfAnswer, 1 * sizeof( short unsigned int )); 
    memcpy(&super_buffer[position+=2], &packet.header.numberOfAuthority, 1 * sizeof( short int ));
    memcpy(&super_buffer[position+=2], &packet.header.numberOfRRs, 1 * sizeof( short int ));
    
    char sizee = (char)(*encodedSize-1);                                                                        //TODO neviem preco -1
    
    memcpy(&super_buffer[position+=2], &sizee, 1 * sizeof( char ));
    memcpy(&super_buffer[position+=1], encodedStr, *encodedSize); 
    memcpy(&super_buffer[position+=*encodedSize-1], domainName, (strlen(domainName)+1) * sizeof( char ));       //TODO neviem preco -1
    
    memcpy(&super_buffer[position+=(strlen(domainName)+1)], &packet.question.qtype, 1 * sizeof( short int ));
    memcpy(&super_buffer[position+2], &packet.question.qclass, 1 * sizeof( short int ));
    
    return super_buffer;
}

void printBuffer(int8_t* buffer,int len){
    for(int i = 0; i <= len; i++){
        printf("\n %hX %c",buffer[i],buffer[i]);
        fflush(stdout);
    }
}

void printArguments(Arguments myArguments){
    printf("BASE_HOST: %s\n", myArguments.BASE_HOST);
    printf("DST_FILEPATH: %s\n", myArguments.DST_FILEPATH);
    printf("SRC_FILEPATH: %s\n", myArguments.SRC_FILEPATH);
    printf("UPSTREAM_DNS_IP: %s\n", myArguments.UPSTREAM_DNS_IP);
    fflush(stdout);
}

int prepareRead(FILE** file, Arguments myArguments){
    if(myArguments.srcFilePathExists == 1){
        *file = fopen(myArguments.SRC_FILEPATH, "r");
    }
    else{
        *file = stdin;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    Arguments myArguments;
    if(argument_parser(argv,argc,&myArguments) != 0){
        fprintf(stderr, "wrong arguments");
    }
    printArguments(myArguments);

//set dns server address    
    char dnsServerAdress[100];
    if (myArguments.dnsIpExists == 1){
        //TODO chack ci nie je addressa moc velka 
        strcpy(dnsServerAdress, myArguments.UPSTREAM_DNS_IP);
    }
    else{
        findSystemDnsServer(dnsServerAdress);
    }

//set reading file from arguments or stdind
    FILE* file;
    prepareRead(&file,myArguments);        
    char* domainName = myArguments.BASE_HOST;
    prepareDomainName(domainName);

//connection
    int family = PF_INET;
    int type = SOCK_STREAM;
    int protocol = IPPROTO_TCP;
    int queueLimit = 1000;
    int socketId = socket(family, type, protocol);
    struct sockaddr_in foreignAddress;
    foreignAddress.sin_family = AF_INET;
    foreignAddress.sin_addr.s_addr = inet_addr(dnsServerAdress);
    foreignAddress.sin_port = htons(53);                      //TODO nastav na 53
    int foreignAddressSize = sizeof(foreignAddress);
    
    int status = connect(socketId, (struct sockaddr *) &foreignAddress, foreignAddressSize);
    char data[100] = {0};
    char character;

    
//first packet
    int8_t* super_buffer;
    long unsigned int encodedSize = 0;
    super_buffer = prepare_dns_packet(domainName, myArguments.DST_FILEPATH, &encodedSize);
    printBuffer(super_buffer, (18 + strlen(domainName)+ 1 + encodedSize));
    send(socketId, super_buffer, sizeof(int8_t)*(18 + strlen(domainName)+ 1 + encodedSize),0);
    free(super_buffer);
    strcpy(data,"\0");

//rest of packets
    int counter = 0;
    while ((character = fgetc(file)) != EOF){
        printf("new char %c\n",character);
        strncat(data, &character, 1);
        if (counter == CHUNK_SIZE){
            int8_t* super_buffer;
            long unsigned int encodedSize = 0;
            super_buffer = prepare_dns_packet(domainName, data, &encodedSize);
            printBuffer(super_buffer, (18 + strlen(domainName)+ 1 + encodedSize));
            send(socketId, super_buffer, sizeof(int8_t)*(18 + strlen(domainName)+ 1 + encodedSize),0);
            free(super_buffer);
            strcpy(data,"\0");
            counter = 0;
        } 
        counter++;
    }

//answer    
    char buffer[1024] = { 0 };
    int valread = recv(socketId, buffer, 1024, 0);
    printf("\n\nanswer of server\n");
    for (int i = 0; i < 1024; i++){
            printf("%d:",buffer[i]);
    }

    return 0;
}






