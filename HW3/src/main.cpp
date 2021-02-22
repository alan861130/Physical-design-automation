#include <iostream>
#include <cstdlib>
#include <time.h>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <cmath>

#include "type_def.h"



using namespace std;

vector<hardblock> hardblock_v , local_hardblock_v , best_hardblock_v;
vector<terminal> terminal_v;
vector<net> net_v;

vector<int> y_boundary;


//reading part
void read_input(char f1[] , char f2[] , char f3[]);
void read_hardblock(char f[]);
void read_net(char f[]);
void read_pl(char f[]);
void set_seed(char f[]);

//calculate area constraint
void get_boundary(void);
void check_boundary(void);

//floor plan part
void init_floor_plan(void);
void init_y_boundary(void);
void clear_y_boundary(void);
int calculate_y(int curr_x , int width , int height);

//simulate_annealing
void simulate_annealing(void);
void SA_setup_1(void);
void SA_setup_2(void);
int round_unit(void);
void packing( int curr_block , int cond); // cond = 0 : search left, cond = 1 : search right
void perturbation(void);
void rotate(void);
void swap(void);
void delete_and_insert(void);
void calculate_wire_length(void);
float calculate_cost( float x, float y,float wirelength);

//output
void output(char f[]);



//constraint
float error_rate;
int total_block_area;
int boundary;
int x_max; // max x boundary of floorplan
int y_max; // max y boundary of floorplan



//reading part
int NumHardRectilinearBlocks;
int NumTerminals;
int NumNets;
int NumPins;
long long int random_seed;

//floor plan part
//none

//simulate annealing
float alpha;
float beta;

int perturbation_count;
int perturbation_time;
float boundary_ratio ; 
int block_count; // used for calculate num of block during packing

int root = 0; // default
int wirelength;

int best_root = 0;
float best_x; // x length of good solution
float best_y; // y length of good sloution
int best_wirelength;
float best_cost;

int current_root = 0;
int current_x;
int current_y;
int current_wirelength;
float current_cost;

//time analysis
clock_t packing_start_t , packing_end_t;
clock_t perturbation_start_t , perturbation_end_t;
clock_t wirelength_start_t , wirelength_end_t;
int packing_t;
int perturbation_t;
int wirelength_t;


int main(int argc , char *argv[]){

    clock_t start_clk, io_clk, init_floor_plan_clk ,sa_clk,end_clk;
    packing_t = 0;
    perturbation_t = 0;
    wirelength_t = 0;

    start_clk = clock();

    //start
    read_input(argv[1] , argv[2] , argv[3] );
    
    //set error rate
    if( strcmp ( argv[5] , "0.1") == 0 ){
        error_rate = 0.1;
    }
    else if( strcmp ( argv[5] , "0.15") == 0 ){
        error_rate = 0.15;
    }
    else{
        cout<<"Wrong error rate"<<endl;
    }

    set_seed(argv[1]);

    
    get_boundary();
    io_clk = clock();
    
    init_floor_plan();
    init_floor_plan_clk = clock();
    //calculate_wire_length();
    //cout<<wirelength<<endl;
    
    simulate_annealing();
    sa_clk = clock();

    output( argv[4] );
    
    //end

    end_clk = clock();
    cout<<endl;
    cout<<"**************************************************************"<<endl;
    cout<<endl;

    cout<<"I/O time : "<< ( (double)(io_clk - start_clk) + (double)(end_clk - sa_clk) )/ CLOCKS_PER_SEC <<endl;

    cout<<endl;
    cout<<"**************************************************************"<<endl;
    cout<<endl;

    cout<<"Initial floorplanning time : "<<(double)(init_floor_plan_clk - io_clk) / CLOCKS_PER_SEC <<endl;

    cout<<endl;
    cout<<"**************************************************************"<<endl;
    cout<<endl;

    cout<<"Perturbation time  : "<<(double)(perturbation_t) / CLOCKS_PER_SEC <<endl;
    cout<<"Packing time  : "<<(double)(packing_t) / CLOCKS_PER_SEC <<endl;
    cout<<"Calculating wirelength  time  : "<<(double)(wirelength_t) / CLOCKS_PER_SEC <<endl;

    cout<<endl;
    cout<<"**************************************************************"<<endl;
    cout<<endl;

    cout<<"Total time duration : "<<(double)(end_clk - start_clk) / CLOCKS_PER_SEC <<endl;

    

}

