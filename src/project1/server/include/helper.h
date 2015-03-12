#include "./server.h"
#include "../../utilities/common.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;
using namespace responsecodes;

/**
  * Helper class to perform the common logic for the requests received by the client
  */
class Helper {
	
	Helper() {};	// constructor

	public:

	// Init method to initalize the server
	static Server initializeServer(int port, bool is_udp) {
		Server server(port, is_udp);
		server.initialize();
		if(!is_udp) {
			server.listen();
		}
		return server;
	};

	// Parser and a mapper which decodes the type of the request from the incoming request
	// this category is used by the server for further processing of the request
	static int getRequestCategory(std::string& request) {

		std::vector<std::string> toks = Common::tokenize(request, const_cast<const char *>(" "));

		if(toks.size() <= 0) {
			return -1;
		}

		if(0 == toks[0].compare("get")) {
			return 1; // 1 = category for get command
		} else if(0 == toks[0].compare("exit")) {
			return 2; // 2= category for exit command
		} else if(0 == toks[0].compare("probe")) {
			return 3; // 3= category for probe messages 
		}

		return -1;
	};

	// method to validate the request based on number of arguments expected
	// @ request : incoming request
	// @ error_code : error in case of any
	static bool validateRequest(std::string &request, std::string &error_code) {

		int c = Helper::getRequestCategory(request);
		bool ret = false;

		switch(c) {
			case 1: {
						// get should have 4 arguments <get> <file> <server> <mechanism>
						if(4 != Common::tokenize(request, const_cast<const char *>(" ")).size()) {
							error_code = ERR_405;
							ret = false;
						} else {
							ret = true;
						}
						break;
					}
			case 2: {	// exit should be just 1 token
						if(1 != Common::tokenize(request, const_cast<const char *>(" ")).size()) {
							error_code = ERR_405;
							ret = false;
						} else {
							ret = true;
						}
						break;
					}
			case 3: {	// its just a probe message
						ret = true;
						break;
					}
			default: {	// any other messages are not entertained
						 error_code = ERR_400;
						 ret = false;
					 }
		}
		return ret;
	};

};
