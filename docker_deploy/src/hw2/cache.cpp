#include "cache.hpp"
#include <stdio.h>
#include <stdlib.h>
#include<iostream>
#include <vector>
#include <time.h>
#include <ctime>
// #include "parser.cpp"

using namespace std;

vector<char> Cache::get(string key) {
    //lock
    cout << "start to look for it " << endl;
    auto it = mp.find(key);
    cout << "find it" << endl;
    if (it == mp.end()) {
        return vector<char>(0);
    }
    lru.splice(lru.begin(), lru, it->second);
    //unlock
    return it->second->second;
}

void Cache::put(string key, vector<char> value) {
    //lock
    auto it = mp.find(key);
    if (it != mp.end()) {
        lru.splice(lru.begin(), lru, it->second);
        it->second->second = value;
        return;
    }
    if (lru.size() == capacity_) {
        mp.erase(lru.back().first);
        lru.pop_back();
    }
    lru.emplace_front(key, value);
    mp[key] = lru.begin();
}

bool Cache::check_fresh(string key, ofstream &outfile, int user_id){
    vector<char>cur_value = get(key);
    Parser parse;
    parse.parseHeader(false,cur_value);
    string cache_control = parse.parse_result["cache-control"];
    // If no cache-control field, it needs revalidation
    if (cache_control == "") {
        return false;
    } else {
        // Contain cache-control field, compare max-age with current age
        string max_age = parse.parse_result["max-age"];
        string expires = parse.parse_result["expire"];
        string last_modified = parse.parse_result["last-modified"];
        // If max-age doesn't exist
        if (max_age != "") {
            cout << "contain max-age" << endl;
            outfile << user_id << ": NOTE max-age: " << max_age << endl;
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

            time_t cur_age = time(NULL);
            struct tm *cur = gmtime(&cur_age);
            const char *cur_time_act = asctime(cur);
            cout << "current time: " << cur_time_act << endl;

            time_t expire_age = prev_age + max_age_int;
            struct tm *exp = gmtime(&expire_age);
            const char *expire_time_act = asctime(exp);
            cout << "expire time: " << expire_time_act << endl;
            if (cur_age <= expire_age) {
                cout << "it is fresh" << endl;
                return true;
            } else {
                // It expires, need revalidate
                cout << "it is not fresh" << endl;
                outfile << user_id << ": in cache, but expired at " << expire_time_act;
                return false;
            }
        } else if (expires != "") {
            cout << "contain expire" << endl;
            outfile << user_id << ": NOTE expire: " << expires << endl;
            struct tm expire_timeinfo;
            size_t gmt_place = expires.find("GMT");
            if (gmt_place != string::npos) {
                expires.erase(gmt_place - 1, 4);
            }
            strptime(expires.c_str(), "%a, %d %b %Y %H:%M:%S", &expire_timeinfo);
            time_t expire_time = mktime(&expire_timeinfo) - 18000;
            struct tm *exp = gmtime(&expire_time);
            const char *expire_time_act = asctime(exp);
            time_t cur_time = time(NULL);
            // If it doesn't expire
            if (cur_time <= expire_time) {
                cout << "it is fresh" << endl;
                return true;
            } else {
                // It expires, need revalidate
                cout << "it is not fresh" << endl;
                outfile << user_id << ": in cache, but expired at " << expire_time_act;
                return false;
            }
        } else if (last_modified != "") {
            cout << "contain last-modify" << endl;
            outfile << user_id << ": NOTE last-modified: " << last_modified << endl;
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
            double cur_diff = difftime(cur_age, date_age);

            if (cur_diff <= valid_diff) {
                // no need to revalidate
                cout << "it is fresh" << endl;
                return true;
            } else {
                // need revalidate
                cout << "it is not fresh" << endl;
                outfile << user_id << ": in cache, but expired at " << expire_time_act;
                return false;
            }
        }
        cout << "it is not fresh" << endl;
        return false;
    }
}

bool Cache::key_in_cache(string key) {
    vector<char> res = get(key);
    if (res.size() == 0) {
        return false;
    }
    return true;
}

// int main() {
//     Cache cache(2);
//     vector<char>info = {'1', '2'};
//     vector<char>info1 = {'3', '4'};
//     // vector<char>info2 = {'5', '6'};
//     cache.put("hi",info);
//     cache.put("hi1",info1);
//     // cache.put("hi2",info2);
//     vector<char>res = cache.get("hi");
//     if (res.size() == 0) {
//         cout << "NULL" << endl;
//     } else {
//         for (int i = 0; i < res.size(); i++) {
//             cout << res[i] << endl;
//         }
//     }
//     return 0;
// }