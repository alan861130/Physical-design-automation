#include <iostream>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <algorithm>
#include <queue>
#include <cmath>
#include <time.h>

#include "type_define.h"

using namespace std;

//*********************************************************//

void read_file(char datapath[]);
void set_random_seed(char datapath[]);
int myrandom(int i){ 
    return rand() %i; 
}

void parameter_init(void);
void history_cost_init(void);

void first_route(int id);
Route propagation( Route cur , int direction);
float calculate_cost( int x , int y , int dir); // dir = 0 for vertical, dir = 1 for horizontal

int calculate_overflow(void);

void second_route(void);
void get_overflow_route(void);
void rip_up(Route node);
void reroute(int ID);


void output(char datapath[]);
//*********************************************************//

vector<Net> Net_v;
int random_seed;
int overflow;

// initial parameters
int grid_x;
int grid_y;
int vertical_capacity;
int horizontal_capacity;
int net_num;

// global routing parameters
int **vertical_utilize_edge;
int **horizontal_utilize_edge;

int **vertical_overflow;
int **horizontal_overflow;

// A* searching parameters
int **parent;
int **map_cost;
int **label;

// history cost
float **vertical_history_cost;
float **horizontal_history_cost;

//*********************************************************//
// comparing cost for routing queue
struct mycmp{

    bool operator()(Route const a, Route const b){
        return a.route_cost > b.route_cost;
    }
};

// sort Net_v from 0 to num_net - 1 
bool sort_id(Net a, Net b) {
    return a.net_id < b.net_id; 
}

// compare overflow of route
struct sort_overflow{
    int x, y;
	bool operator()(Route a, Route b){
		
		if(a.direction_r == 1 || a.direction_r == 2){
			x = vertical_overflow[a.x_r][a.y_r];
		}
        else{
			x = horizontal_overflow[a.x_r][a.y_r];
		}
		if(b.direction_r == 1 || b.direction_r == 2){
			y = vertical_overflow[b.x_r][b.y_r];
		}
        else{
			y = horizontal_overflow[b.x_r][b.y_r];
		}
		return x < y;
	}
};

// compare distance of net
struct sort_distance{
    bool operator()(int a, int b){
		int x = Net_v[a].overflow;
		int y = Net_v[b].overflow;
        if(x != y)
            return y > x;
        else
			return Net_v[b].net_length() < Net_v[a].net_length();		
    }
};

//*********************************************************//

// record of net 
vector<vector<vector<int> > > VIDList, HIDList;

// queue for overflow route
priority_queue <Route, vector<Route>, sort_overflow> overflow_queue;

// queue for reroute needed route
priority_queue <int, vector<int>, sort_distance> reroute_queue;

//*********************************************************//
clock_t c1,c2,c3,c4,c5;

//*********************************************************//

int main(int argc, char *argv[]){

    c1 = clock();

    read_file(argv[1]);

    c2 = clock();

    set_random_seed(argv[1]);
    random_shuffle( Net_v.begin() , Net_v.end() , myrandom);

    parameter_init();
    history_cost_init();

    c3 = clock();

    for(int i = 0 ; i < net_num ; i++){
        // route the random stored nets
        // only route in the bounding box
        //cout<< "routing "<< i << " net..."<<endl;
        first_route(i);
    }

    
    cout<<"**************************************************************"<<endl;
    // sort nets from 0 to num_net - 1
    sort( Net_v.begin() , Net_v.end() , sort_id);

    overflow = calculate_overflow();
    cout<< "initial overflow : "<<overflow<<endl;

    int wirelength = 0;
    for(int i = 0 ; i < net_num ; i++){
        wirelength += Net_v[i].net_length();
    }
    cout<<"Total wirelength : " <<wirelength<<endl;

    c4 = clock();
    //second_route();

    output(argv[2]);

    c5 = clock();

    cout<<"**************************************************************"<<endl;
    cout<<"I/O time : "<< ( (double)(c2 - c1) + (double)(c5 - c4) )/ CLOCKS_PER_SEC <<endl;
    cout<<"Set up parameters time : "<<  (double)( c3 - c2 ) / CLOCKS_PER_SEC <<endl;
    cout<<"Routing time : "<< (double)( c4 - c3 ) / CLOCKS_PER_SEC <<endl;
    
    return 0;
}