//*****************************************************

void read_input(char f1[] , char f2[] , char f3[]){
    
    read_hardblock(f1);
    read_net(f2);
    read_pl(f3);

}

void read_hardblock(char f[]){
    //cout<<"Reading hardblocks..."<<endl;

    ifstream fin(f);
    string t,x,y;

    //first line
    fin>>t>>t>>NumHardRectilinearBlocks;

    //second line
    fin>>t>>t>>NumTerminals;

    //cout<<NumHardRectilinearBlocks<<" "<<NumTerminals<<endl;
    
    //read hardblock
    total_block_area = 0;
    for(int i = 0 ; i < NumHardRectilinearBlocks ; i++){

        fin>> t >> t >> t >> t >> t >> t >> y >> x;
        y = y.substr( 0 , y.length() - 1);
        x = x.substr( 1 , x.length() - 2);
        //cout<< x <<" "<< y <<endl;
        getline(fin , t);
        hardblock h( i , atoi(x.c_str()) , atoi(y.c_str()) );
        //cout<<h.x_length_h<<" "<<h.y_length_h<<endl;
        total_block_area = total_block_area + h.area();
        hardblock_v.push_back(h);

    }

    //blank line
    

    //read terminal
    
    for(int i = 0 ; i < NumTerminals ; i++){
        
        terminal t(i);
        terminal_v.push_back(t);

    }
    

    
    fin.close();
}

void read_net(char f[]){
    //cout<<"Reading nets..."<<endl;

    ifstream fin(f);
    string t , data;
    int degree;

    //first line
    fin>> t >> t >> NumNets;

    //second line
    fin >> t >> t >> NumPins;

    //cout<<NumNets<<" "<<NumPins<<endl;

    //read net
    for(int i = 0 ; i < NumNets ; i++){

        fin>> t >> t >> degree;

        net n(i);
        for(int j = 0 ; j < degree ; j++){

            fin>>data;
            //cout<<data<<endl;

            if(data[0] == 's'){
                //hardblock
                //cout<<"store into hardblock"<<endl;
                int id = atoi( data.substr(2).c_str() );
                //cout<<id<<endl;
                n.add_connected_hardblock(id);
                //hardblock_v[id].add_connected_net(i);
            }

            else{
                //terminal
                //cout<<"store into terminal"<<endl;
                int id = atoi( data.substr(1).c_str() );
                //cout<<id<<endl;
                n.add_connected_terminal(id-1);
                //terminal_v[id].add_connected_net(i);
            }

        }

        net_v.push_back(n);

    }

    fin.close();
}

void read_pl(char f[]){
    //cout<<"Reading pl..."<<endl;

    fstream fin(f);
    string name;
    int id , x , y;

    for(int i = 0 ; i < NumTerminals ; i++){
        fin>>name>>x>>y;

        //id = atoi ( name.substr(1).c_str() );
    
        terminal_v[i].set_pos(x,y);
        //cout<<terminal_v[i].x_t<<" "<<terminal_v[i].y_t<<endl;
    }
    fin.close();
}

void set_seed(char f[]){

    char output_file = { f[13] };  //nx00

    
    if( output_file == '1' ){
        cout<<"n100"<<endl;
        if( error_rate < 0.15){
            cout<<"0.1"<<endl;
            random_seed = 1607231910;
        }
        else{
            cout<<"0.15"<<endl;
            random_seed = 1607232160;
        }
    }
    else if( output_file == '2' ){
        cout<<"n200"<<endl;
        if( error_rate < 0.15){
            cout<<"0.1"<<endl;
            random_seed = 1607224432;
        }
        else{
            cout<<"0.15"<<endl;
            random_seed = 1607232479;
        }
        
    }
    else if( output_file == '3' ){
        cout<<"n300"<<endl;
        if( error_rate < 0.15){
            cout<<"0.1"<<endl;
            random_seed = 1607189782;
        }
        else {
            cout<<"0.15"<<endl;
            random_seed = 1607193512;
        }
        
    }

}

//*****************************************************

void get_boundary(void){

    //cout<<total_block_area<<endl;
    //cout<<error_rate<<endl;
    boundary = (int)(sqrt( total_block_area * (1 + error_rate) )) ;
    //cout<<"boundary = "<<boundary<<endl;
}

