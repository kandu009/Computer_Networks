/*
 * Main Client file which is used to interact with both the content servers as well as
 * Credential servers. Actions supported by a client are
 * 1. register 	: register a new user
 * 2. login 	: login for registered user
 * 3. list		: list all files available for download at each server
 * 4. get		: download a given file from given server using given mechanism
 * 5. wiseget	: Intelligent get to download using the faster protocol from the fastest server
 * 6. exit		: exit or log out a client
 */
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <sys/time.h>
#include "./include/client.h"
#include "../server/include/server.h"
#include <limits.h>

typedef std::map<std::string, std::set<std::string> > stringSetMap;
typedef std::map<std::string, std::pair<double, double> > stringDoubleMap;

using namespace std;

/*
 * method to show the help text to manage client operations
 */
void showCommandList() {
	string commands;
	commands.append("\nEnter 1 to 6 based on the list shown below ...\n")
		.append("1. register\n") 
		.append("2. login\n")
		.append("3. list\n")
		.append("4. get\n")
		.append("5. wiseget\n")
		.append("6. exit\n");
	cout << commands << endl;
}

/*
 * Gets the value of a given key mentioned in the given config file
 * @configKey	: key value
 */
std::string getValueFromConfig(std::string& configKey) {

	if(configKey.empty()) {
		return "";
	}

	ifstream myfile;
	std::string file = "client.cfg";
	myfile.open(Common::getFullPath(file).c_str());
	string line;
	string output;

	if(!myfile.is_open()) {
		// return if file cannot be opened
		cout << "client log:: warn:: could not load the client config file" << endl;
		return "";
	}

	while(getline(myfile, line)) {
		if(line.find(configKey) == 0) {
			// parse the key value pair and choose the right value
			std::vector<std::string> toks = Common::tokenize(line, const_cast<const char *>("="));
			if(2 != toks.size()) {
				continue;
			}
			output = toks[1];
		}
	}

	return output;
}

/*
 * method to calculate the average Round Trip Time from 
 * all the content servers to the client for both TCP and UDP mechanisms
 * 
 * @host	: host of the server
 * @port	: port of the server
 * @is_udp	: true if its a UDP connection, false otherwise
 *
 */
double averageRTT(std::string& host, int port, bool is_udp) {

	double time = 0;
	Client c;
	c.connectToServer(host, port); 		// connect to the server whose RTT should be calculated

	// extract the port on which UDP server should run
	std::string key = "udp_probe_server_port=";
	int udpserverport = atoi(getValueFromConfig(key).c_str());
	Server udpserver(udpserverport, true);
	if(is_udp) {
		udpserver.initialize();			// initialize a UDP server to calculate RTT for UDP case
	}

	for(int i = 0; i < 10; ++i) {		// RTT should be calculated by an average of 10 measurements

		struct timeval start, end;
		std::string response;
		std::stringstream ss;
		ss << "probe message ";

		if(is_udp) {
			// send probe messages to servers to calculate the RTT
			string udpresponse;
			gettimeofday(&start, NULL);
			ss << start.tv_usec << " udp ";
			ss << udpserver.getHostname() << " " << udpserverport;
			c.sendData(ss.str());							// send the probe message to server								

			if(!udpserver.receive(udpresponse)) {	
				cout << udpresponse << endl;				// receive the response to probe messages
			}
			gettimeofday(&end, NULL);

		} else {
			gettimeofday(&start, NULL);						// send the probe message to server
			c.sendData(ss.str());
			c.receive(response);							// receive the response to probe messages
			gettimeofday(&end, NULL);

		}

		time += (double)(end.tv_usec-start.tv_usec);		// Calculate the cumulative RTT
	}

	return (time/10000); // (time_in_microseconds / 1000*10);	// average the RTT value for 10 trips
}

/*
 * method to pre calculate all the RTT times from a client to all content servers
 *
 * @credential				: client object which talks to credential server
 * @serverVsTcpUdpTime		: map of server Vs TCP and UDP times
 * @serverVsFiles			: Map of server Vs their corresponding files
 *
 */
