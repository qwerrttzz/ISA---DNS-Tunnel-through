

#include <pcap.h>
#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <sys/socket.h>

#define BUFSIZE 10000

void open_sniffing_session(pcap_t** handle,char* device, char* errorBuffer);
void got_packet(const struct pcap_pkthdr *header, const u_char *packet);
void set_device(char* device, char* errorBuffer);
void start_sniffing_loop(pcap_t* handle, char* device, char* errorBuffer,int packet_count);
void compile_filter_expression(pcap_t** handle, bpf_program* filter, char* filter_expression);
void set_filter(pcap_t** handle, bpf_program filter);

int main(int argc, char const *argv[])
{
    char device[1000];
    char errorBuffer[PCAP_ERRBUF_SIZE];
    int packet_count = 100;
    char filter_expression[] = "udp or tcp";
    pcap_t* handle;
    struct bpf_program filter;

    set_device(device, errorBuffer);
    open_sniffing_session(&handle, device, errorBuffer);
    compile_filter_expression(&handle, &filter, filter_expression);
    set_filter(&handle,filter);
    start_sniffing_loop(handle, device, errorBuffer, packet_count);

    return 0;
}

void got_packet(const struct pcap_pkthdr *header, const u_char *packet){
    printf("hi");
}

void set_device(char* device, char* errorBuffer){
    pcap_if_t* devices;
    pcap_findalldevs(&devices, errorBuffer);
    strcpy(device,devices->next->name);
    printf("%s",device);
}

void open_sniffing_session(pcap_t** handle, char* device, char* errorBuffer){
    *handle = pcap_open_live(device, BUFSIZE, 1, 1000, errorBuffer);
    if(*handle == NULL){
        fprintf(stderr,"chyba pri pcap_open live %s",errorBuffer);
    }
}

void start_sniffing_loop(pcap_t* handle, char* device, char* errorBuffer,int packet_count){
    pcap_dumper_t *dump;
    u_char* conf = NULL;
    char user[20000];

    if(pcap_loop(handle, packet_count,(pcap_handler) got_packet,conf) < 0){
        pcap_perror(handle, "Chyba pri pcap_loop");
    }
}

void compile_filter_expression(pcap_t** handle, bpf_program* filter, char* filter_expression){
    if(pcap_compile(*handle, filter, filter_expression, 0, -1) == -1){
        fprintf(stderr, "chyba pri pcap compile %s: %s\n", filter_expression, pcap_geterr(*handle));
    }
}

void set_filter(pcap_t** handle, bpf_program filter){
    if(pcap_setfilter(*handle, &filter) == -1){
        fprintf(stderr, "chyba pri pcap_setfilter: %s\n", pcap_geterr(*handle));                
    }
}