void check_boundary(void){

    current_x = 0;
    current_y = 0;
    for(int i = 0 ; i < NumHardRectilinearBlocks ; i++){

        if(hardblock_v[i].x_h + hardblock_v[i].x_length_h > current_x){
            current_x = hardblock_v[i].x_h + hardblock_v[i].x_length_h;
        }

        if(hardblock_v[i].y_h + hardblock_v[i].y_length_h > current_y){
            current_y = hardblock_v[i].y_h + hardblock_v[i].y_length_h;
        }

    }

    //if(x_max > boundary || y_max > boundary){
    //    cout<<"Floorplan is out of boundary..."<<endl;
    //}
}

//*****************************************************

void init_floor_plan(void){

    //cout<<"initial floor plan..."<<endl;
    int curr_x = 0;
    int last_node = 0;
    int left_node = 0;

    init_y_boundary();
    
    for(int current_node = 0 ; current_node < NumHardRectilinearBlocks ; current_node++){
        //root
        if(current_node == 0){
            //write the root block
            hardblock_v[current_node].left_child = 1;
            hardblock_v[current_node].x_h = 0;
            hardblock_v[current_node].y_h = calculate_y(curr_x , hardblock_v[current_node].x_length_h , hardblock_v[current_node].y_length_h);

            //adjust to next block
            curr_x = 0 + hardblock_v[current_node].x_length_h;
            last_node = 0;
            left_node = 0;
        }
        //block has been selected
        else{
            
            if( curr_x + hardblock_v[current_node].x_length_h <= boundary){
                //write current block
                hardblock_v[current_node].parent = last_node;
                hardblock_v[last_node].left_child = current_node;

                hardblock_v[current_node].x_h = curr_x;
                hardblock_v[current_node].y_h = calculate_y(curr_x , hardblock_v[current_node].x_length_h , hardblock_v[current_node].y_length_h);
                

                //adjust to next block
                curr_x = curr_x + hardblock_v[current_node].x_length_h;
                last_node = current_node;

            }

            //move to upper floor
            else{
                hardblock_v[left_node].right_child = current_node;
                hardblock_v[current_node].parent = left_node;

                hardblock_v[current_node].x_h = 0;
                hardblock_v[current_node].y_h = calculate_y(0 , hardblock_v[current_node].x_length_h , hardblock_v[current_node].y_length_h);
                

                //adjust to next block
                curr_x = 0 + hardblock_v[current_node].x_length_h;
                left_node = current_node;
                last_node = current_node;

            }

        }
    
        //cout<<hardblock_v[current_node].id_h<<" "<<hardblock_v[current_node].x_h<<" "<<hardblock_v[current_node].y_h<<endl;
    
    }

}

void init_y_boundary(void){
    for(int i = 0 ; i < boundary*2 ; i++){
        y_boundary.push_back(0);
    }
}

void clear_y_boundary(void){
    for(int i = 0 ; i < boundary*2 ; i++){
        y_boundary[i] = 0;
    }
}

int calculate_y(int curr_x , int width , int height){

    //find max height before put in the block
    int max = 0;
    
    for(int i = curr_x ; i < curr_x + width ; i++){
        if(y_boundary[i] > max){
            max = y_boundary[i];
        }
    }
    //cout<<"old y max = "<<max<<endl;

    int new_max = max + height;
    //cout<<"new y max = "<<new_max<<endl;
    for(int i = curr_x ; i < curr_x + width ; i++){
        y_boundary[i] = new_max;
    }

    return max;
}

//*****************************************************