void initializeRoundTripTimes(Client& credential, stringDoubleMap& serverVsTcpUdpTime, stringSetMap& serverVsFiles) {

	credential.sendData("list");

	std::string response;
	credential.receive(response);

	// parse each server information
	std::vector<std::string> serverfiles = Common::tokenize(response, const_cast<const char *>("|"));
	for(int i =1; i < serverfiles.size(); ++i) {
		std::vector<std::string> toks = Common::tokenize(serverfiles[i], const_cast<const char *>(" "));
		if(toks.size() < 2) {
			continue;				// ignore if there is no files mentioned
		}

		std::set<std::string> files;
		for(int j = 1; j < toks.size(); ++j) {
			files.insert(toks[j]);	// extract the list of files for each server
		}
		// store the map of server and its associated files
		serverVsFiles.insert(std::pair<std::string, std::set<std::string> >(toks[0], files));

		// calculate RTT for tcp and udp
		std::vector<std::string> ts = Common::tokenize(toks[0], const_cast<const char *>(":"));
		double tcptime = averageRTT(ts[0], atoi(ts[1].c_str()), false);
		double udptime = averageRTT(ts[0], atoi(ts[1].c_str()), true);

		// store the server Vs its corresponding TCP and UDP times
		std::pair<double, double> p = std::make_pair(tcptime, udptime);
		serverVsTcpUdpTime.insert(std::pair<std::string, std::pair<double, double> >(toks[0], p));
	}
}

/*
 * method to handle the register request
 * @credential	: client who can talk to credential server
 */
void handleRegisterRequest(Client& credential) {

	std::string request("register "), r;
	std::cout << "Enter <username> <password> <retypepassword> " ;
	std::getline(std::cin, r);
	credential.sendData(request.append(r));		// send registeration command to server

	std::string response;
	credential.receive(response);				// show the response to the registeration request
	cout << response << endl;

}

/*
 * method to handle login request
 * @credential	: client who can talk to credential server
 * @userID		: user ID of the current client
 *
 */
void handleLoginRequest(Client& credential, std::string &userID) {

	cout << "Enter <username> <password> " << endl;	

	std::string request("login "), r;
	std::getline(std::cin, r);
	credential.sendData(request.append(r));						// send the login request to server

	std::string response;
	credential.receive(response);								// show the response received for the login request

	if(0 == response.find(ERR_200)) {
		std::vector<std::string> toks = Common::tokenize(r, const_cast<const char *>(" "));
		if(toks.size() > 0) {
			userID = toks[0];
		}
	}

	cout << response << endl;

}

/*
 * method to handle the list command
 * 
 * @ credential			: client to interact with server
 * @ userID				: user ID of the client
 * @ serverVsFiles		: server vs list of files that can be downloaded
 * @ serverVsTcpUdpTime	: server vs tcp and udp RTT times
 *
 */
void handleListRequest(Client& credential, std::string& userID, 
		stringSetMap& serverVsFiles, stringDoubleMap& serverVsTcpUdpTime) { 

	credential.sendData("list");

	std::string response;
	std::string output;
	credential.receive(response);

	// parse the set of server files
	std::vector<std::string> serverfiles = Common::tokenize(response, const_cast<const char *>("|"));
	for(int i =1; i < serverfiles.size(); ++i) {
		// collect each server information
		std::vector<std::string> toks = Common::tokenize(serverfiles[i], const_cast<const char *>(" "));
		if(toks.size() < 2) {		
			continue;				// ignore if complete information is not given
		}

		std::set<std::string> files;
		for(int j = 1; j < toks.size(); ++j) {
			files.insert(toks[j]);		
		}
		// store the server and files information
		serverVsFiles.insert(std::pair<std::string, std::set<std::string> >(toks[0], files));

		// calculate RTT for tcp and udp
		std::vector<std::string> ts = Common::tokenize(toks[0], const_cast<const char *>(":"));
		double tcptime = averageRTT(ts[0], atoi(ts[1].c_str()), false);
		double udptime = averageRTT(ts[0], atoi(ts[1].c_str()), true);

		// store the server and RTT values
		std::pair<double, double> p = std::make_pair(tcptime, udptime);
		serverVsTcpUdpTime.insert(std::pair<std::string, std::pair<double, double> >(toks[0], p));

		// collect the output response
		stringstream ss;
		ss << serverfiles[i] << " ,TCP Time: " << tcptime << "ms, UDP Time: " << udptime << "ms\n";
		output.append(ss.str());
	}

	// print the output response
	cout << output << endl;

}

