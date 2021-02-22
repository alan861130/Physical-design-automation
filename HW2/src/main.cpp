#include <iostream>
#include <cstring>
#include <string.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <map>
#include <cstdlib>

#include <time.h>


#include "node_type_def.h"

using namespace std;


int get_data_count(char filename[]);
void init(char cellfile[], char netfile[], int Cell_count, int Net_count);
void calculate_p_max(void);

void balance_init_partition(void);
bool calculate_R(void);
void show_cell_partition(void);

void get_cutsize(void);
void update_net(void);

void FM_algorithm(void);
void reset(int best_round);
void init_gain(void);
void show_cell_gain(void);

void create_bucket_list(void);
void insert(Cell_node* c);
void show_bucket_list(void);

void A_find_max_gain(void);
void B_find_max_gain(void);
int change_area_from_A_to_B(int size);
int change_area_from_B_to_A(int size);
void update_gain(Cell_node *c);
void move(Cell_node* c);
void remove(Cell_node* c);

void output(char filename[]);

vector<Cell_node*> init_cell;
vector<Net_node*> init_net;
vector<Cell_node*> move_cell;
vector<Cell_node*> change_gain_cell;

map<string , Cell_node*> cell_arr;
map<string , Net_node*> net_arr;



Cell_node *A_max;
Cell_node *B_max;


int C; //total num of cells
int N; //total num of nets
int P; //total num of pins
int P_max; //max pin in one cell
int init_cut_size = 0;
int cut_size = 0;
int accumulate_gain = 0;

int A_cell_count = 0; // total num of cells in set A
int A_area = 0; // sum of cell size in set A
int A_bucket_list_count; // num of cells in A bucket list

int B_cell_count = 0; // total num of cells in set B
int B_area = 0; // sum of cell size in set B
int B_bucket_list_count; //num of cells in B bucket list

int best_gain = 0;
int FM_round;
int best_round;
int best_balance;
int best_A_cell_count;
int best_B_cell_count;
int best_A_area;
int best_B_area;

map<int , map<string , Cell_node* > > A_bucket_list , B_bucket_list;


int main(int argc, char *argv[]){

    clock_t  clock_a,clock_b,clock_c,clock_d;
    clock_a = clock();

    C = get_data_count(argv[2]);
    N = get_data_count(argv[1]);
    cout<<"Total cell count : "<< C <<endl;
    cout<<"Total net count : "<< N <<endl;

    cout<<"Parsing cells and nets..."<<endl;
    init(argv[2],argv[1], C, N); //calculate the connected net of the cell

    calculate_p_max();
    cout<<"P max = "<<P_max<<endl<<endl;

    cout<<"Balancing initial partition..."<<endl;
    balance_init_partition();

    cout<<"Calculating cut size..."<<endl;
    get_cutsize();
    cout<<"cut size = "<<cut_size<<endl<<endl;

    clock_b = clock();

    
    cout<<"Start FM algorithm..."<<endl;
    FM_algorithm();
    

    clock_c = clock();

    output(argv[3]);

    clock_d = clock();

    cout<<"I / O time : "<< (double)((clock_b - clock_a) + (clock_d - clock_c)) / CLOCKS_PER_SEC <<endl;
    cout<<"Computation time : "<<(double)(clock_c - clock_b) / CLOCKS_PER_SEC <<endl;
    cout<<"Time duration : "<<(double)(clock_d - clock_a) / CLOCKS_PER_SEC <<endl;

    return 0;

}

int get_data_count(char filename[]){

    fstream fin;
    fin.open(filename, ios::in);

    int count = 0;
    string tmps;

    while ( getline ( fin , tmps ) ) {
        count++;
    }
    fin.close();

    return count;

}

