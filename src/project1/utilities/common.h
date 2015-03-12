#ifndef __H__COMMON_H__
#define __H__COMMON_H__

#include <boost/tokenizer.hpp>

/**
  * Class to handle all the common logic for all clients, servers
  */

// response codes used by the server, client communication

namespace responsecodes {

	const std::string ERR_200 = "200 SUCCESS";
	const std::string ERR_400 = "400 INVALID COMMANDS";
	const std::string ERR_401 = "401 USERNAME_ALREADY_EXISTS";
	const std::string ERR_402 = "402 PASSWORD DOES NOT MATCH";
	const std::string ERR_403 = "403 INVALID USERNAME/PASSWORD";
	const std::string ERR_404 = "404 FILE NOT FOUND";
	const std::string ERR_405 = "405 INVALID ARGUMENTS";
	const std::string ERR_406 = "406 NOT LOGGED IN";
	const std::string ERR_407 = "407 PASSWORDS DO NOT MATCH";
	const std::string ERR_408 = "408 USER ALREADY LOGGED IN";
	const std::string ERR_409 = "409 INTERNAL SERVER ERROR";
	const std::string ERR_410 = "410 FILE TRANSFER FAILED";
	const std::string ERR_411 = "411 CLIENT INTERNAL ERROR";
	const std::string ERR_412 = "412 USER IS NOT LOGGED IN";
	const std::string ERR_414 = "414 SOCKET CLOSE FAILED";
	const std::string ERR_415 = "415 FILE NOT HOSTED BY SERVER";

};

using namespace boost;

class Common {
	Common();
	public:
	/**
	  * method to tokenize given input string based on the given delimiter
	  */
	static std::vector<std::string> tokenize(std::string &input, const char* delimiter) {
		char_separator<char> sep(delimiter);
		tokenizer<char_separator <char> > tok(input, sep);
		std::vector<std::string> tokens;
		for(tokenizer<char_separator <char> >::iterator beg=tok.begin(); beg!=tok.end(); ++beg){
			tokens.push_back(*beg);
		}
		return tokens;
	}

	/*
	 * method to tokenize based on the input string using \n as the delimiter
	 */
	static std::vector<std::string> tokenize(std::string &input) {
		tokenizer<> tok(input);
		std::vector<std::string> tokens;
		for(tokenizer<>::iterator beg=tok.begin(); beg!=tok.end(); ++beg){
			tokens.push_back(*beg);
		}
		return tokens;
	}

	/**
	  * Gets the absolute path of all the configuration, login related files used by client and server
	  */
	static std::string getFullPath(std::string& file) {
		std::string out;
		return out.append("/tmp/").append(file);
	}
};


#endif /* __H__COMMON_H__ */
