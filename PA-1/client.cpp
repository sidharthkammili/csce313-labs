/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Sidharth Kammili
	UIN: 533005145
	Date: 09/28/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/wait.h>

using namespace std;


int main (int argc, char *argv[]) {
	int opt; //getopt()
	int p = -1; //PID (unset = -1)
	double t = -1.0; //time seconds (unset < 0)
	int e = -1; //ecg number (1 or 2, unset = -1)
	int m = MAX_MESSAGE; //buffer capacity; defaults to max_message
	bool new_channel_request = false; //request NEWCHANNEL_MSG if -c is present
		
	string filename = ""; //empty if not set
	while ((opt = getopt(argc, argv, "p:t:e:f:")) != -1) { //parse CLI flags
		switch (opt) {
			case 'p': //PID
				p = atoi (optarg);
				break;
			case 't': //time (seconds)
				t = atof (optarg);
				break;
			case 'e': //ecg number (1 or 2)
				e = atoi (optarg);
				break;
			case 'f': //filename
				filename = optarg;
				break;
			case 'm': //buffer capacity override
				m = atoi (optarg);
				break;
			case 'c': //request new channel
				new_channel_request = true;
				break;
		}
	}

	std::vector<FIFORequestChannel*> channels;

	int pid = fork(); //launch server as a child

	//child process: exec the server
	if (pid == 0) {
		//only send option -m to server because server doesn't understand the other 5 options
		std::string m_str = std::to_string(m);
		exec1("./server", "server", "-m", m_str.c_str(), (chat*) nullptr);
		//if execl returns, then it failed:
		perror("exec failed");
		_exit(127);
	}

	//parents process: act as client
	if (pid > 0) {
		//connect to control channel on client side
		FIFORequestChannel channel1("control", FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(&channel1); //index 0 will be the control channel

		if (new_channel_request) {
			//NEWCHANNEL request issued on control channel
			MESSAGE_TYPE new = NEWCHANNEL_MSG;
			channel1.cwrite(&new, sizeof(MESSAGE_TYPE));
		}

		//read back server-allocated channel name
		char namebuffer[64] = {0};
		channel1.cread(namebuffer, sizeof(namebuffer)); //receive C string
		std::string newName(namebuffer);

		//open new channel on client side and track the channel
		FIFORequestChannel* p = new FIFORequestChannel(newName.c_str(), FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(p);
	}

	//


}