void read_file(char datapath[]){

    ifstream fin(datapath);
    
    // read parameters
    string temp;
    fin >> temp >> grid_x >> grid_y;
    fin >> temp >> temp >> vertical_capacity;
    fin >> temp >> temp >> horizontal_capacity;
    fin >> temp >> temp >> net_num;

    // read nets and pins
    int p , x , y;
    for(int i = 0 ; i < net_num ; i++){

        fin >> temp >> temp >> p;
        Net n(i , p);

        fin >> x >> y;
        n.p1.set_coor(x , y);

        fin >> x >> y;
        n.p2.set_coor(x , y);

        Net_v.push_back(n);

    }
    
    // show info
    cout<< "Grid : ( "<< grid_x << " , " << grid_y << " )"<<endl;
    cout<< "Vertical capacity : " << vertical_capacity <<endl;
    cout<< "Horizontal capacity : "<< horizontal_capacity <<endl;
    cout<< "Num of net : " << net_num <<endl; 
    //for(int i = 0 ; i < net_num ; i++){
    //    Net_v[i].show_detail();
    //}
}

void set_random_seed(char datapath[]){
    
    random_seed = time(NULL);
    srand(random_seed);

}

void parameter_init(void){
    
    //cout<<"initialize parameters..."<<endl;
    // set up edge utilization
    vertical_utilize_edge = new int*[grid_x];
    horizontal_utilize_edge = new int*[grid_x];

    // set up overflow
    vertical_overflow = new int*[grid_x];
    horizontal_overflow = new int*[grid_x];

    //set up parent
    parent = new int*[grid_x];

    // set up map cost
    map_cost = new int*[grid_x];

    // set up label
    label = new int*[grid_x];

    // set up size of VIDlist and HIDlsit
    VIDList.resize(grid_x);
    HIDList.resize(grid_x);

    for(int i = 0 ; i < grid_x ; i++){
        //cout<<i<<endl;
        vertical_utilize_edge[i] = new int[grid_y]();
        horizontal_utilize_edge[i] = new int[grid_y]();

        vertical_overflow[i] = new int[grid_y]();
        horizontal_overflow[i] = new int[grid_y]();

        parent[i] = new int[grid_y]();
        map_cost[i] = new int[grid_y]();
        label[i] = new int[grid_y]();

        VIDList[i].resize(grid_y);
        HIDList[i].resize(grid_y);

        for(int j = 0 ; j < grid_y ; j++){
            // initialize the utilize edge 
            vertical_utilize_edge[i][j] = 0;
            horizontal_utilize_edge[i][j] = 0;
        }
    }

}

void history_cost_init(void){
    //cout<<"initialize history cost..."<<endl;
    vertical_history_cost = new float*[grid_x];
    horizontal_history_cost = new float*[grid_y];

    for( int i = 0 ; i < grid_x ; i++){
        vertical_history_cost[i] = new float[grid_y]();
        horizontal_history_cost[i] = new float[grid_y]();

        // initialize the history cost
        for(int j = 0 ; j < grid_y ; j++){
            vertical_history_cost[i][j] = 1;
            horizontal_history_cost[i][j] = 1;
        }

    }
}

//*********************************************************//

