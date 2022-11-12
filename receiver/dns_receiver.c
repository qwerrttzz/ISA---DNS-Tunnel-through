//Serverová aplikace bude naslouchat na implicitním portu pro DNS komunikaci. -Done
//Příchozí datové přenosy bude ukládat na disk ve formě souborů.              -Done
//Komunikační protokol mezi klientem a serverem je implementační detail.      
//
//dns_receiver {BASE_HOST} {DST_FILEPATH}
//$ dns_receiver example.com ./data
//
//{BASE_HOST} slouží k nastavení bázové domény k příjmu dat                   -Done
//{DST_FILEPATH} cesta pod kterou se budou všechny příchozí data/soubory ukládat (cesta specifikovaná klientem bude vytvořena pod tímto adresářem) -Done

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#define CHUNK_SIZE 5

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

void printBuffer(char* buffer,int len){
    for(int i = 0; i <= len; i++){
        printf("\n %hX %c",buffer[i],buffer[i]);
        fflush(stdout);
    }
}

int getDataFromPacket(char* buffer,char* data, int* dataLen){
    *dataLen = (int)buffer[14];
    //printf("dlzka dat: %d\n", *dataLen);

    int counter = 0;
    for (int i = 15; i < (15 + *dataLen); i++){
        //printf("data: %c\n", buffer[i]);
        data[counter] = buffer[i];
        counter++;
    }
    return 0;
}

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};

static char *decoding_table = NULL;

void build_decoding_table() {

    decoding_table = (char*)malloc(256);

    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}

void base64_cleanup() {
    free(decoding_table);
}

unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length) {

    if (decoding_table == NULL) build_decoding_table();

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = (unsigned char *)malloc(*output_length);
    if (decoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}

int numberOfDir(char* path){
    int count=0;
    for (int i = strlen(path); i >= 0; i--)
    {
        if (path[i] == '/')
        {
            count++;
        }
        
    }
    return count;
}

int cutFirstDir(char* path, char* token, int* numberOfDirs){
    
    int i = strlen(path);
    char newPath[strlen(path)];
    char* newToken=(char*) malloc(100);

    strcpy(newToken, "");
    strcpy(newPath, "");

    *numberOfDirs = 0;
    int secondPathCounter = 0;
    int tokenCounter = 0;
    for (int i = 0; i < strlen(path); i++)
    {
        if (path[i]=='/' && i!=0){
            *numberOfDirs = *numberOfDirs + 1;
            newPath[secondPathCounter] = path[i];
            secondPathCounter++;
        }
        else if (*numberOfDirs == 0){
            
            if (path[i] != '/')
            {
                //printf("cut first dir 1\ntoken:%c\n path:%c\n i:%d\n",newToken[tokenCounter],path[i],tokenCounter);
                newToken[tokenCounter] = path[i];
                //printf("cut first dir 2\ntoken:%c\n path:%c\n i:%d\n%s",newToken[tokenCounter],path[i],tokenCounter,newToken);
                tokenCounter++; 
                
            }
            
        }
        else if (*numberOfDirs != 0){
            newPath[secondPathCounter] = path[i];
            secondPathCounter++;
        }
    }
    newToken[tokenCounter] = '\0';
    newPath[secondPathCounter] = '\0';
    //printf("cut first dir\n    oldpath:%s\n    newpath:%s\n    token:%s\n    nb:%d\n",path,newPath,newToken,i);
    
    strcpy(path,newPath);
    strcpy(token, newToken);
    free(newToken);
    return 0;
}

FILE* createFilePath(char* path, char* myLocation){
    
    int count = 0;
    char* token = (char*)malloc(255);
    int numberOfDirs = 0;
    
    cutFirstDir(path, token, &numberOfDirs);
    
    //update location
    char delimeter[2]="/";
    strncat(myLocation, delimeter, 2);
    strncat(myLocation, token, strlen(token));
    
    if (numberOfDirs == 0)
    {   
        //create file in location
        //printf("createFilePath\n    path:%s\n    token:%s\n    myLocation:%s\n", path, token, myLocation);
        return fopen(myLocation,"w");
    }

    //create new dir at location
    //printf("createFilePath\n    path:%s\n    token:%s\n    myLocation:%s\n", path, token, myLocation);
    mkdir(myLocation, S_IRWXU);
    return createFilePath(path, myLocation);
}

char* buildRelativePath(Arguments myArguments, int decodedLen, char* decodedData){
    char* destination = (char*)malloc(strlen(myArguments.DST_FILEPATH)+decodedLen+1);
    strncat(destination, myArguments.DST_FILEPATH,strlen(myArguments.DST_FILEPATH));
    char symbol = '/';
    strncat(destination, &symbol, 1);
    strncat(destination, (char*)decodedData, decodedLen);

    return destination;
}

int getPacketSize(char* buffer){
    int jump = (int)buffer[14];
    int dataCount = 14;
    int counter = 0;
    while (jump != 0)
    {
        dataCount = dataCount + jump+1;

        //printf("                         jump: %d datacount: %d buffer: %hX\n",jump, dataCount,(int)buffer[dataCount]);
        jump = (int)buffer[dataCount];
        
        fflush(stdout);    
    }
    
    
    return dataCount+5; //pricitam posledne flagy
}

void changeFlags(char* buffer){
    int16_t header = 0x8183;
    memcpy(&buffer[2], &header, 1 * sizeof( short int ));
}

void changeQuaryToResponse(char* buffer, int PacketLen){
    changeFlags(buffer);
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
    
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.53");//"127.0.0.53");
    serverAddress.sin_port = htons(8000);
    int serverAddressLen = sizeof(serverAddress);

    int queueLimit = 1000;
    
    struct sockaddr_in clientAddress;
    unsigned int clientAddressLen = sizeof(clientAddress);
    
//connection
    int socketId = socket(family, type, protocol);
    int statusBind = bind(socketId, (struct sockaddr *) &serverAddress, serverAddressLen);
    int statusListen = listen(socketId, queueLimit);
    
    printf("errors %d %d %d  \n",socketId, statusBind, statusListen);
    fflush(stdout);
    int s = accept(socketId, (struct sockaddr *) &clientAddress, &clientAddressLen);
    
//receiving data
    char buffer[1024] = { 0 };
    int size;
    int first = 1;
    FILE* file;
    while((size = recv(s, buffer, 1024, 0)) != 0){
        
        //read data from packet
        char encoded_data[100] = {0};
        int dataLen = 0;
        getDataFromPacket(buffer, encoded_data, &dataLen);
        
        //decode data from packet data
        unsigned char* decodedData;
        long unsigned int decodedLen = 0;
        decodedData = base64_decode(encoded_data, dataLen, &decodedLen);        
        if (first == 1){
            //if first packet create file and addresars acording to data from packet
            char* destination = buildRelativePath(myArguments, decodedLen,(char*) decodedData);
            printf("destination: %s\n",destination);
            
            char cwdBuffer[1024] = { 0 };
            file = createFilePath(destination, getcwd(cwdBuffer, 1024));
            free(destination);
            first = 0;
        }
        else if(first != 1){
            //write data from packets to file
            fprintf(file,"%s",decodedData);
            fflush(file);
            printBuffer(buffer,strlen(buffer));
            fflush(stdout);
        }
            
        //response
        changeQuaryToResponse(buffer, getPacketSize(buffer));
        
        printf("encoded data: %s\ndecoded data: %s \nfirst: %d",encoded_data,decodedData,first);
        printf("\n\n####################################################################################################\n");
        write(s, buffer,getPacketSize(buffer));
        printf("\ndecoded_data:%s\n",decodedData);
        
        
    }
    
    
    
    
    close(socketId);
    shutdown(statusListen, SHUT_RDWR);
    return 0;
}