void simulate_annealing(void){

    //cout<<"start simulate annealing..."<<endl;
    
    //long long int random_seed = time(NULL);
    int choose;
    //long long int random_seed = 1606880011 ;
    cout<<random_seed<<endl;
    srand(random_seed); //give random seed

    //first round  
    alpha = 0.5;
    beta = 0.1;
    SA_setup_1();
    perturbation_time = round_unit()*3  ;
    perturbation_count = 0;
    int good_solution = 0;
    while(perturbation_count < perturbation_time) {

        //cout<<perturbation_round<<" ";
        perturbation_start_t = clock();
        perturbation();
        perturbation_end_t = clock();
        perturbation_t = perturbation_t + ( perturbation_end_t - perturbation_start_t);

        clear_y_boundary();
        block_count = 0;

        packing_start_t = clock();
        packing(root , 0);
        packing_end_t = clock();
        packing_t = packing_t + (packing_end_t - packing_start_t);

        if(block_count != NumHardRectilinearBlocks ){
            cout<<block_count<<endl;
            cout<<"error"<<endl;
            return;
        }
        check_boundary();

        wirelength_start_t = clock();
        calculate_wire_length();
        wirelength_end_t = clock();
        wirelength_t = wirelength_t + (wirelength_end_t - wirelength_start_t);

        current_cost = calculate_cost(current_x , current_y , wirelength);
        //cout<<current_x<<" "<<current_y<<endl;

        
        if( current_cost > best_cost ){
            // bad solution
            hardblock_v = best_hardblock_v;
            root = best_root;
        }
        else{
            //good solution
            //cout<<"good solution"<<endl;
            good_solution++;
            best_hardblock_v = hardblock_v;
            best_root = root;

            best_x = current_x;
            best_y = current_y;
            best_wirelength = wirelength;

            best_cost = calculate_cost(current_x , current_y , wirelength);
            //cout<<"best cost : "<<best_cost<<endl;
        }

        perturbation_count++;

    }

    if(best_x > boundary || best_y > boundary ){
        cout<<"Fail to fix the boundary"<<endl;
        exit(0);
    }
    cout<<best_x<<" "<<best_y<<" "<<best_wirelength<<endl;

    /*
    int k;
    //second round
    for(int i = 0 ; i < 1 ; i ++){
        alpha = 0.1;
        beta = 0.9;
        SA_setup_2();
        perturbation_time = round_unit() * 1 ;
        perturbation_count = 0;
        good_solution = 0;
        k = 1;
        while(1) {

            if(perturbation_count == perturbation_time * k){
                
                
                if(alpha > 0){
                    alpha = alpha - 0.01;
                    beta = beta + 0.01;
                        
                }
                else{
                    break;
                }
                    
                cout<<"alpha = "<<alpha<<" beta = "<<beta<<endl;
                cout<<best_x<<" "<<best_y<<" "<<best_wirelength<<endl;
                best_cost = calculate_cost(best_x , best_y , best_wirelength);
                
                perturbation_count = 0;
            }
            //cout<<perturbation_round<<" ";
            perturbation_start_t = clock();
            perturbation();
            perturbation_end_t = clock();
            perturbation_t = perturbation_t + ( perturbation_end_t - perturbation_start_t);

            clear_y_boundary();
            block_count = 0;
            packing_start_t = clock();
            packing(root , 0);
            packing_end_t = clock();
            packing_t = packing_t + (packing_end_t - packing_start_t);
            if(block_count != NumHardRectilinearBlocks ){
                cout<<block_count<<endl;
                cout<<"error"<<endl;
                return;
            }
            check_boundary();

            wirelength_start_t = clock();
            calculate_wire_length();
            wirelength_end_t = clock();
            wirelength_t = wirelength_t + (wirelength_end_t - wirelength_start_t);
            current_cost = calculate_cost(current_x , current_y , wirelength);
            //cout<<current_x<<" "<<current_y<<endl;

            if( current_cost > best_cost ){
                // bad solution
                hardblock_v = best_hardblock_v;
                root = best_root;
            }
            else{
                //good solution
                //cout<<"good solution"<<endl;
                if(current_x <= boundary && current_y <= boundary ){
                    good_solution++;
                    best_hardblock_v = hardblock_v;
                    best_root = root;

                    best_x = current_x;
                    best_y = current_y;
                    best_wirelength = wirelength;

                    best_cost = calculate_cost(current_x , current_y , wirelength);
                    //cout<<"best cost : "<<best_cost<<endl;
                }
                else if(current_x <= boundary * 1.15 && current_y <= boundary *1.15){
                    continue;
                }
                else{
                    break;
                }
                
            }
            perturbation_count++;

        }
        
        cout<<random_seed<<" "<<best_x<<" "<<best_y<<" "<<best_wirelength<<endl;
    }
    
    */
    
    
}

void SA_setup_1(void){

    best_hardblock_v = hardblock_v;
    
    clear_y_boundary();
    block_count = 0;
    packing(root , 0); // walk through all blocks
    if(block_count != NumHardRectilinearBlocks ){
        cout<<block_count<<endl;
        cout<<"error"<<endl;
        return;
    }
    check_boundary();
    calculate_wire_length();

    best_x = current_x;
    best_y = current_y;
    best_wirelength = wirelength;

    best_cost = calculate_cost( current_x , current_y , wirelength);

    //cout<<best_cost<<endl;

}