void init(char cellfile[], char netfile[], int Cell_count, int Net_count){

    fstream fin1, fin2;
    
    fin1.open(cellfile, ios::in);
    fin2.open(netfile, ios::in);

    

    char name[10],tmp1[4],tmp2[4],stop_cond[10] ;
    strcpy(stop_cond , "}");
    int pin_count = 0 , size;

    //cell
    for(int i = 0 ; i < Cell_count ; i ++){
        
        fin1>>name>>size;
        Cell_node *CNode = new Cell_node(name,size);

        if(A_area < B_area){
            // put cell into A set
            CNode->cell_set = 0;
            A_cell_count++;
            A_area = A_area + CNode->cell_size;

        }
        else{
            // put cell into B set
            CNode->cell_set = 1;
            B_cell_count++;
            B_area = B_area + CNode->cell_size;

        }

        init_cell.push_back(CNode);
        cell_arr[name] = CNode;
        //cout<<cell_arr[name]->cell_name<<endl;
    }

    //net
    for(int i = 0 ; i < Net_count ; i++){
        
        fin2>>tmp1>>name>>tmp2;
        Net_node *NNode = new Net_node(name);

        while(1){

            fin2>>tmp1;
            if(strcmp(tmp1 , stop_cond) == 0 ){
                break;
            }

            else{

                cell_arr[tmp1]->cell_pin++;
                cell_arr[tmp1]->connected_net.push_back(NNode->net_name);
                P++;

                NNode->connected_cell.push_back(tmp1);

            }
        } 

        init_net.push_back(NNode);
        net_arr[name] = NNode;
    }

    fin1.close();
    fin2.close();

}

void calculate_p_max(void){

    P_max = 0;
    for(int i = 0 ; i < init_cell.size(); i++){
        if ( init_cell[i]->cell_pin > P_max){
            P_max = init_cell[i]->cell_pin;
        }
    }
}

void balance_init_partition(void){

    show_cell_partition();

    if(calculate_R() == 1){
        cout<<"No need for balancing"<<endl;
    }
    else{
        cout<<"Need balancing"<<endl;

        int i = 0;
        while( i < C ){

            if(calculate_R() == 1){
                break;
            }
            if( A_area > B_area && init_cell[i]->cell_set == 1 ){

                init_cell[i]->cell_set = 0;
                A_area = A_area - init_cell[i]->cell_size;
                B_area = B_area + init_cell[i]->cell_size;

            }
            else if(A_area < B_area && init_cell[i]->cell_set == 0){
                init_cell[i]->cell_set = 1;
                A_area = A_area + init_cell[i]->cell_size;
                B_area = B_area - init_cell[i]->cell_size;

            }

            i++;
        }
    }

}

bool calculate_R(void){

    int n = (A_area + B_area)/10  , m = A_area - B_area ;
    //cout<< n << " " << m <<endl;

    if( abs( m ) <= n ){
        return 1;
    }
    else{
        return 0;
    }

}

void show_cell_partition(void){

    //cout<<"Cells in A set : ";
    //for(int i = 0 ; i < init_cell.size(); i++){
    //    if(init_cell[i]->cell_set == 0){
    //        cout<<init_cell[i]->cell_name<<" ";
    //    }
    //}
    //cout<<endl;
    cout<<"A set cell count : "<<A_cell_count<<endl;
    cout<<"A set cell area : "<< A_area <<endl<<endl;

    //cout<<"Cells in B set : ";
    //for(int i = 0 ; i < init_cell.size(); i++){
    //    if(init_cell[i]->cell_set == 1){
    //        cout<<init_cell[i]->cell_name<<" ";
    //    }
    //}
    cout<<endl;
    cout<<"B set cell count : "<<B_cell_count<<endl;
    cout<<"B set cell area : "<< B_area <<endl<<endl;

}

void get_cutsize(void){

    update_net();
    cut_size = 0;
    for(int i = 0 ; i < init_net.size(); i++){
        //cout<<init_net[i].net_name<<" "<<init_net[i].A<<" "<<init_net[i].B<<endl;
        if(init_net[i]->A > 0  && init_net[i]->B > 0 ){
            
            cut_size++;
        }

    }
    init_cut_size = cut_size;

}

