#include <boost/tokenizer.hpp>
#include <iostream>
#include <fstream>
#include <string.h>
#include <set>
#include "../include/credential_server.h"
#include "../../utilities/common.h"

using namespace std;
using namespace responsecodes;
using namespace boost;

/*
 * @m_port 		: port on which the credential server runs
 * @m_credsFile 	: login.dat file which keeps a record of all the registered user credentials for authentication
 * @m_configFile	: config file which keeps all the server related configuration information
 */
CredentialServer::CredentialServer(): m_credsFile("login.dat"), m_configFile("credential.cfg") {
	// initialize the user credentials on load
	char buf[32];
	gethostname(buf,sizeof buf);
	m_host = std::string(buf).append(".cselabs.umn.edu");

	loadUserCredentials();
	loadServerConfiguration();
}

/*
 * method to load all the user creadentials from login.dat file
 */
void CredentialServer::loadUserCredentials() {

	ifstream myfile;
	myfile.open(Common::getFullPath(m_credsFile).c_str());
	string line;

	if(!myfile.is_open()) {
		cout << "credential server log:: warn:: could not load the login.dat file" << endl;
		return;
	}
	while(getline (myfile,line)) {
		// split each line by space in the format [ <username> <password> ]
		std::vector<std::string> toks = Common::tokenize(line, const_cast<const char *>(" "));
		if(2 != toks.size()) {
			continue; // this means an invalid entry
		}
		m_userIdPwd.insert(std::pair<std::string, std::string>(toks.at(0), toks.at(1)));
	}
	myfile.close();
}

/*
 * load the config parameters
 * load the server port mentioned as a key value pair <port=<value>>
 * load the list of content servers given as <servers=server1,file1,file2|server2,file2,file3>
 */
void CredentialServer::loadServerConfiguration() {

	ifstream myfile;
	myfile.open(Common::getFullPath(m_configFile).c_str());
	string line;

	if(!myfile.is_open()) {
		cout << "credential server log:: warn:: could not load the config file" << endl;
		return;
	}

	while(getline(myfile, line)) {

		if(line.find("port=") == 0) { // extract port information

			std::vector<std::string> toks = Common::tokenize(line, const_cast<const char *>("="));
			m_port = atoi(toks[1].c_str());

		} else if(line.find("servers=") == 0) { // extract the list of content servers

			std::vector<std::string> servertoks = Common::tokenize(line, const_cast<const char *>("="));
			if(servertoks.size() < 2) {
				continue;
			}

			std::vector<std::string> toks = Common::tokenize(servertoks[1], const_cast<const char *>("|"));
			if(1 > toks.size()) {
				continue;   // if there is no correct information, keep searching
			}

			for(int i = 0; i < toks.size(); ++i) {

				std::vector<std::string> filetoks = Common::tokenize(toks[i], const_cast<const char *>(","));
				if(2 > filetoks.size()) {
					continue;   // there should be atleast one file available at each server, else ignore
				}
				std::set<std::string> fileList;
				for(int j = 1; j < filetoks.size(); ++j) {
					fileList.insert(filetoks[j]);
				}
				m_serverVsFiles.insert(std::pair<std::string, std::set<std::string> >(filetoks[0], fileList));
			}
		} 
		else {
			continue;
		}
	}
	myfile.close();
}

/*
 * method to initialize the server i.e., create, bind and listen on a socket
 */
bool CredentialServer::initialize() {

	if(!m_socket.create()) {
		cout << "credential server log:: error:: Could not create a socket" << endl;
		return false;
	}
	if(!m_socket.bind(m_host, m_port)) {
		cout << "credential server log:: error:: Could not bind to port [ " << m_port << " ]" << endl;
		return false;
	}
	if(!m_socket.listen()) {
		cout << "credential server log:: error:: Could not listen on socket." << endl;
		return false;
	}
	return true;
}

/*
 * method to accept the client connection
 * @client_socket 	: socket on which client request is accepted and served later
 */
bool CredentialServer::accept(Socket &client_socket) {
	if(!m_socket.accept(client_socket)) {
		cout << "credential server log:: error:: Could not accept client request ... " << endl;
		return false;
	}
	return true;
}

/*
 * method to send data to the client
 * @data	: data to be sent
 */
bool CredentialServer::send(std::string& data) {
	if(!m_socket.send(data)) {
		cout << "credential server log:: error:: Could not send message ... [ " << data << " ]" << endl;
		return false;
	}
	return true;
}

/*
 * method to receive data from client
 * @data    : data received
 */
bool CredentialServer::receive(std::string& data) {
	if(!m_socket.recv(data)) {
		cout << "credential server log:: error:: Could not recieve message properly ... [ " << data << " ]" << endl;
		return false;
	}
	return true;
}