void SA_setup_2(void){

    hardblock_v = best_hardblock_v;
    
    clear_y_boundary();
    block_count = 0;
    packing(root , 0); // walk through all blocks
    if(block_count != NumHardRectilinearBlocks ){
        cout<<block_count<<endl;
        cout<<"error"<<endl;
        return;
    }
    check_boundary();
    calculate_wire_length();

    best_cost = calculate_cost( best_x, best_y  , best_wirelength);

}

int round_unit(void){

    return NumHardRectilinearBlocks * NumHardRectilinearBlocks;

}

void packing( int curr_block , int cond){


    if(curr_block == root){

        hardblock_v[curr_block].x_h = 0;
        hardblock_v[curr_block].y_h = calculate_y(0 , hardblock_v[curr_block].x_length_h , hardblock_v[curr_block].y_length_h);

    }
    //search left
    else if(cond == 0){
        int parnet = hardblock_v[curr_block].parent;

        hardblock_v[curr_block].x_h = hardblock_v[parnet].x_h + hardblock_v[parnet].x_length_h;
        hardblock_v[curr_block].y_h = calculate_y(hardblock_v[curr_block].x_h , hardblock_v[curr_block].x_length_h , hardblock_v[curr_block].y_length_h);

    }
    //search right
    else{
        int parnet = hardblock_v[curr_block].parent;

        hardblock_v[curr_block].x_h = hardblock_v[parnet].x_h;
        hardblock_v[curr_block].y_h = calculate_y(hardblock_v[curr_block].x_h , hardblock_v[curr_block].x_length_h , hardblock_v[curr_block].y_length_h);

    }

    
    //left child existed
    if( hardblock_v[curr_block].left_child != -1){
        //go to left child and extend left
        packing(hardblock_v[curr_block].left_child , 0);
    }
    
    //right child existed
    if( hardblock_v[curr_block].right_child != -1){
        //go to right child and extend right
        packing(hardblock_v[curr_block].right_child , 1);
    }
    block_count++;
    //cout<<hardblock_v[curr_block].id_h<<" "<<hardblock_v[curr_block].x_h<<" "<<hardblock_v[curr_block].y_h<<endl;
    //cout<<"left child : "<<hardblock_v[curr_block].left_child<<" right child : "<<hardblock_v[curr_block].right_child<<endl;
}

void perturbation(void){

    int cond = rand()%3;
    if(cond == 0){
        //rotate
        rotate();
    }
    else if(cond == 1){
        //swap
        swap();
    }
    else{
        //delete and insert
        delete_and_insert();
    }
}

void rotate(void){
    //cout<<"rotate"<<endl;
    int select_block = rand() % NumHardRectilinearBlocks;
    
    if(hardblock_v[select_block].rotate == 0){
        hardblock_v[select_block].rotate = 1;
    }
    else{
        hardblock_v[select_block].rotate = 0;
    }
    //rotate steps
    int temp_x = hardblock_v[select_block].x_length_h;
    hardblock_v[select_block].x_length_h = hardblock_v[select_block].y_length_h;
    hardblock_v[select_block].y_length_h = temp_x;
    
    
}

