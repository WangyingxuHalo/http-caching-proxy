#ifndef MY_PARSER_H
#define MY_PARSER_H
#include <vector>
#include <map>
#include <string>

using namespace std;

class Parser{
public:
    vector<char> raw_header;
    bool req; //true if it is header from client, false if it is header from server
    map<string,string> parse_result;
    void parseHeader(bool header_type, vector<char> raw); //raw_header = raw; if client_header call parseClient, else call parseServer
    //vector<char> getRequested();
    map<string,string> getResult();
    string getHeader();
    bool isPrivate();

private:
    void parseRequest();
    void parseResponse(vector<char> raw_input);
};

#endif