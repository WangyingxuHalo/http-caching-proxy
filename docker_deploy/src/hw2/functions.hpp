#include <fstream>
#include <iostream>
#include <vector>
#include <ctime>
#include <time.h>
#include <pthread.h>
#include "server.hpp"
#include "client.hpp"
#include "parser.hpp"
#include "cache.hpp"
#include "user.hpp"
#include "myexception.hpp"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
ofstream outfile("/var/log/erss/proxy.log");

string curr_time(){
    time_t curr = time(NULL);
    struct tm *cur = gmtime(&curr);
    const char *c = asctime(cur);
    string str(c);
    return str;
}

void print_expiry(int user_id, Parser &parse){
    string max_age = parse.parse_result["max-age"];
    string expires = parse.parse_result["expire"];
    string last_modified = parse.parse_result["last-modified"];
    if (max_age != "") {
        int max_age_int = stoi(max_age);
        // Format date string
        struct tm prev_timeinfo;
        string prev_date = parse.parse_result["date"];
        cout << "prev_data: " << prev_date << endl;
        size_t gmt_place = prev_date.find("GMT");
        if (gmt_place != string::npos) {
            prev_date.erase(gmt_place - 1, 4);
        }
        strptime(prev_date.c_str(), "%a, %d %b %Y %H:%M:%S", &prev_timeinfo);
        
        time_t prev_age = mktime(&prev_timeinfo) - 18000;
        // time_t rep_age = mktime(prev_date);
        struct tm *rep = gmtime(&prev_age);
        const char *rep_time_act = asctime(rep);
        cout << "response time: " << rep_time_act << endl;

        time_t expire_age = prev_age + max_age_int;
        struct tm *exp = gmtime(&expire_age);
        const char *expire_time_act = asctime(exp);
        outfile << user_id << ": cached, expires at " << expire_time_act;
    } else if (expires != "") {
        struct tm expire_timeinfo;
        size_t gmt_place = expires.find("GMT");
        if (gmt_place != string::npos) {
            expires.erase(gmt_place - 1, 4);
        }
        strptime(expires.c_str(), "%a, %d %b %Y %H:%M:%S", &expire_timeinfo);
        time_t expire_time = mktime(&expire_timeinfo) - 18000;
        struct tm *exp = gmtime(&expire_time);
        const char *expire_time_act = asctime(exp);
        outfile << user_id << ": cached, expires at " << expire_time_act;
    } else if (last_modified != "") {
        struct tm date_timeinfo;
        string res_date = parse.parse_result["date"];
        size_t gmt_place = res_date.find("GMT");
        if (gmt_place != string::npos) {
            res_date.erase(gmt_place - 1, 4);
        }
        strptime(res_date.c_str(), "%a, %d %b %Y %H:%M:%S", &date_timeinfo);
        time_t date_age = mktime(&date_timeinfo) - 18000;

        struct tm last_timeinfo;
        gmt_place = last_modified.find("GMT");
        if (gmt_place != string::npos) {
            last_modified.erase(gmt_place - 1, 4);
        }
        strptime(last_modified.c_str(), "%a, %d %b %Y %H:%M:%S", &last_timeinfo);
        time_t last_age = mktime(&last_timeinfo) - 18000;

        double valid_diff = difftime(date_age, last_age) / 10;

        time_t cur_age = time(NULL);
        time_t exp_age = cur_age + valid_diff;
        struct tm *exp = gmtime(&exp_age);
        const char *expire_time_act = asctime(exp);
        outfile << user_id << ": cached, expires at " << expire_time_act;
    }
}

void connect_with_server(int user_fd, int server_fd, int user_id) {
    send(user_fd, "HTTP/1.1 200 OK\r\n\r\n", 19, 0);
    pthread_mutex_lock(&mutex);
    outfile << user_id << ": Responding \"HTTP/1.1 200 OK\"" << endl;
    pthread_mutex_unlock(&mutex);
    cout << "sent successfully" << endl;
    fd_set readfds;
    int max_fd = max(user_fd, server_fd);
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(user_fd, &readfds);
        FD_SET(server_fd, &readfds);
        select(max_fd + 1, &readfds, NULL, NULL, NULL);
        int fds[2];
        fds[0] = user_fd;
        fds[1] = server_fd;
        int len = 0;
        int sendLen = 0;
        for (int i = 0 ; i < 2; i++) {
            vector<char>response(65536, 0);
            if (FD_ISSET(fds[i], &readfds)) {
                len = recv(fds[i], response.data(), response.size(), 0);
                if (len <= 0) {
                    return;
                }
                sendLen = send(fds[1 - i], response.data(), len, 0);
            }
        }
    }
}

