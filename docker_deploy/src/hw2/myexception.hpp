#ifndef MY_EXCEPTION_H
#define MY_EXCEPTION_H
#include <exception>
#include <iostream>
#include <string>

using namespace std;
class MyException : public exception
{
    string _msg;
public:
    MyException(const string& msg) : _msg(msg){}

    virtual const char* what()
    {
        return _msg.c_str();
    }
}; 
#endif