void update_net(void){
    
    for(int i = 0 ; i < init_net.size(); i++){  
        //select the ith net
        init_net[i]->A = 0;
        init_net[i]->B = 0;
        
        for(int j = 0 ; j < init_net[i]->connected_cell.size(); j++){  
            //ith net's jth connected cell

            string name = init_net[i]->connected_cell[j];
            //cout<<cell_arr[name]->cell_name<<endl;
            
            if( cell_arr[name]->cell_set == 0){
                // cell is in set A
                //cout<<cell_arr[name]->cell_name<<" is in A set"<<endl;
                init_net[i]->A++;
                
            }
            else if (cell_arr[name]->cell_set == 1){
                // set is in set B
                //cout<<cell_arr[name]->cell_name<<" is in B set"<<endl;
                init_net[i]->B++;

            }
            
        }

    }
    //if(init_net[i].A == 1 && init_net[i].B == 1){
    //    cout<<"Cut !"<<endl;
    //}

}

void FM_algorithm(void){

    FM_round = 0 ;
    best_round = 0;
    best_gain = 0;
    accumulate_gain = 0;
    move_cell.clear();
    int area_boundary = (A_area + B_area)/10 ;
    
    A_bucket_list_count = A_cell_count;
    B_bucket_list_count = B_cell_count;
    init_gain();
    create_bucket_list();
    

    while( FM_round < C ){

        //cout<<FM_round<<" "<<endl;;
        //cout<<A_bucket_list_count<<" "<<B_bucket_list_count<<endl;
        //show_bucket_list();

        
        if( A_bucket_list_count > 0 && B_bucket_list_count == 0){
            //cout<<"only A"<<endl;
            A_find_max_gain();
            update_gain(A_max);
            
        }
        else if(A_bucket_list_count == 0 && B_bucket_list_count > 0 ){
            //cout<<"only B"<<endl;
            B_find_max_gain();
            update_gain(B_max);
            
        }
        else if( A_bucket_list_count > 0 && B_bucket_list_count > 0 ){
            //cout<<"both A B"<<endl;
            A_find_max_gain();
            B_find_max_gain();
            int gain_A = A_max ->cell_gain;
            int gain_B = B_max ->cell_gain;
            

            if(gain_A > gain_B){
                if( change_area_from_A_to_B(A_max->cell_size) < area_boundary ){
                    update_gain(A_max);
                }
                else if( change_area_from_B_to_A(B_max->cell_size) < area_boundary){
                    update_gain(B_max);
                }
                
            }
            else if ( gain_A == gain_B){
                int a_change = change_area_from_A_to_B(A_max->cell_size);
                int b_change = change_area_from_B_to_A(B_max->cell_size);
                if( a_change < area_boundary && b_change < area_boundary ){
                    if( A_max->cell_size < B_max->cell_size ){
                        update_gain(A_max);
                    }
                    else{
                        update_gain(B_max);
                    }
                }
                else if( a_change < area_boundary && b_change >= area_boundary ){
                    update_gain(A_max);
                }
                else if( b_change < area_boundary && a_change >= area_boundary ){
                    update_gain(B_max);
                }
                else{
                    //cout<<"update error"<<endl;
                    break;
                }
                

            }
            else{
                
                if( change_area_from_B_to_A(B_max->cell_size) < area_boundary){
                    update_gain(B_max);
                }
                else if( change_area_from_A_to_B(A_max->cell_size) < area_boundary ){
                    update_gain(A_max);
                }
            }

        }
        else{
            //cout<<"no A B"<<endl;
            break;
        }
        if(accumulate_gain < best_gain){
            int diff = best_gain - accumulate_gain;
            //cout<<diff<<endl;
            if(diff > cut_size/100){
                break;
            }
        }
        FM_round ++;

    }
        
    if( best_gain > 0 ){
        //cout<<FM_round<<endl;
        cout<<"The "<<best_round<<" round is the best round with gain "<<best_gain<<endl;
        reset(best_round);
        A_cell_count = best_A_cell_count;
        B_cell_count = best_B_cell_count;

        A_area = best_A_area;
        B_area = best_B_area;

        //cout<<A_cell_count<<" "<<B_cell_count<<endl;
        //get_cutsize();
        //cout<<"cutsize : "<<cut_size<<endl;

        FM_algorithm();
    }

    else{
        //reset(best_round);
        //cout<<"cut size : ";
        get_cutsize();
        cout<<"cutsize : "<<cut_size<<endl;
        cout<<"FM algorithm stop..."<<endl<<endl;
        return;
    }
    
}

