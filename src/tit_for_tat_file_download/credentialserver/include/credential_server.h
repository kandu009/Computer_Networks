#ifndef __H_CREDENTIAL_SERVER_H__
#define __H_CREDENTIAL_SERVER_H__

#include "../../server/include/socket.h"
#include <map>
#include <set>

class CredentialServer {

	public:
		// constructor		
		CredentialServer();
		
		// server socket related methods
		bool initialize();
		bool accept(Socket &client_socket);
		
		// data transmission methods
		bool send(std::string& data);
		bool receive(std::string& data);
		
		// register/login related methods
		bool validateClient(std::string &input, std::string &response);
		bool loginOrRegister(std::string &input, std::string &response);
		bool isUserLoggedIn(std::string &userID);
		bool userIdAlreadyExists(std::string& userID);

		// list related methods
		bool list(std::string &output, std::string &response);

		// exit related methods
		void exit(std::string& userID, std::string& response);

	private:
		// initialization of credential server helper methods
		void loadUserCredentials();
		void loadServerConfiguration();

	private:
		int m_port;															// server port
		Socket m_socket;													// socket on which server sends or receives data
		std::string m_credsFile;											// login.dat file location
		std::string m_configFile;											// server.cfg file location
		std::string m_host;													// host name on which server runs
		std::map<std::string, std::string> m_userIdPwd;						// map to store user credentials
		std::set<std::string> m_loggedInUsers;								// set of logged in users
		std::map<std::string, std::set<std::string> > m_serverVsFiles;		// list of content servers and its associated files
};

#endif /*__H_CREDENTIAL_SERVER_H__*/
