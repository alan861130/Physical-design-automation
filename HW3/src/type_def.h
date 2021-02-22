#include <iostream>
#include <string>
#include <vector>

using namespace std;

class hardblock{

public:
    hardblock(int id , int x , int y);
    void add_connected_net(int id);
    int area(void);


    int id_h;
    int x_h;
    int y_h;
    int x_length_h;
    int y_length_h;
    vector<int> connected_net_h;

    int parent;  //parent's id
    int left_child;  //left child's id
    int right_child;  //right child's id

    bool rotate;
    int rotate_round;
};

hardblock::hardblock(int id , int x , int y){

    id_h = id;
    x_length_h = x;
    y_length_h = y; 
    

    //cout<<name_h<<" "<<x_length_h<<" "<<y_length_h<<endl;

    //init parrent and child
    parent = -1;
    left_child = -1;
    right_child = -1;

    //rotate info
    rotate = 0;
      
}

void hardblock::add_connected_net(int id){
    
    connected_net_h.push_back(id);

}

int hardblock::area(void){
    return x_length_h * y_length_h;
}



class terminal{

public:
    terminal(int id);
    void add_connected_net(int id);
    void set_pos(int x, int y);

    int id_t;
    int x_t;
    int y_t;
    vector<int> connected_net_t;
    
};

terminal::terminal(int id){
    
    id_t = id;
    
}

void terminal::add_connected_net(int id){

    connected_net_t.push_back(id);

}

void terminal::set_pos(int x , int y){
    x_t = x;
    y_t = y;
    //cout<<x_t<<" "<<y_t<<endl;
}



class net{

public:
    net(int id);
    void add_connected_hardblock(int i);
    void add_connected_terminal(int i);

    int id_n;
    vector<int> connected_hardblock;
    vector<int> connected_terminal;
    
};

net::net(int id){

    id_n = id;

}

void net::add_connected_hardblock(int id){

    connected_hardblock.push_back(id);

}

void net::add_connected_terminal(int id){

    connected_terminal.push_back(id);

}