void reset(int best_round){

    
    for(int i  = 0 ;i < C ; i++){

        init_cell[i]->cell_gain = 0;
        init_cell[i]->lock = 0;
    }

    //cout<<"before reset ";
    //get_cutsize();
    //cout<<cut_size<<endl;
    
    //cout<<"moved cell : ";
    for(int i = 0  ; i <= best_round  ; i++){
        //cout<<move_cell[i]->cell_name<<" "<<move_cell[i]->cell_gain<<endl;
        if(move_cell[i]->cell_set == 0 ){
            move_cell[i]->cell_set = 1;
        }
        else{
            move_cell[i]->cell_set = 0;
        }
        //cout<<"after "<<move_cell[i]->cell_set<<endl;
        //get_cutsize();
        //cout<<"cut size : "<<cut_size<<endl;
    }
    
    
    //cout<<endl;
    update_net();
        

}

void init_gain(void){
    
    for( int i = 0 ; i < init_cell.size(); i++){
        init_cell[i]->cell_gain = 0;
        for(int j = 0 ; j < init_cell[i]->connected_net.size(); j++){
            
            string name = init_cell[i]->connected_net[j];

            if(init_cell[i]->cell_set == 0){
                //for cell in set A

                if( net_arr[name]->A == 1){
                    init_cell[i]->cell_gain ++;
                }
                
                if( net_arr[name]->B == 0){
                    init_cell[i]->cell_gain --;
                }

            }

            else if(init_cell[i]->cell_set == 1){
                // for cell in set B

                if( net_arr[name]->B == 1){
                    init_cell[i]->cell_gain ++;
                }

                if( net_arr[name]->A == 0){
                    init_cell[i]->cell_gain --;
                }
            }
            
        }
        
        
    }
    //show_cell_gain();
}

void show_cell_gain(void){

    for(int i = 0 ; i < init_cell.size() ; i++){
        cout<<init_cell[i]->cell_name<<"'s gain : "<<init_cell[i]->cell_gain<<endl;
    }
    cout<<endl;
}

void create_bucket_list(void){
    
    A_bucket_list.clear();
    B_bucket_list.clear();

    for(int i = -P_max ; i <= P_max ; i++){
        A_bucket_list[i].clear(); //A set
        B_bucket_list[i].clear(); //B set
    }

    for(int i = 0 ; i < C ; i++ ){
        insert(init_cell[i]);
    }
    //show_bucket_list();
}

void insert(Cell_node* c){

    string name = c->cell_name;
    int gain = c->cell_gain;
    int set = c->cell_set;
    
    if( set == 0){
        
        A_bucket_list[gain].insert(make_pair(name , c));
    }
    else{
        B_bucket_list[gain].insert(make_pair(name , c));

    }
    

}

void show_bucket_list(void){
    

    cout<<"A_bucket list : "<<endl;
    for(int i = P_max ; i >= -P_max ; i--){

        cout<<i<<" ";
        if(A_bucket_list[i].size() > 0 ){
            
            map<string , Cell_node*>::iterator iter;
            for(iter = A_bucket_list[i].begin(); iter != A_bucket_list[i].end(); iter++){
                cout<<" -> "<<iter->first<<"("<<iter->second->cell_size <<")";
                
            }
            
        }
        cout<<endl;
        
    }
    cout<<endl;

    cout<<"B_bucket list : "<<endl;
    for(int i = P_max ; i >= -P_max ; i--){

        cout<<i<<" ";
        if(B_bucket_list[i].size() > 0 ){

            map<string , Cell_node*>::iterator iter;
            for(iter = B_bucket_list[i].begin(); iter != B_bucket_list[i].end(); iter++){
                cout<<" -> "<<iter->first<<"("<<iter->second->cell_size<<")";
            }
            
        }
        cout<<endl;
        
    }
    cout<<endl;
}