void post_request(int user_fd, int server_fd, vector<char> &info, int user_id, Parser &parser, string host_name) {
    send(server_fd, info.data(), info.size(), 0);
    vector<char>response(65536, 0);
    int len = recv(server_fd, response.data(), response.size(), 0);
    pthread_mutex_lock(&mutex);
    outfile << user_id << ": Received \"" << parser.parse_result["First"] << "\" from" << host_name << endl;
    pthread_mutex_unlock(&mutex);
    send(user_fd, response.data(), len, 0);
}

void get_response(int server_fd, vector<char>&response, int user_id, string host_name) {
    // cout << "into get" << endl;
    int first_len = recv(server_fd, response.data(), response.size(), 0);
    // for (int i = 0; i < response.size(); i++) {
    //     cout << response[i];
    // }
    // cout << endl;
    // cout << "into next" << endl;
    // Parser response
    Parser parseR;
    parseR.parseHeader(false,response);

    pthread_mutex_lock(&mutex);
    outfile << user_id << ": Received \"" << parseR.parse_result["First"] << "\" from " << host_name << endl;
    pthread_mutex_unlock(&mutex);

    string resp = parseR.parse_result["Status"];
    string chunk_status = parseR.parse_result["transfer-encoding"];

    // If chunked  
    if (chunk_status == "chunked") {
        cout << "into chunked" << endl;
        int cur_index = first_len;
        string cur_str(response.begin(), response.end());
        // Constantly receive from server
        while (cur_str.find("0\r\n\r\n") == string::npos) {
            int len_msg = recv(server_fd, &response.data()[cur_index], 65536, 0);
            cur_index += len_msg;
            cur_str = "";
            cur_str.insert(cur_str.end(), response.begin(), response.end());
        }
        // send(user_fd, response.data(), response.size(), 0);
    } else {
        cout << "not chunked" << endl;
        // Not chunked, get content length, receive
        int cur_index = first_len;
        int message_len = -1;
        if (parseR.parse_result["content-length"] != "") {
            message_len = stoi(parseR.parse_result["content-length"]);
        }
        while (cur_index < message_len) {
            response.resize(cur_index + 65536);
            cout << "after resize, size: " << response.size() << endl;
            int len_recv = recv(server_fd, &response.data()[cur_index], 65536, 0);
            if (len_recv == 0 || len_recv == -1) {
                cout << "receive null" << endl;
                break;
            }
            cout << "receive size: " << len_recv << endl;
            cur_index += len_recv;
            cout << "cur_index: " << cur_index << endl;
        }
        // send(user_fd, response.data(), response.size(), 0);
    }
}

