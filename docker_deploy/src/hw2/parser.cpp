#include <vector>
#include <map>
#include <sstream>
#include <string>
#include <iostream>


#include "parser.hpp"

using namespace std;

void Parser::parseHeader(bool header_type, vector<char> raw){
    req = header_type;
    raw_header = raw;
    if (req){
        parseRequest();
    }else{
        string s(raw.begin(),raw.end());
        string pattern1 = "\n\r\n";
        string pattern2 = "\n\n";
        string rst = "";
        size_t pos1 = s.find(pattern1);
        size_t pos2 = s.find(pattern2);
        if (pos1!=string::npos){
            rst = s.substr(0,pos1);
        }else if (pos2!=string::npos){
            rst = s.substr(0,pos2);
        }else{
            // cout << "different heading ending" << endl;
            exit(1);
        }
        vector<char> raw_input(rst.begin(),rst.end());
        parseResponse(raw_input);
    }
}

map<string,string> Parser::getResult(){
    return parse_result;
}

void Parser::parseRequest(){
    bool count = true;
    string key = "";
    string value = "";
    bool column = false;
    for (int i=0; i<raw_header.size();i++){
        if (i==0){
            char c = raw_header[0];
            if (c=='C'){
                value = "CONNECT";
                i = 7;
            }else if (c=='G'){
                value = "GET";
                i = 3;
            }else{ //request is POST
                value = "POST";
                i = 4;
            }
            parse_result["Request"] = value;
            value = "";
        }else{
            if (count){
                if (iswspace(raw_header[i])){
                    size_t split = value.find_last_of(":");
                    if (split == -1){
                        parse_result["TO"] = value;
                    }else{
                        if (split<value.size()-2 && value.substr(split+1,2)=="//"){
                            
                            parse_result["TO"] = value;
                        }else{
                            parse_result["TO"] = value.substr(0,split);
                        }
                    }
                    value = "";
                    count = false;
                    //get rid of HTTP/1.1
                    while (raw_header[i]!='\n'){
                        ++i;
                    }
                }else{

                    value+=raw_header[i];
                }
            }else{ //if passed the first line
                if (raw_header[i]=='\n') continue;
                if (raw_header[i]=='\r'){ //line end
                    parse_result[key]=value;
                    // cout << "key " <<key << "  value " <<value <<endl;
                    key = "";
                    value = "";
                    column = false;
                }else{
                    if (!column && raw_header[i]==':'){
                        column = true;
                        i++;
                    }else if (!column){
                        key += raw_header[i];
                    }else{ //column
                        value += raw_header[i];
                    }
                }
            }
        }
    }
    /*
    for (auto kv:parse_result){
        cout << kv.first << ":" <<kv.second <<endl;
    }
    */
    string v = parse_result["Host"];
    //cout << "v: " << parse_result["Host"] <<endl;
    size_t split = v.find_last_of(":");
    // cout << split <<endl;
    if (split==-1){
        parse_result["Host"] = v;
        // cout << "-1" <<endl;
    }else{
        parse_result["Host"] = v.substr(0,split);
        // cout << "-2" <<endl;
    }
    string rst = "";
    for (int i=0;i<raw_header.size();i++){
        if (raw_header[i]=='\r'){
            parse_result["First"] = rst;
            break;
        }else{
            rst+=raw_header[i];
        }
    }
}