/*
 * method to check if a user is already logged in
 * @userID	: user ID of the client 
 */
bool CredentialServer::isUserLoggedIn(std::string &userID) {
	return (m_loggedInUsers.find(userID) != m_loggedInUsers.end());
}

/*
 * method to check if a user already exists with the given userID
 * @userID  : user ID of the client
 */
bool CredentialServer::userIdAlreadyExists(std::string& userID) {
	return (m_userIdPwd.find(userID) != m_userIdPwd.end());
}

/*
 * method to validate client request 
 * evaluates the number of arguments based on the request type
 *     1. register 	: it should be 4 arguments 'register <user> <password> <retype password>'
 *     2. login		: it should be 3 arguments 'login <user> <password>'
 * checks for authentication for login i.e., checks if password is correct or not
 * evaluates the register request i.e., checks if password and retype passwords match * 
 * 
 * @input		: request for logging in or registering
 * @response	: response of the server to be sent to client
 */
bool CredentialServer::validateClient(std::string &input, std::string &response) {

	if(input.empty()) {
		response = ERR_405; 
		return false;
	}

	std::vector<std::string> tokens = Common::tokenize(input);	

	if(4 == tokens.size() && "register" == tokens.at(0)) {
		//check if the userID already exists
		if(userIdAlreadyExists(tokens.at(1))) {
			response = ERR_401;
			return false;
		}
		//check if password and retype passwords match
		if(0 != tokens.at(2).compare(tokens.at(3))) {
			response = ERR_402;
			return false;
		}
		response = ERR_200;
		return true;

	} else if(3 == tokens.size() && "login" == tokens.at(0)) {

		//check if the user is already logged in
		if(isUserLoggedIn(tokens.at(1))) {
			response = ERR_408;
			return false;
		}

		std::map<string, string>::iterator it = m_userIdPwd.find(tokens.at(1));
		if(it == m_userIdPwd.end()) {
			response = ERR_403;
			return false;
		}

		if(0 != it->second.compare(tokens.at(2))) {
			response = ERR_403;
			return false;
		}

		response = ERR_200;
		return true;
	}

	response = ERR_405;
	return false;
}

/*
 * method to register/login a user
 * if register, adds the client details to login.dat file
 * if login, add the userID to logged in users data structure
 *
 * @input		: request for logging in or registering
 * @response	: response of the server to be sent to client
 */
bool CredentialServer::loginOrRegister(std::string &input, std::string &output) {

	std::vector<std::string> tokens = Common::tokenize(input);

	if(0 == tokens.at(0).compare("register")) {
		m_userIdPwd.insert(std::pair<std::string, std::string>(tokens.at(1), tokens.at(2)));
		// update it to the login.dat file	
		ofstream myfile;
		myfile.open(Common::getFullPath(m_credsFile).c_str(), std::ios_base::app);
		if(!myfile.is_open()) {
			output = ERR_409;
			return false;
		}
		myfile << tokens.at(1);
		myfile << " ";
		myfile << tokens.at(2);
		myfile << "\n";
		myfile.close();

	} else {
		// this will definetly be 'login' as validated earlier.
		m_loggedInUsers.insert(tokens.at(1));
	}

	output == ERR_200;
	return true;

}

/*
 * method used to list all the content servers and the files associated with each of them for downloading
 *
 * @output		: output with all the list of content servers and its associated files 
 * @response	: response code of the operation to be sent to client
 */
bool CredentialServer::list(std::string &output, std::string &response) {
	for(std::map<std::string, std::set<std::string> >::iterator it = m_serverVsFiles.begin(); it != m_serverVsFiles.end(); ++it) {

		output.append(it->first);
		for(std::set<std::string>::iterator iter = it->second.begin(); iter != it->second.end(); ++iter) {
			output.append(" ").append(*iter);
		}
		output.append("|");
	}
	response = ERR_200;
	return true;
}

/*
 * method to handle the exit request sent by a client
 * @ input 		: the command sent by client
 * @ response 	: server response back to the client
 *
 */
void CredentialServer::exit(std::string& input, std::string& response) {

	std::vector<std::string> toks = Common::tokenize(input, const_cast<const char *>(" "));
	// incorrect exit command encountered
	if(2 != toks.size()) {
		response.append(ERR_400).append("Exit command is incorrect!");
		return;
	}
	// remove the user from loggedin users and send back the response
	string userID = toks[1];
	if(isUserLoggedIn(userID)) {
		m_loggedInUsers.erase(userID);
		response.append(ERR_200);
		cout << "Logged out the user [ " << userID << " ] " << endl;
	} else {
		// if the user is not already logged in, warn the client
		response.append(ERR_412);
	}
}