void get_request(int user_fd, int user_id, int server_fd, vector<char> &info, string key, Cache *my_cache, Parser &parser) {
    cout << "key: " << key << endl;
    bool in_cache = my_cache->key_in_cache(key);

    string host_name = parser.parse_result["Host"];

    // bool cache_result = my_cache.checkCache(key);
    // If it is not in cache, send request to server to request
    if (in_cache == false){
        cout << "not in cache" << endl;
        pthread_mutex_lock(&mutex);
        outfile << user_id << ": not in cache" << endl; 
        pthread_mutex_unlock(&mutex);
        // send(user_fd, cache_result.data(), cache_result.size(), 0);
        // return; 
        pthread_mutex_lock(&mutex);
        outfile << user_id << ": Requesting \"" << parser.parse_result["First"] << "\" from " << host_name << endl;
        pthread_mutex_unlock(&mutex);
        send(server_fd, info.data(), info.size(), 0);
        vector<char>response(65536, 0);
        
        get_response(server_fd, response, user_id, host_name);
        Parser parse1;
        parse1.parseHeader(false,response);
        if (parse1.parse_result["cacheable"]=="yes"){ //means we can cache
            cout << "put into cache" << endl;
            pthread_mutex_lock(&mutex);
            my_cache->put(key, response);
            pthread_mutex_unlock(&mutex);
            string cc = parse1.parse_result["cache-control"];
            if (!cc.empty()){
                if (cc.find("must-revalidate")!=-1){
                    pthread_mutex_lock(&mutex);
                    outfile << user_id << ": cached, but requires re-validation" << endl;
                    pthread_mutex_unlock(&mutex);
                }else{
                    pthread_mutex_lock(&mutex);
                    print_expiry(user_id, parse1);
                    pthread_mutex_unlock(&mutex);
                }
            }
        }else{
            pthread_mutex_lock(&mutex);
	        if (parse1.parse_result["cacheable"]==""){
		        outfile << user_id << ": not cacheable because cache control not specified" << endl;
	        }else{
            	outfile << user_id << ": not cacheable because " << parse1.parse_result["cacheable"] <<endl;
	        }
	        pthread_mutex_unlock(&mutex);
        }
        send(user_fd, response.data(), response.size(), 0);
        string first = parse1.parse_result["First"];
        pthread_mutex_lock(&mutex);
        outfile << user_id << ": Responding " << first << endl;
        pthread_mutex_unlock(&mutex);
    } else{
        // If in cache, check if it is fresh
        pthread_mutex_lock(&mutex);
        bool cache_fresh = my_cache->check_fresh(key, outfile, user_id);
        pthread_mutex_unlock(&mutex);
        pthread_mutex_lock(&mutex);
        vector<char>response = my_cache->get(key);
        pthread_mutex_unlock(&mutex);
        Parser parse1;
        parse1.parseHeader(false,response);
        // fresh, directly send to user
        if (cache_fresh == true) {
            cout << "it is fresh" << endl;
            pthread_mutex_lock(&mutex);
            outfile << user_id << ": in cache, valid"<< endl;
            pthread_mutex_unlock(&mutex);
            send(user_fd, response.data(), response.size(), 0);
            string first = parse1.parse_result["First"];
            pthread_mutex_lock(&mutex);
            outfile << user_id << ": Responding " << first << endl;
            pthread_mutex_unlock(&mutex);
        } else {
            cout << "data not fresh" << endl;
            pthread_mutex_lock(&mutex);
            outfile << user_id << ": in cache, requires validation" << endl;
            pthread_mutex_unlock(&mutex);
            // Not fresh, revalidate
            Parser parseR;
            parseR.parseHeader(false,response);
            string etag = parseR.parse_result["etag"];
            string last_modified = parseR.parse_result["last-modified"];
            string header = parser.getHeader();
            // Concat previous header with following information

            // If it contains etag, send if-none-match
            if (etag != "") {
                cout << "contain etag" << endl;
                pthread_mutex_lock(&mutex);
                outfile << user_id << ": NOTE ETag: " << etag << endl;
                pthread_mutex_unlock(&mutex);
                string new_header = header + "\r\n" + "If-None-Match: " + etag + "\r\n\r\n";
                vector<char>new_header_vector(new_header.begin(), new_header.end());
                cout << "etag: " << etag << endl;
                cout << "send If-None-Match" << endl;
                send(server_fd, new_header_vector.data(), new_header_vector.size(), 0);
                vector<char>new_response(65536, 0);
                get_response(server_fd, new_response, user_id, host_name);
                // parse this part
                Parser parseR1;
                parseR1.parseHeader(false,new_response);
                string res_code = parseR1.parse_result["Status"];
                // If code is 304, send to user from cache
                if (res_code == "304") {
                    cout << "code = 304" << endl;
                    send(user_fd, response.data(), response.size(), 0);
                    string first = parseR1.parse_result["First"];
                    pthread_mutex_lock(&mutex);
                    outfile << user_id << ": Responding " << first << endl;
                    pthread_mutex_unlock(&mutex);
                } else if (res_code == "200") {
                    cout << "code = 200" << endl;
                    // Else if it is 200, we update cache and send it back to user
                    // no-cache or private, no store
                    if (parseR1.parse_result["cacheable"]=="yes"){
                        cout << "cacheable" << endl;
                        cout << "put into cache" << endl;
                        pthread_mutex_lock(&mutex);
                        my_cache->put(key, new_response);
                        pthread_mutex_unlock(&mutex);
                        string cc = parseR1.parse_result["cache-control"];
                        if (!cc.empty()){
                            if (cc.find("must-revalidate")!=-1){
                                pthread_mutex_lock(&mutex);
                                outfile << user_id << ": cached, but requires re-validation" << endl;
                                cout << "cached, but requires re-validation" << endl;
                                pthread_mutex_unlock(&mutex);
                            }else{
                                pthread_mutex_lock(&mutex);
                                print_expiry(user_id, parseR1);
                                pthread_mutex_unlock(&mutex);
                            }
                        }
                    }else{
                        pthread_mutex_lock(&mutex);
	                    if (parseR1.parse_result["cacheable"]==""){
		                    outfile << user_id << ": not cacheable because cache control not specified" << endl;
	                    }else{
            	            outfile << user_id << ": not cacheable because " << parseR1.parse_result["cacheable"] <<endl;
	                    }
                        pthread_mutex_unlock(&mutex);
                    }
                    send(user_fd, new_response.data(), new_response.size(), 0);
                    cout << "send to user" << endl;
                    string first = parseR1.parse_result["First"];
                    pthread_mutex_lock(&mutex);
                    outfile << user_id << ": Responding " << first << endl;
                    pthread_mutex_unlock(&mutex);
                }
            } else if (last_modified != "") {
                cout << "contain last-modified" << endl;
                pthread_mutex_lock(&mutex);
                outfile << user_id << ": NOTE Last-Modified: " << last_modified << endl;
                pthread_mutex_unlock(&mutex);
                // If there's no etag and it has last-modify, send if-modified-since
                string new_header = header + "\r\n" + "If-Modified-Since: " + last_modified + "\r\n\r\n";
                vector<char>new_header_vector(new_header.begin(), new_header.end());
                cout << "last_modified: " << last_modified << endl;
                cout << "send If-Modified-Since" << endl;
                send(server_fd, new_header_vector.data(), new_header_vector.size(), 0);
                vector<char>new_response(65536, 0);
                get_response(server_fd, new_response, user_id, host_name);
                Parser parseR1;
                parseR1.parseHeader(false,new_response);
                string res_code = parseR1.parse_result["Status"];
                // If code is 304, send to user from cache
                if (res_code == "304") {
                    send(user_fd, response.data(), response.size(), 0);
                    string first = parseR1.parse_result["First"];
                    pthread_mutex_lock(&mutex);
                    outfile << user_id << ": Responding " << first << endl;
                    pthread_mutex_unlock(&mutex);
                } else if (res_code == "200") {
                    // Else if it is 200, we update cache and send it back to user
                    // no-cache or private, no store
                    if (parseR1.parse_result["cacheable"]=="yes"){
                        cout << "put into cache" << endl;
                        pthread_mutex_lock(&mutex);
                        my_cache->put(key, new_response);
                        pthread_mutex_unlock(&mutex);
                        string cc = parseR1.parse_result["cache-control"];
                        if (!cc.empty()){
                            if (cc.find("must-revalidate")!=-1){
                                pthread_mutex_lock(&mutex);
                                outfile << user_id << ": cached, but requires re-validation" << endl;
                                pthread_mutex_unlock(&mutex);
                            }else{
                                pthread_mutex_lock(&mutex);
                                print_expiry(user_id, parseR1);
                                pthread_mutex_unlock(&mutex);
                            }
                        }
                    }else{
                        pthread_mutex_lock(&mutex);
	                    if (parseR1.parse_result["cacheable"]==""){
		                    outfile << user_id << ": not cacheable because cache control not specified" << endl;
	                    }else{
            	            outfile << user_id << ": not cacheable because " << parseR1.parse_result["cacheable"] <<endl;
	                    }
                        pthread_mutex_unlock(&mutex);
                    }
                    send(user_fd, new_response.data(), new_response.size(), 0);
                    string first = parseR1.parse_result["First"];
                    pthread_mutex_lock(&mutex);
                    outfile << user_id << ": Responding " << first << endl;
                    pthread_mutex_unlock(&mutex);
                }
            } else {
                // No these fields, send request again
                cout << "not fresh, send again" << endl;
                send(server_fd, info.data(), info.size(), 0);
                vector<char>new_response(65536, 0);
                get_response(server_fd, new_response, user_id, host_name);
                Parser parseR1;
                parseR1.parseHeader(false,new_response);
                string res_code = parseR1.parse_result["Status"];
                // If code is 304, send to user from cache
                if (res_code == "304") {
                    send(user_fd, response.data(), response.size(), 0);
                    string first = parseR.parse_result["First"];
                    pthread_mutex_lock(&mutex);
                    outfile << user_id << ": Responding " << first << endl;
                    pthread_mutex_unlock(&mutex);
                } else if (res_code == "200") {
                    // Else if it is 200, we update cache and send it back to user
                    // no-cache or private, no store
                    if (parseR1.parse_result["cacheable"]=="yes"){
                        cout << "put into cache" << endl;
                        pthread_mutex_lock(&mutex);
                        my_cache->put(key, new_response);
                        pthread_mutex_unlock(&mutex);
                        string cc = parseR1.parse_result["cache-control"];
                        if (!cc.empty()){
                            if (cc.find("must-revalidate")!=-1){
                                pthread_mutex_lock(&mutex);
                                outfile << user_id << ": cached, but requires re-validation" << endl;
                                pthread_mutex_unlock(&mutex);
                            }else{
                                pthread_mutex_lock(&mutex);
                                print_expiry(user_id, parseR1);
                                pthread_mutex_unlock(&mutex);
                            }
                        }
                    }else{
                        pthread_mutex_lock(&mutex);
	                    if (parseR1.parse_result["cacheable"]==""){
		                    outfile << user_id << ": not cacheable because cache control not specified" << endl;
	                    }else{
            	            outfile << user_id << ": not cacheable because " << parseR1.parse_result["cacheable"] <<endl;
	                    }
                        pthread_mutex_unlock(&mutex);
                    }
                    send(user_fd, new_response.data(), new_response.size(), 0);
                    string first = parseR1.parse_result["First"];
                    pthread_mutex_lock(&mutex);
                    outfile << user_id << ": Responding " << first << endl;
                    pthread_mutex_unlock(&mutex);
                }
            }
        }
    }
}