void first_route(int id){

    // clear parameters for A* searching
    for(int i = 0 ; i < grid_x ; i++){
        for(int j = 0 ; j < grid_y ; j++){
            // set to -1 if haven't been calculate

            parent[i][j] = -1;
            map_cost[i][j] = -1;
            label[i][j] = 0;
        }
    }

    // create a sorted queues for sub route
    priority_queue< Route , vector<Route> , mycmp> Route_queue;

    // create two route for source and target
    Net n = Net_v[id];
    Pin s = n.p1;
    Pin t = n.p2;

    Route Source( s.x() , s.y() , 0 , 0);
    Route Target( t.x() , t.y() , 0 , 0);
    Route_queue.push(Source);

    //cout<< Net_v[id].net_id << endl;
    //cout<<" source : " << s.x() << " " << s.y() << endl;
    //cout<<" target : " << t.x() << " " << t.y() << endl;

    //cout<<"*********************************************************"<<endl;
    //cout<<"routing in the bounding box...";
    while( Route_queue.size() != 0){

        Route current = Route_queue.top();  // the route with smallest cost
        Route_queue.pop(); // delete from the queue

        
        if( current.x_r == Target.x_r && current.y_r == Target.y_r){

            // current step reach the target
            //cout<< " reach target" <<endl;
            break;
        }

        // generate 4 direction route from current
        for(int i = 1 ; i <= 4 ; i++){
            //cout<< "direction : "<<i;
            // remove route which is out of the bounding box
            
            if(current.x_r > Target.x_r){
                if(i == 4){
                    continue;
                }
            }
            else if( current.x_r < Target.x_r){
                if( i == 3){
                    continue;
                }
            }
            else{
                if( i == 3 || i == 4){
                    continue;
                }
            }

            if(current.y_r > Target.y_r){
                if(i == 1){
                    continue;
                }
            }
            else if( current.y_r < Target.y_r){
                if(i == 2){
                    continue;
                }
            }
            else{
                if(i == 1 || i == 2){
                    continue;
                }
            }
            /*
            if(i == 1 ){
                if(Target.y_r < current.y_r || current.y_r + 1 > grid_y){
                    continue;
                }
            }
            else if(i == 2){
                if(Target.y_r > current.y_r || current.y_r - 1 < 0){
                    continue;
                }
                
            }
            else if(i == 3){
                if(Target.x_r > current.x_r || current.x_r - 1 < 0){
                    continue;
                }
                
            }
            else if(i == 4){
                if(Target.x_r < current.x_r || current.x_r + 1 > grid_x){
                    continue;
                }
                
            }*/

            //cout<<" calculate next route"<<endl;
            Route next = propagation( current , i );

            // setup label
            // current label = prev label + 1
            if(i == 1){
                label[next.x_r][next.y_r] = label[next.x_r][next.y_r - 1] + 1;
            }

            else if (i == 2){
                label[next.x_r][next.y_r] = label[next.x_r][next.y_r + 1] + 1;
            }

            else if(i == 3){
                label[next.x_r][next.y_r] = label[next.x_r + 1][next.y_r] + 1;
            }

            else if(i == 4){
                label[next.x_r][next.y_r] = label[next.x_r - 1][next.y_r] + 1;
            }

            if(map_cost[next.x_r][next.y_r] != -1 && next.route_cost >= map_cost[next.x_r][next.y_r]){
                // not a good route solution
                //cout<< " bad solution" <<endl;
                continue;
            }
            else{
                // good route solution
                // update and store
                //cout<<" good solution"<<endl;
                map_cost[next.x_r][next.y_r] = next.route_cost;
                parent[next.x_r][next.y_r] = i;
                
                //cout<<"next : "<<next.x_r<<" "<<next.y_r<<" "<<i<<endl;
                
                // if a good solution appear, stop search other solution
                i = 5;

                Route_queue.push(next);
                //Net_v[id].routing_path.push_back(next);
            }

        }

    }
    
    // print direction of parent
    /*for (int i = 0 ; i < grid_x ; i++){
        for(int j = 0 ; j < grid_y ; j++){
            if(parent[i][j] != -1){
                cout<<i<<" "<<j<<" "<<parent[i][j]<<" "<<endl;
            }   
        }
    }*/

    //cout<<"*********************************************************"<<endl;
    //cout<<"trace back...";
    Route node( t.x() , t.y() , 0 , 0);
    while(1){

        Net_v[id].routing_path.push_back(node);

        if(node.x_r == s.x() && node.y_r == s.y()){
            // reach the source
            //cout<<" reach source"<<endl;
            break;
        }

        int parent_direction = parent[node.x_r][node.y_r];
        if( parent_direction == 1){
            // up
            
            horizontal_utilize_edge[node.x_r][node.y_r]++;
            HIDList[node.x_r][node.y_r].push_back(Net_v[id].net_id);
            node.y_r --;
        }
        else if( parent_direction == 2){
            // down 
            horizontal_utilize_edge[node.x_r][node.y_r]++;
            HIDList[node.x_r][node.y_r].push_back(Net_v[id].net_id);
            node.y_r ++;

        }
        else if( parent_direction == 3){
            // left
            vertical_utilize_edge[node.x_r][node.y_r]++;
            VIDList[node.x_r][node.y_r].push_back(Net_v[id].net_id);
            node.x_r ++;
        }
        else if( parent_direction == 4){
            // right
            
            vertical_utilize_edge[node.x_r][node.y_r]++;
            VIDList[node.x_r][node.y_r].push_back(Net_v[id].net_id);
            node.x_r --;
        }
        //cout<< "parnet : "<< node.x_r <<" "<<node.y_r<<endl;

    }
     

    
}