void A_find_max_gain(void){

    int i = P_max;
    int flag = 0;
    int area_boundary = (A_area + B_area)/10 ;
    A_max = NULL;
 
    while( i >= -P_max){
        //cout<<i<<" ";
        

        if(A_bucket_list[i].size() > 0){
            
            A_max = cell_arr[A_bucket_list[i].begin()->first];
            flag = 1;
            
            
        }
        
        if(flag == 1){
            break;
        }
        i--;
    }
    if(flag == 0){
        cout<<"no cell in A bucket list"<<endl;
    }
    
}

void B_find_max_gain(void){

    int i = P_max;
    int flag = 0;
    int area_boundary = (A_area + B_area)/10 ;
    B_max = NULL;

    
    
    while( i >= -P_max){
        //cout<<i<<" ";
        

        if(B_bucket_list[i].size() > 0){
            
            B_max = cell_arr[B_bucket_list[i].begin()->first];
            flag = 1;
            
            
        }
        
        if(flag == 1){
            break;
        }
        i--;
    }
}

int change_area_from_A_to_B(int size){

    
    return abs( A_area - B_area - 2*size );

}

int change_area_from_B_to_A(int size){

    return abs( B_area - A_area - 2*size );

}

void update_gain(Cell_node *c){

    change_gain_cell.clear();

    //int cell_count = 0;
    //cout<<"Base cell is "<<c->cell_name<<" with gain "<<c->cell_gain<<endl;
    accumulate_gain = accumulate_gain + c->cell_gain;
    move_cell.push_back(c);
    // add c's gain to accumulate gain

    c->lock = 1;// lock c

    if(c->cell_set == 0){
        A_bucket_list_count--;
        

        for(int i = 0 ; i < c->connected_net.size() ; i++){
            //calculate A and B of nets which connected to cell c

            string name_n = c->connected_net[i];

            if( net_arr[name_n]->B == 0 ){
                // no cell in set B => all other cells connected to this net gain++
                
                for(int j = 0; j < net_arr[name_n]->connected_cell.size() ; j++){
                    
                    string name_c = net_arr[name_n]->connected_cell[j];

                    if( cell_arr[name_c]->lock == 0){
                        //cell is not locked
                        //cout<<name_c<<"'s gain ++"<<endl;
                        if(cell_arr[name_c]->gain_change == 0){
                            //cell_count++;
                            change_gain_cell.push_back(cell_arr[name_c]);
                        }
                        else if(cell_arr[name_c]->gain_change == -1){
                            //cell_count--;
                            cout<<"cell count --"<<endl;
                            for(vector<Cell_node*>::iterator iter=change_gain_cell.begin(); iter!=change_gain_cell.end(); iter++){
                                if( *iter == cell_arr[name_c] ){
                                    iter = change_gain_cell.erase(iter);
                                }
                                    
                                        
                            }
                        }
                        cell_arr[name_c]->gain_change++;

                        //insert c into set A with new gain
                        //move(cell_arr[name_c]);

                    }

                }

            }

            else if( net_arr[name_n]->B == 1 ){
                // one cell in set B => the cell in B gain --
                // become a normal net
                
                for(int j = 0; j < net_arr[name_n]->connected_cell.size() ; j++){
                    
                    string name_c = net_arr[name_n]->connected_cell[j];

                    if( cell_arr[name_c]->lock == 0 && cell_arr[name_c]->cell_set == 1){
                        //cell is not locked

                        //cout<<name_c<<"'s gain --"<<endl;
                        if(cell_arr[name_c]->gain_change == 0){
                            //cell_count++;
                            change_gain_cell.push_back(cell_arr[name_c]);
                        }
                        else if(cell_arr[name_c]->gain_change == 1){
                            //cell_count--;
                            cout<<"cell count --"<<endl;
                            for(vector<Cell_node*>::iterator iter=change_gain_cell.begin(); iter!=change_gain_cell.end(); iter++){
                                if( *iter == cell_arr[name_c] ){
                                    iter = change_gain_cell.erase(iter);
                                }
                                    
                                        
                            }
                        }
                        cell_arr[name_c]->gain_change--;

                        //insert c into set B with new gain
                        //move(cell_arr[name_c]);

                    }

                }

            }

            
            net_arr[name_n]->A --;
            net_arr[name_n]->B ++;
            //cout<<c->cell_name<<" move to B set"<<endl;
            //c->cell_set = 1;
            //F(n) = F(n) - 1
            //T(n) = T(n) + 1

            if( net_arr[name_n]->A == 0 ){
                
                for(int j = 0; j < net_arr[name_n]->connected_cell.size() ; j++){
                    
                    string name_c = net_arr[name_n]->connected_cell[j];

                    if( cell_arr[name_c]->lock == 0  ){
                        //cell is not locked

                        //cout<<name_c<<"'s gain --"<<endl;
                        if(cell_arr[name_c]->gain_change == 0){
                            //cell_count++;
                            change_gain_cell.push_back(cell_arr[name_c]);
                        }
                        else if(cell_arr[name_c]->gain_change == 1){
                            //cell_count--;
                            cout<<"cell count --"<<endl;
                            for(vector<Cell_node*>::iterator iter=change_gain_cell.begin(); iter!=change_gain_cell.end(); iter++){
                                if( *iter == cell_arr[name_c] ){
                                    iter = change_gain_cell.erase(iter);
                                }
                                        
                            }
                        }
                        
                        cell_arr[name_c]->gain_change--;

                        //insert c into set B with new gain
                        //move(cell_arr[name_c]);

                    }

                }
            }

            else if( net_arr[name_n]->A == 1){
                // net only have one cell in set A (that's cell c) ==> cells in A gain++
                

                for(int j = 0; j < net_arr[name_n]->connected_cell.size() ; j++){
                    
                    string name_c = net_arr[name_n]->connected_cell[j];
                    

                    if( cell_arr[name_c]->lock == 0 && cell_arr[name_c]->cell_set == 0 ){
                        //cell is not locked
                        //cout<<name_n<<" ";
                        
                        //cout<<name_c<<"'s gain ++"<<endl;
                        if(cell_arr[name_c]->gain_change == 0){
                            //cell_count++;
                            change_gain_cell.push_back(cell_arr[name_c]);
                        }
                        else if(cell_arr[name_c]->gain_change == -1){
                            //cell_count--;
                            cout<<"cell count --"<<endl;
                            for(vector<Cell_node*>::iterator iter=change_gain_cell.begin(); iter!=change_gain_cell.end(); iter++){
                                if( *iter == cell_arr[name_c] ){
                                    iter = change_gain_cell.erase(iter);
                                }
                                    
                                        
                            }
                        }
                        
                        cell_arr[name_c]->gain_change++;

                        //insert c into set B with new gain
                        //move(cell_arr[name_c]);

                    }

                }

            }

        }
        
        remove(c);
        //c->cell_set = 1;
        A_area = A_area - c->cell_size;
        B_area = B_area + c->cell_size;
        A_cell_count--;
        B_cell_count++;
        //update area
        
    }
    
    else if(c->cell_set == 1){
        //move c from B to A
        B_bucket_list_count--;
        
        for(int i = 0 ; i < c->connected_net.size() ; i++){
            //calculate A and B of nets which connected to cell c

            string name_n = c->connected_net[i];

            if( net_arr[name_n]->A == 0 ){
                // no cell in set A => all other cells (in B set) connected to this net gain++
                
                for(int j = 0; j < net_arr[name_n]->connected_cell.size() ; j++){
                    
                    string name_c = net_arr[name_n]->connected_cell[j];

                    if( cell_arr[name_c]->lock == 0){
                        //cell is not locked

                        //cout<<name_n<<" ";
                        //cout<<name_c<<"'s gain ++"<<endl;
                        if(cell_arr[name_c]->gain_change == 0){
                            //cell_count++;
                            change_gain_cell.push_back(cell_arr[name_c]);
                        }
                        else if(cell_arr[name_c]->gain_change == -1){
                            //cell_count--;
                            cout<<"cell count --"<<endl;
                            for(vector<Cell_node*>::iterator iter=change_gain_cell.begin(); iter!=change_gain_cell.end(); iter++){
                                if( *iter == cell_arr[name_c] ){
                                    iter = change_gain_cell.erase(iter);
                                }
                                                
                            }
                        }
                        cell_arr[name_c]->gain_change++;

                        //insert c into set B with new gain
                        //move(cell_arr[name_c]);

                    }

                }

            }

            else if( net_arr[name_n]->A == 1 ){
                // one cell in set A => the cells in A gain --
                // become a normal net

                for(int j = 0; j < net_arr[name_n]->connected_cell.size() ; j++){
                    
                    string name_c = net_arr[name_n]->connected_cell[j];

                    if( cell_arr[name_c]->lock == 0 && cell_arr[name_c]->cell_set == 0){
                        //cell is not locked

                        //cout<<name_n<<" ";
                        //cout<<name_c<<"'s gain --"<<endl;
                        if(cell_arr[name_c]->gain_change == 0){
                            //cell_count++;
                            change_gain_cell.push_back(cell_arr[name_c]);
                        }
                        else if(cell_arr[name_c]->gain_change == 1){
                            //cell_count--;
                            cout<<"cell count --"<<endl;
                            for(vector<Cell_node*>::iterator iter=change_gain_cell.begin(); iter!=change_gain_cell.end(); iter++){
                                if( *iter == cell_arr[name_c] ){
                                    iter = change_gain_cell.erase(iter);
                                }
                                    
                                        
                            }
                        }
                        cell_arr[name_c]->gain_change--;

                        //insert c into set B with new gain
                        //move(cell_arr[name_c]);

                    }

                }

            }

            net_arr[name_n]->A ++;
            net_arr[name_n]->B --;
            //cout<<c->cell_name<<" move to A set"<<endl;
            //c->cell_set = 0;
            //F(n) = F(n) - 1
            //T(n) = T(n) + 1
            
            
            if( net_arr[name_n]->B == 0){
                //no cell in set B

                for(int j = 0; j < net_arr[name_n]->connected_cell.size() ; j++){
                    
                    string name_c = net_arr[name_n]->connected_cell[j];
                    
                    if( cell_arr[name_c]->lock == 0  ){
                        //cell is not locked

                        //cout<<name_n<<" ";
                        //cout<<name_c<<"'s gain --"<<endl;
                        if(cell_arr[name_c]->gain_change == 0){
                            //cell_count++;
                            change_gain_cell.push_back(cell_arr[name_c]);
                        }
                        else if(cell_arr[name_c]->gain_change == 1){
                            //cell_count--;
                            cout<<"cell count --"<<endl;
                            for(vector<Cell_node*>::iterator iter=change_gain_cell.begin(); iter!=change_gain_cell.end(); iter++){
                                if( *iter == cell_arr[name_c] ){
                                    iter = change_gain_cell.erase(iter);
                                }
 
                                        
                            }
                        }
                        cell_arr[name_c]->gain_change--;

                        //insert c into set B with new gain
                        //move(cell_arr[name_c]);

                    }

                }
            }
             
            else if( net_arr[name_n]->B == 1){
                // net only have one cell in set B (that's cell c) ==> cells in A gain++
                
                for(int j = 0; j < net_arr[name_n]->connected_cell.size() ; j++){
                    
                    string name_c = net_arr[name_n]->connected_cell[j];

                    if( cell_arr[name_c]->lock == 0 && cell_arr[name_c]->cell_set == 1 ){
                        //cell is not locked

                        //cout<<name_c<<"'s gain++"<<endl;
                        if(cell_arr[name_c]->gain_change == 0){
                            //cell_count++;
                            change_gain_cell.push_back(cell_arr[name_c]);
                        }
                        else if(cell_arr[name_c]->gain_change == -1){
                            //cell_count--;
                            cout<<"cell count --"<<endl;
                            for(vector<Cell_node*>::iterator iter=change_gain_cell.begin(); iter!=change_gain_cell.end(); iter++){
                                if( *iter == cell_arr[name_c] ){
                                    iter = change_gain_cell.erase(iter);
                                }
                                        
                            }
                            
                        }
                        cell_arr[name_c]->gain_change++;

                        //insert c into set B with new gain
                        //move(cell_arr[name_c]);

                    }

                }

            }

        }
        
        remove(c);
        
        //c->cell_set = 0;
        A_area = A_area + c->cell_size;
        B_area = B_area - c->cell_size;
        A_cell_count++;
        B_cell_count--;
        //update area

        //B_unlock_cell_count--;

    }

    
    if(change_gain_cell.size() != 0){
        //cout<<change_gain_cell.size()<<endl;
        for(int i = 0 ; i < change_gain_cell.size() ; i++){
            //cout<<change_gain_cell[i]->cell_name<<" ";
            move(change_gain_cell[i]);

        }
    }
    
    //cout<<"accumulate_gain = "<<accumulate_gain<<endl;
    if( accumulate_gain > best_gain ){
        
        //cout<<"reach best gain"<<endl;
        best_round = FM_round;
        best_gain = accumulate_gain;
        best_balance = abs(A_area - B_area);
        best_A_cell_count = A_cell_count;
        best_A_area = A_area;
        best_B_cell_count = B_cell_count;
        best_B_area = B_area;
    }
    else if( accumulate_gain == best_gain ){
        int balance = abs(A_area - B_area);
        if(balance < best_balance){
            
            best_round = FM_round;
            best_balance = balance;
            best_A_cell_count = A_cell_count;
            best_A_area = A_area;
            best_B_cell_count = B_cell_count;
            best_B_area = B_area;

        }
    }
    
}