/*
 * method to handle the get request
 * 
 * @ filename	: file to be downloaded
 * @ server		: server from where file is downloaded
 * @ mechanism	: tcp ot udp mechanism used
 * @ response	: response returned from server
 * @ serverVsFiles	: server vs list of files
 *
 */ 
void handleGetRequest(std::string& filename, std::string& server, 
		std::string& mechanism, std::string &response, stringSetMap& serverVsFiles) {

	// check if the file exists at the server
	bool foundFile = false;
	for(stringSetMap::iterator it = serverVsFiles.begin(); it != serverVsFiles.end(); ++it) {
		if(server.compare(it->first) == 0) {
			std::set<std::string> files = it->second;
			if(files.find(filename) != files.end()) {
				foundFile = true;
			}
		}
	}
	if(!foundFile) {
		response = ERR_415;		// return with error response in case if we don't find the file
		return;
	}

	// extract the server IP and port information
	std::vector<std::string> toks = Common::tokenize(server, const_cast<const char *>(":"));
	if(2 != toks.size()) {			
		response.append(ERR_405).append(" Invalid server/port information.");
		return;
	}

	// connect to the server
	Client cl;			
	cl.connectToServer(toks[0], atoi(toks[1].c_str()));
	string request("get ");
	request.append(filename).append(" ").append(server).append(" ").append(mechanism);

	if(mechanism.compare("udp") == 0) {

		// if udp, start a UDP server that receives the file send by the server
		std::string key = "udp_server_port=";
		int udpserverport = atoi(getValueFromConfig(key).c_str());
		Server udpserver(udpserverport, true);
		udpserver.initialize();

		// send the request
		std::stringstream ss;
		ss << udpserverport;
		request.append(" ").append(udpserver.getHostname()).append(" ").append(ss.str());
		struct timeval start, end;
		gettimeofday(&start, NULL);
		cl.sendData(request);		

		// receive the file
		string udpresponse;
		if(!udpserver.receiveFile(filename, udpresponse)) {
			response = ERR_410;
			cout << "Could not receive file [ " << filename << " ]" << endl;
			return;
		}
		gettimeofday(&end, NULL);
		// calculated the time taken to download the file
		double time = (double)(end.tv_usec-start.tv_usec);

		// print the response
		cout << udpresponse << " " << filename << " " << server << " " << mechanism << " ";
		cout << "time : " << time << " ms." << endl;
	} else {

		// if its a tcp connection. receive the file on the same connection
		struct timeval start, end;
		gettimeofday(&start, NULL);

		// send file download request
		cl.sendData(request);
		string tcpresponse;
		//receive the file
		cl.receiveFile(filename, tcpresponse);
		gettimeofday(&end, NULL);

		// calculate time takn to download the file
		double time = (double)(end.tv_usec-start.tv_usec);

		// print the response
		cout << tcpresponse << " " << filename << " " << server << " " << mechanism << " ";
		cout << "time : " << time << " ms." << endl;
	}

}

/*
 * method to handle get request
 */
void handleGetRequest(stringSetMap& serverVsFiles) {

	// If get is called before list, it should not be allowed. as client is 
	// not supposed to know the content server details
	if(serverVsFiles.size() <= 0) {
		std::cout << "How do you know the server details. Please go through authentic way to get files ..." << std::endl;
		return;
	}

	std::string request;
	cout << "Enter <filename> <servername:port> <mechanism> " <<endl;
	std::getline(std::cin, request);

	// parse the request arguments
	std::vector<std::string> toks = Common::tokenize(request, const_cast<const char *>(" "));
	std::string response;
	if(3 != toks.size()) {
		response.append(ERR_405);
	} else {
		handleGetRequest(toks[0], toks[1], toks[2], response, serverVsFiles);
	}
	// print the response
	cout << response << endl;
}

/*
 * method to handle wiseget
 * @ credential			: client which talks to server
 * @ serverVsTcpUdpTime	: servers vs its correpsonding tcp and udp time
 * @ serverVsFiles		: server vs its list of files
 *
 */