Route propagation( Route cur , int direction){

    Route temp = cur;
    temp.direction_r = direction;

    // up
    if(direction == 1){
        temp.y_r ++;  // change the y coor
    }

    //down
    else if(direction == 2){
        temp.y_r --;  // change the y coor
    }

    // left
    else if(direction == 3){
        temp.x_r --;  // change the y coor
    }

    // right
    else if(direction == 4){
        temp.x_r ++;  // change the y coor
    }

    //cout<<"add cost"<<endl;
    // add the cost of new step
    if( direction == 1 || direction == 2){
        // vertical
        temp.route_cost = temp.route_cost + calculate_cost(temp.x_r , temp.y_r , 0);
    }
    else{
        // horizontal
        temp.route_cost = temp.route_cost + calculate_cost(temp.x_r , temp.y_r , 1);
    }
    

    return temp;
}

float calculate_cost( int x , int y , int dir){

    float history_cost = 0;
    float congest_penalty = 0;
    //cout<<x<<" "<<y<<endl;
    if(dir == 0){
        // vertical
        //cout<<"vertical"<<endl;
        
        history_cost = vertical_history_cost[x][y];
        congest_penalty = (float)(pow((vertical_utilize_edge[x][y] + 1.0) / vertical_capacity, 5));
    }

    else{
        // horizontal
        //cout<<"horizontal"<<endl;
        
        history_cost = horizontal_history_cost[x][y];
        congest_penalty = (float)(pow((horizontal_utilize_edge[x][y] + 1.0) / horizontal_capacity, 5));
    }
    //cout<<" history cost : "<<history_cost<<endl;
    //cout<<" congest penalty : "<<congest_penalty<<endl;
    return history_cost * congest_penalty;

}

int calculate_overflow(void){
    //cout<<"calculate overflow..."<<endl;
    //cout<<"vertical capacity : "<< vertical_capacity <<endl;
    //cout<<"horizontal capacity : "<< horizontal_capacity <<endl;

    int count = 0;
    // vertical
    for( int i = 0; i < grid_x ; i++){
        for(int j = 0 ; j < grid_y ; j++){
            //cout<<vertical_utilize_edge[i][j];
            if( vertical_utilize_edge[i][j] > vertical_capacity){
                vertical_overflow[i][j] = vertical_utilize_edge[i][j] - vertical_capacity;
            }
            else{
                vertical_overflow[i][j] = 0;
            }
            //vertical_overflow[i][j] = max( vertical_utilize_edge[i][j] - vertical_capacity , 0); 

            count += vertical_overflow[i][j];
        }
    }

    // horizontal
    for( int i = 0 ; i < grid_x ; i++){
        for(int j = 0 ; j < grid_y ; j++){
            //cout<<horizontal_utilize_edge[i][j];
            if(horizontal_utilize_edge[i][j] > horizontal_capacity){
                horizontal_overflow[i][j] = horizontal_utilize_edge[i][j] - horizontal_capacity;
            }
            else{
                horizontal_overflow[i][j] = 0;
            }
            //horizontal_overflow[i][j] = max( horizontal_utilize_edge[i][j] - horizontal_capacity , 0);
            count += horizontal_overflow[i][j];
        }
    }

    return count;

}

