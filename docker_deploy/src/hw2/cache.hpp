#ifndef MY_CACHE_H
#define MY_CACHE_H
#include <unordered_map>
#include <list>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <ctime>
#include "parser.hpp"

using namespace std;

class Cache {
private:
    unordered_map<string, list<pair<string, vector<char> > >::iterator>mp;
    list<pair<string, vector<char> > >lru;
    int capacity_;
public:
    Cache() : capacity_(10) {}
    Cache(int capacity) : capacity_(capacity) {}
    
    vector<char> get(string key);
    void put(string key, vector<char> value);
    bool key_in_cache(string key);
    bool check_fresh(string key, ofstream &outfile, int user_id); 
};

#endif