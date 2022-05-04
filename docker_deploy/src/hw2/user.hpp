#include <string>

class User{
private:
    int user_id;
    int user_fd;
    string user_ip;

public:
    Cache *my_cache;
    int get_id() { return user_id; }
    int get_fd() { return user_fd; }
    string get_ip() { return user_ip; }
    void set_id(int id) { user_id = id; }
    void set_fd(int fd) { user_fd = fd; }
    void set_ip(string ip) {user_ip = ip; } 
};