//*********************************************************//

void second_route(void){
    // reroute the overflow route
    int round = 0;
    int max_round = 1;
    while(round < max_round){
        round ++;

        // find out overflow route
        get_overflow_route();

        while( overflow_queue.size() != 0){

            Route n = overflow_queue.top(); // take the route with biggest overflow
            overflow_queue.pop();

            rip_up(n);

        }
        



        overflow = calculate_overflow();
        if(overflow == 0){
            break;
        }
    }
}

void get_overflow_route(void){

    //cout<<"get overflow route..."<<endl;
    overflow_queue = {}; // clear the queue

    for( int i = 0 ; i < grid_x ; i++){
        for(int j = 0 ; j < grid_y ; j++){

            if( horizontal_overflow[i][j] > 0){
                // if current position is overflow, store position into queue 
                overflow_queue.push(Route(i , j , 0 , 0)); // set position as 0 
                horizontal_history_cost[i][j]++;
            }

            if( vertical_overflow[i][j] > 0){
                overflow_queue.push(Route(i , j , 1 , 0));
                vertical_history_cost[i][j]++;
            }

        }
    }
}

void rip_up(Route node){

    //cout<< "rip up net..."<<endl;

    vector<int> ripupList;
	int cost = 0;
	if( node.direction_r==1){
        ripupList = HIDList[node.x_r][node.y_r];
    }
		
	else{
        ripupList = VIDList[node.x_r][node.y_r];
    }

    /*for(int i = 0 ; i < ripupList.size() ; i++){
        cout<< ripupList[i]<<endl;
    }*/
		
	//rip-up phase
    
	for(int i = 0; i < ripupList.size(); ++i){

		int ID = ripupList[i];
		int k;

		Net n = Net_v[ID];

        //cout<<"n1 : "<< n.p1.x() <<" "<< n.p1.y()<<endl;
        //cout<<"n2 : "<< n.p2.x() <<" "<<n.p2.y()<<endl;

		Route routeA = n.routing_path[0]; // start form 0
        //cout<< routeA.x_r <<" "<<routeA.y_r<<endl; 
		Route routeB; 

		cost = 0;
		for(unsigned j = 1; j < n.routing_path.size(); ++j){

			routeB = n.routing_path[j]; 
            //cout<< routeB.x_r <<" "<<routeB.y_r<<endl; 
			if(routeB.x_r - routeA.x_r == -1 && routeB.y_r == routeA.y_r){

				cost += max(vertical_utilize_edge[routeB.x_r][routeB.y_r] - vertical_capacity, 0);
				vertical_utilize_edge[routeB.x_r][routeB.y_r]--;

                int flag = 0;
				for(k = 0; k < VIDList[routeB.x_r][routeB.y_r].size(); ++k){
                    if(VIDList[routeB.x_r][routeB.y_r][k] == ID){
                        //cout<<"hit"<<endl;
                        flag = 1;
                        break;
                    }	
                }
				if( flag == 1){
                    VIDList[routeB.x_r][routeB.y_r].erase(VIDList[routeB.x_r][routeB.y_r].begin() + k);
                }
				

			}
            else if(routeB.x_r == routeA.x_r && routeB.y_r - routeA.y_r == 1){
				cost += max(horizontal_utilize_edge[routeA.x_r][routeA.y_r] - horizontal_capacity, 0);
				horizontal_utilize_edge[routeA.x_r][routeA.y_r]--;

                int flag = 0;
				for(k = 0; k < HIDList[routeA.x_r][routeA.y_r].size(); ++k){
                    if(HIDList[routeA.x_r][routeA.y_r][k]==ID){
                        //cout<<"hit"<<endl;
                        flag = 1;
                        break;
                    }	
                }
				
                if(flag == 1){
                    HIDList[routeA.x_r][routeA.y_r].erase(HIDList[routeA.x_r][routeA.y_r].begin() + k);
                }
				
			}
			else if(routeB.x_r - routeA.x_r ==1 && routeB.y_r == routeA.y_r){
				
                cost += max(vertical_utilize_edge[routeB.x_r][routeB.y_r] - vertical_capacity, 0);
				vertical_utilize_edge[routeB.x_r][routeB.y_r]--;

                int flag = 0;
				for(k = 0; k < VIDList[routeB.x_r][routeB.y_r].size(); ++k){
                    if(VIDList[routeB.x_r][routeB.y_r][k] == ID){
                        //cout<<"hit"<<endl;
                        flag = 1;
                        break;
                    }	
                }
				
                if(flag == 1){
                    VIDList[routeB.x_r][routeB.y_r].erase(VIDList[routeB.x_r][routeB.y_r].begin() + k);
                }
				

			}
            else if(routeB.x_r == routeA.x_r && routeB.y_r - routeA.y_r == -1){
				
                cost += max(horizontal_utilize_edge[routeA.x_r][routeA.y_r] - horizontal_capacity, 0);
				horizontal_utilize_edge[routeA.x_r][routeA.y_r]--;

                int flag = 0;
				for(k = 0; k < HIDList[routeA.x_r][routeA.y_r].size(); ++k){
                    if(HIDList[routeA.x_r][routeA.y_r][k]==ID){
                        //cout<<"hit"<<endl;
                        flag = 1;
                        break;
                    }	
                }
				
                if(flag == 1){
                    HIDList[routeA.x_r][routeA.y_r].erase(HIDList[routeA.x_r][routeA.y_r].begin() + k);
                }
				
			}

			routeA = routeB;
		}
		n.overflow = cost;
		n.routing_path.clear();
		reroute_queue.push(ripupList[i]);
        //if(cost != 0){
        //    cout<<"cost = "<<cost<<endl;
        //}
            
        
	}
    

    // reroute
    cout<<"reroute..."<<endl;
    while ( reroute_queue.size() != 0){
        int ID = reroute_queue.top();
        reroute_queue.pop();
        
        //cout<<ID<<endl;
        reroute(ID);
        cout<<calculate_overflow()<<endl;
    }
    

}