string get_port(const char *method) {
    string port;
    if (strcmp(method, "CONNECT") == 0) {
        port = "443";
    } else {
        port = "80";
    }
    return port;
}

void *process_request(void *use) {
    User *user = (User*) use;
    int user_fd = user->get_fd();
    Cache *my_cache = user->my_cache;
    int user_id = user->get_id();
    // Get request information
    vector<char>info(65536, 0);
    // memset(info, 0, sizeof(info));
    int recv_len = recv(user_fd, info.data(), info.size(), 0);
    // cout << "Header Received: " << endl;
    // if (recv_len == 0 || recv_len == -1) {
    //     throw Exception("receive nothing for header");
    // }
    // for (int i = 0; i < info.size(); i++) {
    //     cout << info[i];
    // }
    // cout << endl;
    // cout << "User IP: " << endl;
    // cout << my_server.get_client_ip() << endl;

    // Use parser to interpret header
    Parser parser;
    parser.parseHeader(true,info);
    const char *host = parser.parse_result["Host"].c_str();
    string host_name = parser.parse_result["Host"];
    //cout << "Host:" << host <<endl;
    const char *method = parser.parse_result["Request"].c_str();
    string TO = parser.parse_result["TO"]; //get the url
    string first = parser.parse_result["First"];
    pthread_mutex_lock(&mutex);
    outfile << user_id << ": " << "\"" << first << "\"" << " from " << user->get_ip() << " @ " << curr_time();
    pthread_mutex_unlock(&mutex);
    // Get the port number. CONNECT->443, GET & POST ->80
    const char *port_str = get_port(method).c_str();

    // Initilize connection with server
    if (strlen(host) == 0) {
        return NULL;
    }
    Client my_client(port_str, host);
    int server_fd = my_client.get_server_fd();

    if (strcmp(method, "CONNECT") == 0) {
        connect_with_server(user_fd, server_fd, user_id);
        pthread_mutex_lock(&mutex);
        outfile << user_id << ": Tunnel closed" << endl; 
        pthread_mutex_unlock(&mutex);
    } else if (strcmp(method, "POST") == 0) {
        post_request(user_fd, server_fd, info, user_id, parser, host_name);
        pthread_mutex_lock(&mutex);
        outfile << user_id << ": Tunnel closed" << endl; 
        pthread_mutex_unlock(&mutex);
    } else if (strcmp(method, "GET") == 0) {
        get_request(user_fd, user_id, server_fd, info, TO, my_cache, parser); //also send the url as the key 
    } else {
        string res_header = "Responding \"HTTP/1.1 400 Bad Request\"";
        vector<char>send_str(res_header.begin(), res_header.end());
        send(user_fd, send_str.data(), send_str.size(), 0);
        pthread_mutex_lock(&mutex);
        outfile << user_id << ": " << res_header << endl;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}
