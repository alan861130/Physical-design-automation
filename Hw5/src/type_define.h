#include <iostream>
#include <vector>
#include <cmath>

using namespace std;



class Pin{

    public:

        Pin(){};
        void set_coor( int x , int y );
        int x(void);
        int y(void);

        int x_pin;
        int y_pin;

};

void Pin::set_coor( int x , int y ){

    x_pin = x;
    y_pin = y;

}

int Pin::x(void){

    return x_pin;

}

int Pin::y(void){

    return y_pin;

}

class Route{

    public:
        Route(){}
        Route(int x , int y , int dir , float c){
            x_r = x;
            y_r = y;
            direction_r = dir;
            route_cost = c;
        }

        int x_r;
        int y_r;
        int direction_r;  // 0 for unknown
                          // 1 for up
                          // 2 for down
                          // 3 for left
                          // 4 for right
                          
        float route_cost;
        
};



class Net{

    public:
        Net( int x , int y );
        void show_detail(void);
        int net_length(void);

        int net_id;
        int pin_num; // 2 pin net
        Pin p1;
        Pin p2;

        vector<Route> routing_path;
        int overflow;

};

Net::Net( int x , int y ){
    net_id = x;
    pin_num = y;
    overflow = 0;
}

void Net::show_detail(void){

    cout<< "ID = " << net_id <<endl;
    cout<<" P1 : ( "<< p1.x() << " , " << p1.y()<<" )"<< endl;
    cout<<" P2 : ( "<< p2.x() << " , " << p2.y()<<" )"<< endl;

}

int Net::net_length(void){

    int length;
    length = abs( p1.x() - p2.x() ) + abs( p1.y() - p2.y());
    return length;
}