void swap(void){
    
    int select_block_1 = rand() % NumHardRectilinearBlocks;
    while( select_block_1 == 0){
        select_block_1 = rand() % NumHardRectilinearBlocks;
    }
    int select_block_2 = rand() % NumHardRectilinearBlocks;

    while( select_block_1 == select_block_2 || select_block_1 == hardblock_v[select_block_2].parent || select_block_2 == hardblock_v[select_block_1].parent || select_block_2 == 0){
        select_block_2 = rand() % NumHardRectilinearBlocks;
    }

    //int select_block_1 = 0;
    //int select_block_2 = 99;
    

    //cout<<"swap "<<hardblock_v[select_block_1].id_h<<" "<<hardblock_v[select_block_2].id_h<<endl;
    
    int h1_parent = hardblock_v[select_block_1].parent;
    int h1_lchild = hardblock_v[select_block_1].left_child;
    int h1_rchild = hardblock_v[select_block_1].right_child;

    //cout<<h1_parent<<" "<<h1_lchild<<" "<<h1_rchild<<endl;

    int h2_parent = hardblock_v[select_block_2].parent;
    int h2_lchild = hardblock_v[select_block_2].left_child;
    int h2_rchild = hardblock_v[select_block_2].right_child;

    //cout<<h2_parent<<" "<<h2_lchild<<" "<<h2_rchild<<endl;

    hardblock_v[select_block_1].parent = h2_parent;
    hardblock_v[select_block_1].left_child = h2_lchild;
    hardblock_v[select_block_1].right_child = h2_rchild;

    hardblock_v[select_block_2].parent = h1_parent;
    hardblock_v[select_block_2].left_child = h1_lchild;
    hardblock_v[select_block_2].right_child = h1_rchild;

    if(h1_lchild != -1){
        hardblock_v[h1_lchild].parent = select_block_2;
    }
    if(h1_rchild != -1){
        hardblock_v[h1_rchild].parent = select_block_2;
    }
    if(h2_lchild != -1){
        hardblock_v[h2_lchild].parent = select_block_1;
    }
    if(h2_rchild != -1){
        hardblock_v[h2_rchild].parent = select_block_1;
    }


    if(h1_parent == -1){
        hardblock_v[select_block_2].parent = -1;
        root = select_block_2;
    }
    else{
        if(hardblock_v[h1_parent].left_child == select_block_1){
            hardblock_v[h1_parent].left_child = select_block_2;
        }
        else{
            hardblock_v[h1_parent].right_child = select_block_2;
        }
    }
    
    if(h2_parent == -1){
        hardblock_v[select_block_1].parent = -1;
        root = select_block_1;
    }
    else{
        if(hardblock_v[h2_parent].left_child == select_block_2){
            hardblock_v[h2_parent].left_child = select_block_1;
        }
        else{
            hardblock_v[h2_parent].right_child = select_block_1;
        }
    }

    //cout<<hardblock_v[select_block_1].parent<<" "<<hardblock_v[select_block_1].left_child<<" "<<hardblock_v[select_block_1].right_child<<endl;
    //cout<<hardblock_v[select_block_2].parent<<" "<<hardblock_v[select_block_2].left_child<<" "<<hardblock_v[select_block_2].right_child<<endl;
}

void delete_and_insert(void){

    int select_block = rand() % NumHardRectilinearBlocks;
    int pos = rand() % NumHardRectilinearBlocks;

    while( select_block == pos || select_block == hardblock_v[pos].parent || pos == hardblock_v[select_block].parent){
        pos = rand() % NumHardRectilinearBlocks;
    }
    //cout<<"delete "<<select_block<<" and insert into "<<pos<<endl;

    //delete
    int p = hardblock_v[select_block].parent;
    int lchild = hardblock_v[select_block].left_child;
    int rchild = hardblock_v[select_block].right_child;
    
    if(p == -1){

        root = lchild;

        hardblock_v[lchild].parent = -1;
        hardblock_v[lchild].right_child = rchild;
        hardblock_v[rchild].parent = lchild; 
    }
    else{

        hardblock_v[lchild].right_child = rchild;
        if( rchild != -1){
            hardblock_v[rchild].parent = lchild;
        }

        hardblock_v[lchild].parent = p;
        if(hardblock_v[p].left_child == select_block){
            hardblock_v[p].left_child = lchild;
        }
        else{
            hardblock_v[p].right_child = lchild;
        }
    }

    p = hardblock_v[pos].parent;
    lchild = hardblock_v[pos].left_child;
    rchild = hardblock_v[pos].right_child;

    if(p == -1){

        root = select_block;
        hardblock_v[select_block].parent = -1;
        
        hardblock_v[select_block].left_child = pos;
        hardblock_v[pos].parent = select_block;

        hardblock_v[select_block].right_child = rchild;
        hardblock_v[pos].right_child = -1;
        hardblock_v[rchild].parent = select_block;
    }

    else{
        hardblock_v[select_block].left_child = pos;
        hardblock_v[pos].parent = select_block;

        hardblock_v[pos].right_child = -1;
        hardblock_v[select_block].right_child = rchild;
        if(rchild != -1){
            hardblock_v[rchild].parent = select_block;
        }

        hardblock_v[select_block].parent = p;
        if(hardblock_v[p].left_child == pos ){
            hardblock_v[p].left_child = select_block;
        }
        else{
            hardblock_v[p].right_child = select_block;
        }
    }
    

}