void Parser::parseResponse(vector<char> raw_input){
    string key = "";
    string value = "";
    bool column = false;
    //bool  = false;
    bool firstLine = true;
    for (int i=0; i<raw_input.size()-2;i++){
        if (firstLine){
            if (i!=0 && isdigit(raw_input[i-1]) && isdigit(raw_input[i]) && isdigit(raw_input[i+1])){
                value+=raw_input[i-1];
                value+=raw_input[i];
                value+=raw_input[i+1];
                parse_result["Status"] = value;
                value = "";
                firstLine = false;
                while (raw_input[i]!='\n'){
                    ++i;
                }
            }else{
                continue;
            }
            /*
            if (!whitespace && !iswspace(raw_header[i])){
                continue;
            }else if (!whitespace && iswspace(raw_header[i]){
                whitespace = true;
            }else{
                if (isdigit(raw_header[i])){
                    value+=raw_header[i];
                }else{
                    parse_result["Status"] = value;
                    value = "";
                    firstLine = false;
                    while (raw_header[i]!='\n'){
                        ++i;
                    }
                }
            }
            */
        }else{
            if (raw_input[i]=='\n') continue;
            if (raw_input[i]=='\r'){ //line end
                parse_result[key]=value;
                key = "";
                value = "";
                column = false;
                //i++; //to '\n'
            }else{
                if (!column && raw_input[i]==':'){
                    column = true;
                    i++;
                }else if (!column){
                    if (raw_input[i]=='\n') continue;
                    if (isalpha(raw_input[i])){
                        key += tolower(raw_input[i]);
                    }else{
                        key += raw_input[i];
                    }
                    
                }else{ //column
                    if (raw_input[i]=='\n') continue;
                    value += raw_input[i];
                }
                
            }
        }
    }
    string s = parse_result["cache-control"];
    if (!s.empty()){
        size_t pos = s.find("max-age=");
        if (pos!=-1){
            pos+=8;
            string v = "";
            while (isdigit(s[pos])){
                v+=s[pos];
                pos++;
            }
            parse_result["max-age"] = v;
        }
        size_t ip = s.find("no-store");
        size_t pri = s.find("private");
        if (ip!=-1 || pri!=-1){
            if (ip!=-1){
                parse_result["cacheable"]  = "no-store";
            }else{
                parse_result["cacheable"] = "private";
            }
        }else{
            parse_result["cacheable"] = "yes";
        }
    }
    string rst = "";
    for (int i=0;i<raw_header.size();i++){
        if (raw_header[i]=='\r'){
            parse_result["First"] = rst;
            break;
        }else{
            rst+=raw_header[i];
        }
    }   
}

string Parser::getHeader() {
    string request(raw_header.begin(), raw_header.end());
    size_t index = request.find("\r\n\r\n");
    return request.substr(0, index);
}


// int main(){
//     string test = "CONNECT www.google.com:443 HTTP/1.1\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0\r\nProxy-Connection: keep-alive\r\nConnection: keep-alive\r\nHost: www.google.com:443\r\n\r\n";
//     vector<char> raw(test.begin(),test.end());
    
//     string test2 = "200 OK\r\nAccess-Control-Allow-Origin: *\r\nConnection: Keep-Alive\r\nContent-Encoding: gzip\r\nContent-Type: text/html; charset=utf-8\r\nDate: Mon, 18 Jul 2016 16:06:00 GMT\r\nEtag: \"c561c68d0ba92bbeb8b0f612a9199f722e3a621a\"\r\nCache-Control: max-age=1\r\nKeep-Alive: timeout=5, max=997\r\nLast-Modified: Mon, 18 Jul 2016 02:36:04 GMT\r\nServer: Apache\r\nSet-Cookie: mykey=myvalue; expires=Mon, 17-Jul-2017 16:06:00 GMT; Max-Age=31449600; Path=/; secure\r\n\r\n";
// /*
// Transfer-Encoding: chunked
// Vary: Cookie, Accept-Encoding
// X-Backend-Server: developer2.webapp.scl3.mozilla.com
// X-Cache-Info: not cacheable; meta data too large
// X-kuma-revision: 1085259
// x-frame-options: DENY
// ";
// */

//     vector<char> raw2(test2.begin(),test2.end());
//     Parser parser;
//     parser.parseHeader(true,raw);
//     for (auto kv : parser.parse_result){
//         cout<< kv.first << ": " << kv.second << endl;
//     }
//     cout << "test" <<endl;
//     return 0;
// }
