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
#include <cstring>
#include <unistd.h>
#include <vector>
#include <sys/wait.h>
#include <sys/stat.h>

using namespace std;


int main (int argc, char *argv[]) {
	int opt; //getopt()
	int p = -1; //PID (unset = -1)
	double t = -1.0; //time seconds (unset < 0)
	int e = -1; //ecg number (1 or 2, unset = -1)
	int m = MAX_MESSAGE; //buffer capacity; defaults to max_message
	bool new_channel_request = false; //request NEWCHANNEL_MSG if -c is present
		
	string filename = ""; //empty if not set
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) { //parse CLI flags
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

	//ensure output dir exists
	mkdir("received", 0755);

	std::vector<FIFORequestChannel*> channels;

	int pid = fork(); //launch server as a child

	//child process: exec the server
	if (pid == 0) {
		//only send option -m to server because server doesn't understand the other 5 options
		std::string m_str = std::to_string(m);
		execl("./server", "server", "-m", m_str.c_str(), (char*) nullptr);
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
			MESSAGE_TYPE nc = NEWCHANNEL_MSG;
			channel1.cwrite(&nc, sizeof(MESSAGE_TYPE));
		//read back server-allocated channel name
		char namebuffer[64] = {0};
		channel1.cread(namebuffer, sizeof(namebuffer)); //receive C string
		std::string newName(namebuffer);

		//open new channel on client side and track the channel
		FIFORequestChannel* p = new FIFORequestChannel(newName.c_str(), FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(p);
	}

	//use most recently created channel (a new channel if requested, or else: control)
	FIFORequestChannel* chan = channels.back();

	//mode 1, single data point if -p, -t, and -e are all set
	if (p!=-1 && e!=-1 && t!=-1.0) {
		datamsg push(p,t,e); //construct request struct
		char buffer[MAX_MESSAGE]; //temp buffer for sending bytes
		memcpy(buffer, &push, sizeof(datamsg)); //raw-copy struct bytes into buffer
		chan->cwrite(buffer, sizeof(datamsg)); //send sizeof(datamsg)

		double reply = 0.0; //server replies w/ one double
		chan->cread(&reply, sizeof(double)); //read 1 double
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}

	//mode 2: first 1000 samples for a person (both ecg1 and ecg2) if only -p is set
	else if(p!=-1) {
		double time = 0.0; //start at t = 0.000
		ofstream outputFile("received/x1.csv"); //write CSV to received/x1.csv
		for (int i = 0; i < 1000; i++) {
			datamsg push1(p, time, 1); //request ecg1 at time
			char buffer[MAX_MESSAGE];
			memcpy(buffer, &push1, sizeof(datamsg));
			chan->cwrite(buffer, sizeof(datamsg));
			double reply1 = 0.0;
			chan->cread(&reply1, sizeof(double));

			datamsg push2(p, time, 2); //request ecg2 at time
			//char buffer[MAX_MESSAGE] //reuses same buffer, so not necessary again
			memcpy(buffer, &push2, sizeof(datamsg));
			chan->cwrite(buffer, sizeof(datamsg));
			double reply2 = 0.0;
			chan->cread(&reply2, sizeof(double));

			//write row "time, ecg1, ecg2"
			outputFile << time << ',' << reply1 << ',' << reply2 << endl;
			time += 0.004; //next sample time
		}
		outputFile.close();
	}

	//mode 3: file transfer if -f is present (independent of p/t/e branch)
	if (!filename.empty()) {
		//1) probe file size w/ filemsg(0,0) + filename in one write
		filemsg fm(0,0);
		int len = sizeof(filemsg) + (filename.size() + 1); //header + "name\0"
		char* buffer3 = new char[len]; //request buffer

		memcpy(buffer3, &fm, sizeof(filemsg)); //pack header
		strcpy(buffer3 + sizeof(filemsg), filename.c_str()); //pack trailing filename
		chan->cwrite(buffer3, len); //single write

		__int64_t filesize = 0;
		chan->cread(&filesize, sizeof(__int64_t)); //server returns size

		//prepare ouptut in binary mode (for arbitrary files)
		ofstream outputFile("received/"+filename, ios::binary);

		//allocate data buffers: buffer4 for reading data, buffer5 for subsequent requests
		char* buffer4 = new char[m]; //data receive buffer (<= buffer capacity)
		char* buffer5 = new char[len]; //request buffer (header+name)
		__int64_t offset = 0;

		//2) chunk loop for all full-size chunks of length m
		while (offset < filesize - m) { //process all but last chunk
			filemsg fn(offset, m); //header for this chunk
			memcpy(buffer5, &fn, sizeof(filemsg));
			strcpy(buffer5 + sizeof(filemsg), filename.c_str());
			chan->cwrite(buffer5, len); //single write per request
			chan->cread(buffer4, m); //read m bytes
			outputFile.write(buffer4, m); //write to output
			offset += m;
		}

		//3) handle final chunk (could be m, or could be smaller)
		__int64_t lastChunk = filesize - offset;
		filemsg fn(offset, lastChunk);
		memcpy(buffer5, &fn, sizeof(filemsg));
		strcpy(buffer5 + sizeof(filemsg), filename.c_str());
		chan->cwrite(buffer5, len);
		chan->cread(buffer4, lastChunk);
		outputFile.write(buffer4, lastChunk); 
		outputFile.close();
		
		//free dynamic buffers for this transfers
		delete[] buffer5;
		delete[] buffer4;
		delete[] buffer3;
	}

	//shutdown for all channels (control + optional new channels)
	for (long unsigned int i = 0; i < channels.size(); i++) {
		MESSAGE_TYPE mes = QUIT_MSG; //tell server we're done
		FIFORequestChannel* chan = channels[i];
		chan->cwrite(&mes, sizeof(MESSAGE_TYPE));
		if (i!=0) { //delete only heap-allocated channels
			delete chan; //control was on stack
		}
	}

	int status = 0;
	waitpid(pid, &status, 0); //reap server child
	return 0;
}


}
