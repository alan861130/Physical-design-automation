#include <iostream>
#include <cstring>
#include <vector>

using namespace std;

class Cell_node{
    friend class Net_node;

public:
    Cell_node(string name,int size) : cell_name(name), cell_size(size), cell_set(0), cell_pin(0), cell_gain(0),gain_change(0), lock(0){}

    string cell_name;
    int cell_size;
    int cell_set; //A = 0, B = 1
    int cell_pin;
    int cell_gain;
    int gain_change;
    int lock; //1 for lock, 0 for unlock

    vector<string> connected_net;

};

class Net_node{
    friend class Cell_node;

public:
    Net_node(string name) : net_name(name), A(0), B(0){}

    string net_name;
    int A ;
    int B ;

    vector<string> connected_cell;

};






