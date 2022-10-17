.PHONY: all receiver sender clean

all: receiver sender

receiver:
	#g++ ./receiver/dns_receiver_events.h ./receiver/dns_receiver_events.c ./receiver/dns_receiver.c -o receiver_run
	g++ -o server ./receiver/dns_receiver.c -lpcap
sender:
	#g++ ./sender/dns_sender_events.h ./sender/dns_sender_events.c ./sender/dns_sender.c -o sender_run
	gcc -o client sender/dns_sender.c -lpcap
clean:
	rm sender_run receiver_run