void handleWisegetRequest(Client& credential, stringDoubleMap& serverVsTcpUdpTime, stringSetMap& serverVsFiles) {

	// If wise get is called before list, we need the RTT data
	if(serverVsTcpUdpTime.size() <= 0) {
		initializeRoundTripTimes(credential, serverVsTcpUdpTime, serverVsFiles);
	}

	string filename;
	string server;
	string protocol;

	cout << "Enter <filename> " <<endl; 
	std::getline(std::cin, filename);	

	// choose the server with minimum RTT
	int minTime = INT_MAX;
	for(stringDoubleMap::iterator it = serverVsTcpUdpTime.begin(); it != serverVsTcpUdpTime.end(); ++it) {
		std::pair<double, double> p = it->second;
		if(p.first < p.second) {
			if(p.first < minTime) {
				protocol = "tcp";
				server = it->first;
			}
		} else {
			if(p.second < minTime) {
				protocol =  "udp";
				server = it->first;
			}
		}
	}
	// make a get request with the abover server information
	string response;
	handleGetRequest(filename, server, protocol, response, serverVsFiles);
	cout << response << endl;

}

/*
 * method to handle the exit request
 * @credential 	: client which talks to server
 * @userID		: user ID of client
 */
void handleExitRequest(Client& credential, std::string& userID) {

	// If there is no user ID , ignore
	if(userID.empty()) {
		cout << "Exiting from the program ... " << endl;
		return;
	}

	// send exit request
	credential.sendData("exit " + userID);
	string response;
	//print the response received
	credential.receive(response);
	cout << response << endl;

}

int main(int argc, char *argv[]) {

	Client credential;

	//pick up credential server configuration from config file
	std::string key("credentialserver=");
	std::string hostport(getValueFromConfig(key));
	std::vector<std::string> toks = Common::tokenize(hostport, const_cast<const char *>(":"));
	if(2 != toks.size()) {
		cout << "Cannot reach the server. Please check the server configuration again.." << endl;
		return 1;
	}
	if(!credential.connectToServer(toks[0], atoi(toks[1].c_str()))) {
		std::cout << "could not connect to server. please try again ..." << endl;
		return 1;
	}

	// map to keep all the server vs files information
	stringSetMap serverVsFiles;
	// map to keep all the server vs RTT information
	stringDoubleMap serverVsTcpUdpTime;

	string userID;

	// flag to exit from the while loop when client says exit
	bool isExitRequest = false;

	while(!isExitRequest) {

		showCommandList();

		string stdinp;
		std::getline(std::cin, stdinp);
		int input = atoi(stdinp.c_str());

		switch(input) {

			case 1: {
						handleRegisterRequest(credential);
						break;
					}
			case 2: {
						// this means user is already logged in and he is changing the login. He should be asked to exit and relogin.
						if(!userID.empty()) {
							cout << "You are already logged in. Please exit and login again ..." << endl;
							break;
						}
						handleLoginRequest(credential, userID);
						break;
					}
			case 3: {
						// This means, even before logging in, file list is requested. This should not be allowed. 
						if(userID.empty()) {
							cout << "You should first login before doing anything  ... " << endl;
							break;
						}

						handleListRequest(credential, userID, serverVsFiles, serverVsTcpUdpTime); 
						break;
					}
			case 4: {
						// This means, even before logging in, file download is being requested. this should not be allowed.
						if(userID.empty()) {
							cout << "You should first login before doing anything  ... " << endl;
							break;
						}

						handleGetRequest(serverVsFiles);
						break;
					}
			case 5: {
						// This means, even before logging in, file download is being requested. this should not be allowed.
						if(userID.empty()) {
							cout << "You should first login before doing anything  ... " << endl;
							break;
						}

						handleWisegetRequest(credential, serverVsTcpUdpTime, serverVsFiles);
						break;
					}
			case 6: {
						isExitRequest = true;
						handleExitRequest(credential, userID);
						break;
					}
			default: {
						 // show the list of options again so that user gives you correct input.
						 cout << "Invalid input received. Please choose again ! " << endl;
						 showCommandList();
						 break;
					 }
		}
	}

	return 0;

}