void move(Cell_node* c){

    //cout<<"move step"<<endl;
    string name = c->cell_name ;
    int set = c->cell_set;
    int gain = c->cell_gain;
    //cout<<name<<" "<<set<<" "<<gain<<endl;
    multimap<string , Cell_node*>::iterator iter;
    if( set == 0){

        A_bucket_list[gain].erase(name);
        
        
    }
    else{

        B_bucket_list[gain].erase(name);
    }
    

    c->cell_gain = c->cell_gain + c->gain_change;
    c->gain_change = 0;
    
    gain = c->cell_gain;
    //cout<<"new gain"<<gain<<endl;
    // move to set A
    //cout<<"insert "<<name<<endl;
    if(set == 0){
        
        A_bucket_list[gain].insert(make_pair(name , c));
    }
    // move to set B
    else{
        
        B_bucket_list[gain].insert(make_pair(name , c));
    }
    

}

void remove(Cell_node* c){

    //cout<<"remove step"<<endl;
    string name = c->cell_name;
    int set = c->cell_set;
    int gain = c->cell_gain;
    //cout<<name<<" "<<set<<" "<<gain<<endl;
    
    multimap<string , Cell_node*>::iterator iter;
    if( set == 0){
        A_bucket_list[gain].erase(name);
    }
    else{
        B_bucket_list[gain].erase(name);
    }

    

}

void output(char filename[]){

    int best_A_set_cell_count = 0 , best_B_set_cell_count = 0;
    for(int i = 0 ; i < C ; i++){
        if(init_cell[i]->cell_set == 0){
            best_A_set_cell_count++;
        }
        else{
            best_B_set_cell_count++;
        }
    }

    //cout<<datapath<<endl;

    fstream file;
    file.open(filename , ios::out);

    get_cutsize();
    file<<"cut_size "<<cut_size <<endl;
    file<<"A "<< best_A_set_cell_count<<endl;
    for(int i = 0 ; i < C; i++){
        if(init_cell[i]->cell_set == 0){
            file<<init_cell[i]->cell_name<<endl;
        }
    }

    file<<"B "<< best_B_set_cell_count<<endl;
    for(int i = 0 ; i < C; i++){
        if(init_cell[i]->cell_set == 1){
            file<<init_cell[i]->cell_name<<endl;
        }
    }

    file.close();

}
