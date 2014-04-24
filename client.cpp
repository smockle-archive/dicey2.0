#include "client.h"

using namespace dicey2;

int main(int argc, char* argv[]) {
	dicey2::srv_ip_address = argc > 1 ? argv[1] : "127.0.0.1";//"131.204.14.205";
	dicey2::filename = argc > 2 ? argv[2] : "TestFile";

    if(!openSocket())
		return 0;
	
	std::cout << "GET " << filename << std::endl << std::endl;
	
	
	std::string getRequestString = "GET " + filename;
	char * getRequestCharArray = (char*)getRequestString.c_str();

	if(!sendMessage(getRequestCharArray)){
		perror("Unable to send GET request.");
		return 0;
	}

	bit pktRcvd = 0;
	while(!pktRcvd){
		if(rcvPacket())
			pktRcvd = 1;
	}



	return 0;
}

bool dicey2::openSocket(){
	//create socket    
	if ((skt = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("Unable to create socket.");
		return 0;
	}

	memset((char *)&sktaddr, 0, sizeof(sktaddr));
	sktaddr.sin_family = AF_INET;
	sktaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sktaddr.sin_port = htons(0);

	if (bind(skt, (struct sockaddr *)&sktaddr, sizeof(sktaddr)) < 0){
		perror("Unable to bind socket.");
		return 0;
	}

	memset((char *)&srvaddr, 0, sizeof(srvaddr));
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_port = htons(PORT_NO);
	inet_pton(AF_INET, srv_ip_address.c_str(), &(srvaddr.sin_addr));

	std::cout << "Server IP: " << inet_ntoa(srvaddr.sin_addr) << ":" << ntohs(srvaddr.sin_port) << std::endl;
	return 1;
}

bool dicey2::sendPacket(Packet myPkt){
	char * packetAsArray = myPkt.getPacketAsCharArray();
	if (!sendMessage(packetAsArray))
	{
		std::cout << std::endl << std::endl << "Sending Packet: seq_num = " << myPkt.getSeqNum() << "; ack = " << myPkt.getAck() << "; checksum = " << myPkt.getChecksum() << "; data = " << myPkt.getData() << std::endl;
		return 0;
	}
	else
		return 1;
}

bool dicey2::sendMessage(char * messageArray){
	if(sendto(skt, messageArray, strlen(messageArray), 0, (struct sockaddr *)&srvaddr, sizeof(srvaddr)) < 0){
		perror("Unable to send message.");
		return 0;
	}
	return 1;
}

bool dicey2::rcvPacket(){
	int rcvPoll = 0;
	struct pollfd ufds;
	time_t timer;

	ufds.fd = skt;
	ufds.events = POLLIN;
	rcvPoll = poll(&ufds, 1, 20);

	if( rcvPoll == -1 ) {
		perror("Unable to poll socket.");
		return 0; 
	} 
	else if( rcvPoll == 0 ) {
		perror("Timeout");
		return 0;
	} 
	else {
		int recvLen;
		recvLen = recvfrom(skt, buffer, PACKET_SIZE, 0, (struct sockaddr *)&srvaddr, &srvaddrLen);
		if (recvLen > 0){
			Packet* srvPkt = new Packet;                 
			buffer[recvLen] = 0;
			memcpy(srvPkt, buffer, PACKET_SIZE);
			char * pktData = srvPkt->getData();
			char * sampleData = new char[48];
					for (int k = 0; k < 48; k++){
						sampleData[k] = pktData[k];
					}
			std::cout << std::endl << std::endl << "Received Packet: seq_num = " << srvPkt->getSeqNum() << "; ack = " << srvPkt->getAck() << "; checksum = " << srvPkt->getChecksum() << "; data = " << sampleData << "; recvLen = " << recvLen << std::endl;
			if (recvLen < PACKET_DATA_SIZE)
				return 1;
		} 
	}
	return 0;
}