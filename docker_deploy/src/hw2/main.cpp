#include "functions.hpp"

int main(void) {    
    //init a Cache obj
    Cache my_cache(50);
    int id = 1; //user id counter

    while (true) {
        try {
            // Init server
            Server my_server("12345");
            cout << "Start listening" << endl;
            int user_fd = my_server.accept_request();
            pthread_t thread;
            pthread_mutex_lock(&mutex); 
            User *user = new User();
            user->set_fd(user_fd);
            user->set_id(id++);
            user->set_ip(my_server.get_client_ip());
            user->my_cache = &my_cache;
            pthread_mutex_unlock(&mutex);
            pthread_create(&thread, NULL, process_request, user);
        } catch(exception &e) {
            cout << "catched" << endl;
            pthread_mutex_lock(&mutex);
            outfile << id << ": " << e.what() << endl;
            pthread_mutex_unlock(&mutex);
        }
    }
    return EXIT_SUCCESS;
}