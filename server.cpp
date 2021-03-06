#include "server.h"

using namespace dicey2;

bool dicey2::sendMessage(char * messageArray){
	if(sendto(skt, messageArray, strlen(messageArray), 0, (struct sockaddr *)&addr2, sizeof(addr2)) < 0){
		perror("Unable to send message.");
		return 0;
	}
	return 1;
}

bool dicey2::sendPacket(Packet myPkt){
	if(sendto(skt, &myPkt, PACKET_SIZE, 0, (struct sockaddr *)&addr2, sizeof(addr2)) < 0){
		perror("Unable to send message.");
		return 0;
	}
	return 1;
}

int main(int argc, char* argv[]) {
    dicey2::prob_corrupt = argc > 1 ? std::atof(argv[1]) : 0.2;
    dicey2::prob_loss = argc > 2 ? std::atof(argv[2]) : 0.2;
    dicey2::prob_delay = argc > 3 ? std::atof(argv[3]) : 0.3;
    dicey2::length_delay = argc > 4 ? std::atof(argv[4]) : 4;
	
	if((skt = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("Unable to create socket.");
		return 0;
	}

	memset((char *)&addr1, 0, sizeof(addr1));
	addr1.sin_family = AF_INET;
	addr1.sin_addr.s_addr = htonl(INADDR_ANY);
	addr1.sin_port = htons(PORT_NO);

	if (bind(skt, (struct sockaddr *)&addr1, sizeof(addr1)) < 0){
		perror("Unable to bind socket.");
		return 0;
	}

	bit rcvdRequest = 0;
	while(!rcvdRequest){
		rlen = recvfrom(skt, buffer, PACKET_DATA_SIZE, 0, (struct sockaddr *)&addr2, &addr2Len);
		if (rlen > 0){
			rcvdRequest = 1;
			buffer[rlen] = 0;

			char * requestData = new char[rlen];
			char * filename = new char[rlen - 4];
			for(int i = 0; i < rlen; i++){
				requestData[i] = buffer[i];
				filename[i] = buffer[i + 4];
			}
			std::cout << "Received request: " << requestData << ", filename = " << filename << std::endl;
		
			//attempt to open file
			std::ifstream dataFile;
			dataFile.open(filename);
			if(dataFile.is_open()){
			
				//get filesize
  				dataFile.seekg (0, dataFile.end);
  				int filesize = dataFile.tellg();
  				dataFile.seekg(0, dataFile.beg);

				//is file 80KB or more? if not, send error
				if(filesize <= 80000){
					dataFile.close();
					std::cout << "ERROR: File is too small.";
					return 0;
				}

				//calculate how many times to pull out 507 bytes
				int numPackets = 1 + ((filesize - 1)/PACKET_DATA_SIZE);

				std::cout << "File size: " << filesize << ", packets to send = " << numPackets << std::endl;

				//create a buffer of packets that comprise the entire file to send
				Packet * fileBuffer = new Packet[numPackets];
				
				for (int i = 0; i < numPackets; i++){
					char * pktData = new char[PACKET_DATA_SIZE];
					dataFile.read(pktData, PACKET_DATA_SIZE);

                    if (i == numPackets - 1) { // this is the last packet
                      //std::cout << "Packet #" << i + 1 << " out of " << numPackets << std::endl;
                      pktData[506] = '\0';
                    }
                  
					Packet filePkt(i%32,  pktData);
					
					
					fileBuffer[i] = filePkt;
				}
              
				//acks we have already received
				int ackCount = 0;
				int base = 0;

				while (ackCount < numPackets){
					
					Packet printPkt = fileBuffer[ackCount];
					//pthread_create()
					char * pktData = printPkt.getData();
					sendPacket(printPkt);
					char * sampleData = new char[48];
					for (int k = 0; k < 48; k++){
						sampleData[k] = pktData[k];
					}
					std::cout << std::endl << std::endl << "Packet: seq_num = " << printPkt.getSeqNum() << "; ack = " << printPkt.getAck() << "; checksum = " << printPkt.getChecksum() << "; data = \"" << sampleData << "\"" << std::endl;
					ackCount++;
				}

                //send first window
                for (int i = 0; i < WINDOW_SIZE; i++){

                }


                //for (int i = 0; i < numPackets; i++) {
                  //  sendPacket(fileBuffer[i]);
                    //if (i%32 == 16 || i%32 == 0) {
                      //  std::cout << "ACK RECEIVED. SENDING NEXT WINDOW." << std::endl;
                    //}
                //}
			}
            dataFile.close();		
		}
	}

	return 0;
}