void reroute(int ID){
    // clear parameters for A* searching
    for(int i = 0 ; i < grid_x ; i++){
        for(int j = 0 ; j < grid_y ; j++){
            // set to -1 if haven't been calculate

            parent[i][j] = -1;
            map_cost[i][j] = -1;
            label[i][j] = 0;
        }
    }

    // create a sorted queues for sub route
    priority_queue< Route , vector<Route> , mycmp> Route_queue;

    // create two route for source and target
    Net n = Net_v[ID];
    Pin s = n.p1;
    Pin t = n.p2;

    Route Source( s.x() , s.y() , 0 , 0);
    Route Target( t.x() , t.y() , 0 , 0);
    Route_queue.push(Source);

    //cout<< Net_v[ID].net_id << endl;
    //cout<<" source : " << s.x() << " " << s.y() << endl;
    //cout<<" target : " << t.x() << " " << t.y() << endl;

    if( Source.x_r == 0 || Source.y_r == 0 || Target.x_r == 0 || Target.y_r == 0){
        return;
    }

    //cout<<"*********************************************************"<<endl;
    //cout<<Route_queue.size()<<endl;
    //cout<<"routing in the global...";

    while( Route_queue.size() != 0){
        Route current = Route_queue.top();  // the route with smallest cost
        Route_queue.pop(); // delete from the queue

        
        if( current.x_r == Target.x_r && current.y_r == Target.y_r){

            // current step reach the target
            //cout<< " reach target" <<endl;
            break;
        }

        // generate 4 direction route from current
        for(int i = 1 ; i <= 4 ; i++){
            //cout<< "direction : "<<i;

            if(i == 1 && current.y_r + 1 >= grid_y){
                continue;
            }
            else if(i == 2 && current.y_r - 1 <= 0){
                continue;
            }
            else if( i == 3 && current.x_r - 1 <= 0){
                continue;
            }
            else if( i == 4 && current.x_r + 1 >= grid_x){
                continue;
            }

            //cout<<" calculate next route"<<endl;
            Route next = propagation( current , i );
        
            if(map_cost[next.x_r][next.y_r] != -1 && next.route_cost >= map_cost[next.x_r][next.y_r]){
                // not a good route solution
                //cout<< " bad solution" <<endl;
                continue;
            }
            else{
                // good route solution
                // update and store
                //cout<<" good solution"<<endl;
                map_cost[next.x_r][next.y_r] = next.route_cost;
                parent[next.x_r][next.y_r] = i;
                
                //cout<<"next : "<<next.x_r<<" "<<next.y_r<<" "<<i<<endl;
                
                // if a good solution appear, stop search other solution
                //i = 5;

                Route_queue.push(next);
                //Net_v[id].routing_path.push_back(next);
            }

        }

    }
    
    // print direction of parent
    /*for (int i = 0 ; i < grid_x ; i++){
        for(int j = 0 ; j < grid_y ; j++){
            if(parent[i][j] != -1){
                cout<<i<<" "<<j<<" "<<parent[i][j]<<" "<<endl;
            }   
        }
    }*/

    //cout<<"*********************************************************"<<endl;
    //cout<<"trace back...";
    Route node( t.x() , t.y() , 0 , 0);
    while(1){

        Net_v[ID].routing_path.push_back(node);

        if(node.x_r == s.x() && node.y_r == s.y()){
            // reach the source
            //cout<<" reach source"<<endl;
            break;
        }

        int parent_direction = parent[node.x_r][node.y_r];
        if( parent_direction == 1){
            // up
            
            horizontal_utilize_edge[node.x_r][node.y_r]++;
            //HIDList[node.x_r][node.y_r].push_back(Net_v[ID].net_id);
            node.y_r --;
        }
        else if( parent_direction == 2){
            // down 
            horizontal_utilize_edge[node.x_r][node.y_r]++;
            //HIDList[node.x_r][node.y_r].push_back(Net_v[ID].net_id);
            node.y_r ++;

        }
        else if( parent_direction == 3){
            // left
            vertical_utilize_edge[node.x_r][node.y_r]++;
            //VIDList[node.x_r][node.y_r].push_back(Net_v[ID].net_id);
            node.x_r ++;
        }
        else if( parent_direction == 4){
            // right
            
            vertical_utilize_edge[node.x_r][node.y_r]++;
            //VIDList[node.x_r][node.y_r].push_back(Net_v[ID].net_id);
            node.x_r --;
        }
        //cout<< "parnet : "<< node.x_r <<" "<<node.y_r<<endl;

    }
    
}

//*********************************************************//

void output(char datapath[]){

    ofstream fout( datapath );
    for(int i = 0 ; i < net_num ; i++){

        Net n = Net_v[i];

        fout<< "net"<<n.net_id<<" "<<n.net_id<<endl;
        for(int j = 0 ; j < n.routing_path.size() - 1; j++){
            fout<<"("<<n.routing_path[j].x_r<<", "<<n.routing_path[j].y_r<<", 1)-";
            fout<<"("<<n.routing_path[j+1].x_r<<", "<<n.routing_path[j+1].y_r<<", 1)"<<endl;
        }
        fout<<"!"<<endl;
    }

}