void calculate_wire_length(void){
    
    wirelength = 0;
    int local_wirelength;
    int x_coor_min ;  // left
    int x_coor_max ;  //right
    int y_coor_min ;  //top
    int y_coor_max ;  //down
    int center_x; // x coordinate of the center of hardblock 
    int center_y; // y coordinate of the center of hardblock
    int terminal_x; // x coordinate of terminal
    int terminal_y; // y coordinate of termianl

    int hardblock_num;
    int terminal_num;

    

    for(int i = 0 ; i < NumNets; i++){

        x_coor_min = 10000;
        x_coor_max = -1;
        y_coor_min = 10000;
        y_coor_max = -1;

        // count hardblock first
        hardblock_num = net_v[i].connected_hardblock.size();
        terminal_num = net_v[i].connected_terminal.size();

        //cout<<"***********************************************"<<endl;
        //cout<<"Number of net : "<<i<<endl;
        

        if(hardblock_num != 0){
            //cout<<"hardblock : "<<endl;
            for(int j = 0 ; j < hardblock_num ; j++){
                
                int id = net_v[i].connected_hardblock[j];

                center_x = hardblock_v[id].x_h + (hardblock_v[id].x_length_h / 2); 
                center_y = hardblock_v[id].y_h + (hardblock_v[id].y_length_h / 2);
                //cout<<id<<"( "<<center_x<<" "<<center_y<<" )"<<endl;

                if( center_x < x_coor_min ){
                    x_coor_min = center_x;
                } 
                if( center_x > x_coor_max){
                    x_coor_max = center_x;
                }
                if( center_y < y_coor_min){
                    y_coor_min = center_y;
                }
                if( center_y > y_coor_max){
                    y_coor_max = center_y;
                }

                
            }
        }

        
        if(terminal_num != 0){
            //cout<<"terminal :"<<endl;

            for(int j = 0 ; j < terminal_num ; j++){

                int id = net_v[i].connected_terminal[j];
                terminal_x = terminal_v[id].x_t;
                terminal_y = terminal_v[id].y_t;
                //cout<<id<<"( "<<terminal_x<<" "<<terminal_y<<" )"<<endl;


                if( terminal_x < x_coor_min ){
                    x_coor_min = terminal_x;
                } 
                if( terminal_x > x_coor_max){
                    x_coor_max = terminal_x;
                }
                if( terminal_y < y_coor_min){
                    y_coor_min = terminal_y;
                }
                if( terminal_y > y_coor_max){
                    y_coor_max = terminal_y;
                }
   
            }
        }
        
        //cout<<"X Max : "<<x_coor_max<<endl;
        //cout<<"X Min : "<<x_coor_min<<endl;
        //cout<<"Y Max : "<<y_coor_max<<endl;
        //cout<<"y Min : "<<y_coor_min<<endl;

        local_wirelength = ( x_coor_max - x_coor_min ) + ( y_coor_max - y_coor_min );
        //cout<<"Local wirelength : "<<local_wirelength<<endl;
        wirelength = wirelength + local_wirelength;

    }

}

float calculate_cost( float x, float y,float wirelength){

    float area = (x * y) / (best_x * best_y);
    float ratio = abs(1 - (x / y) );
    float w = wirelength / best_wirelength;

    //cout<<area<<" "<<w<<" "<<ratio<<endl;
    float output = alpha * area + beta * w + (1 - alpha - beta) *ratio ;
    //cout<<"cost = "<<output<<endl;
    return output;

}

void output(char f[]){

    ofstream  fout(f);

    calculate_wire_length();
    cout<<wirelength<<endl;

    //hardblock_v = best_hardblock_v;
    //calculate_wire_length();
    //cout<<wirelength<<endl;

    fout<<"Wirelength "<<best_wirelength<<endl;
    fout<<"Blocks"<<endl;
    for(int i = 0 ; i < NumHardRectilinearBlocks ; i++){

        fout<<"sb"<<hardblock_v[i].id_h<<" "<<hardblock_v[i].x_h<<" "<<best_hardblock_v[i].y_h<<" "<<best_hardblock_v[i].rotate<<endl;
        
    }
}
