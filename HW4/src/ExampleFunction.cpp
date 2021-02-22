#include <iostream>
#include "ExampleFunction.h"
#include <cmath>
#include <cstdlib>

using namespace std;

// minimize 3*x^2 + 2*x*y + 2*y^2 + 7
// min W(x,y)+λΣ(Db(x,y)–Mb)^2

ExampleFunction::ExampleFunction(Placement &placement)
	:_placement(placement)
{
	
	Width =_placement.boundryRight() - _placement.boundryLeft();
	Height = _placement.boundryTop() - _placement.boundryBottom();
	Area = Width * Height;
	alpha = Width / 600;
	beta = 0;
	module_num = _placement.numModules();
	net_num = _placement.numNets();
	

	exp_arr = new double[module_num * 4]();
	x_pos = 0;  
	x_neg = 0;
	y_pos = 0;
	y_neg = 0;

	bin_num = 10;
	bin_width = Width / bin_num;
	bin_height = Height / bin_num;
	bin_density = new double[bin_num * bin_num]();
	x_b = 0;
	y_b = 0;
	overlap_g = new double[module_num * 2]();
	target_density = 0;
	for(int i = 0 ; i < module_num ; i++){
		target_density += _placement.module(i).width() * _placement.module(i).height();
	}
	target_density = target_density / Area;
	
}

void ExampleFunction::evaluateFG(const vector<double> &x, double &f, vector<double> &g){
	

	f = 0; // initialize the objective function
	double f1 = 0; // initialize the objection function of wirelength model
	double f2 = 0; // initialize the objection function of density function

	// initialize parameters for LSE
	x_pos = 0;
	x_neg = 0;
	y_pos = 0;
	y_neg = 0;

	// initial g
	int size_g = g.size();
	for(int i = 0 ; i < size_g ; i++){
		g[i] = 0;
	}

	//*****************************************************************************************************//
	//////////////////////
	// wirelength model //
	//////////////////////

	// calculate LSE
	for(int i = 0 ; i < module_num ; i++){
		Module cur = _placement.module(i);
		
		// calculate the parameters of LSE function
		exp_arr[4 * i] = exp( x[2 * i] / alpha );
		exp_arr[4 * i + 1] = exp( (-1) * ( x[2 * i] / alpha ) );
		exp_arr[4 * i + 2] = exp( x[2 * i + 1] / alpha );
		exp_arr[4 * i + 3] = exp( (-1) * (x[2 * i + 1] / alpha ) );

		//sum of those arameters
		x_pos += exp_arr[4 * i];
		x_neg += exp_arr[4 * i + 1];
		y_pos += exp_arr[4 * i + 2];
		y_neg += exp_arr[4 * i + 3];

	}

	
	
	// calculate gradient 
	for(int i = 0 ; i < net_num ; i++){

		int pin_num = _placement.net(i).numPins();
		for(int j = 0 ; j < pin_num ; j++){
			Module cur =  _placement.module(i);
			int cur_ID = _placement.net(i).pin(j).moduleId();

			// if module is fixed, g = 0
			if( cur.isFixed() == 1 ){
				g[ 2 * cur_ID ] = 0;
				g[ 2 * cur_ID + 1] = 0;
			}
			// if module is not fixed, calculate g with LSE parameters
			else{
				g[ 2 * cur_ID ] += ( exp_arr[ 4 * cur_ID ] / (alpha * x_pos) ) - ( exp_arr[ 4 * cur_ID + 1] / (alpha * x_neg) );
				g[ 2 * cur_ID + 1] += ( exp_arr[ 4 * cur_ID + 2] / (alpha * y_pos) ) - ( exp_arr[ 4 * cur_ID + 3] / (alpha * y_neg) );
			}

		}
		// implement the f1
		f1 += alpha *  ( log(x_pos) + log(x_neg) + log(y_pos) + log(y_neg) );  // LSE wirelength solution
	}


	// return when it is 1st iteration
	if(beta == 0){
		//cout<< "Return when it is 1st iteration"<<endl;
		return;
	}


	//*****************************************************************************************************//
	//////////////////////////
	// bin density function //
	//////////////////////////

	// initialize bin density
	for(int i = 0 ; i < bin_num * bin_num ; i++){
		bin_density[i] = 0;
	}


	// initialize overlap g
	for(int i = 0 ; i < module_num * 2 ; i++){
		overlap_g[i] = 0;
	}


	// smoothing by Bell-shaped function
	// total num of bins : bin_num * bin_num
	// testing which bin should each module to locate
	for(int i = 0 ; i < bin_num ; i++){

		for(int j = 0 ; j < bin_num ; j++){

			// calculate the coor of center of bin ( x_b , y_b )
			x_b = ( (i + 0.5) * bin_width ) + _placement.boundryLeft();
			y_b = ( (j + 0.5) * bin_height ) + _placement.boundryBottom();

			// try every module in bin[i][j]
			for(int k = 0 ; k < module_num ; k++){

				Module cur = _placement.module(k);
				double cur_width = cur.width(); // x coor of current module
				double cur_height = cur.height(); // y coor of current module
				
				// if not fixed, calculate the bin density and g of overlap
				if( cur.isFixed() == 0){

					// calculate a , b for x and y
					x_a = 4 / ( (cur_width + bin_width) * ( 2 * cur_width + bin_width) );
					x_b = 4 / ( cur_width * ( 2 * cur_width + bin_width) );
					y_a = 4 / ( (cur_height + bin_height) * ( 2 * cur_height + bin_height) );
					y_b = 4 / ( cur_height * ( 2 * cur_height + bin_height) );

					// calculate the dx = | xi - xb | and dy = | yi - yb |
					double xi = cur.centerX(); // xi = x[2 * k]
					double yi = cur.centerY(); // yi = x[2 * k + 1]
					dx = abs( xi - x_b );
					dy = abs( yi - y_b );

					double cond1_x = bin_width  + (cur_width / 2);
					double cond2_x = cond1_x + bin_width;
					double cond1_y = bin_height  + (cur_height / 2);
					double cond2_y = cond1_y + bin_height;


					// calculate overlap function for x coor
					if ( dx >= 0  && dx <= cond1_x ){
						
						overlap_x = 1 - ( x_a * pow( dx , 2 ) );

					}
					else if ( dx >= cond1_x && dx <= cond2_x){
						
						overlap_x = x_b * pow( ( dx - bin_width - ( cur_width / 2 ) ) , 2);

					}
					else if ( dx >= cond2_x ){

						overlap_x = 0;

					}
					else{
						cout<<"[error] Wrong dx in overlap function."<<endl;
					}


					// calculate overlap function for y coor
					if (dy >= 0 && dy <= cond1_y ){

						overlap_y = 1 - ( y_a * pow( dy , 2 ) );

					}
					else if( dy >= cond1_y && dy <= cond2_y ){

						overlap_y = y_b * pow( ( dy - bin_height - ( cur_height / 2 ) ) , 2);

					}
					else if ( dy >= cond2_y ){

						overlap_y = 0;

					}
					else{
						cout<<"[error] Wrong dy in overlap function."<<endl;
					}

					// calculate density ratio
					density_r = cur.area() / (bin_width * bin_height);


					// calculate g of overlap function for x coor
					if (dx <= cond1_x ){

						overlap_g[2 * k] = density_r * (-2) * x_a * dx * overlap_y;

					}
					else if( dx >= cond1_x && dx <= cond2_x ){

						overlap_g[2 * k] = density_r * 2 * x_b * (dx - ( bin_width + cur_width / 2 ) ) * overlap_y;

					}
					else{
						overlap_g[2 * k] = 0;
					}


					// calculate g of overlap function for y coor
					if (dy <= cond1_y ){

						overlap_g[2 * k + 1] = density_r * (-2) * y_a * dy * overlap_x;

					}
					else if( dy >= cond1_y && dy <= cond2_y ){

							overlap_g[2 * k + 1] = density_r * 2 * y_b * (dy - ( bin_height + cur_height / 2 ) ) * overlap_x;

					}
					else{
						overlap_g[2 * k + 1] = 0;
					}

				}

				// calculate bin density for bin[i][j]
				bin_density[ j * bin_num + i ] += density_r * overlap_x * overlap_y;

			}
		
			// calculate f2
			f2 += beta * pow(( bin_density[j * bin_num + i] - target_density) , 2);

			// calculate true grad
			for(int k = 0 ; k < module_num ; k++){
				g[2 * k] += beta * 2 * ( bin_density[j * bin_num + i] - target_density) * overlap_g[2 * k];
				g[2 * k + 1] += beta * 2 * ( bin_density[j * bin_num + i] - target_density) * overlap_g[2 * k + 1];
			}
		}
	}

	//*****************************************************************************************************//
	//////////////////////////
	// calculate F          //
	//////////////////////////
	f = f1 + f2;

}

void ExampleFunction::evaluateF(const vector<double> &x, double &f){

	f = 0; // initialize the objective function
	double f1 = 0; // initialize the objection function of wirelength model
	double f2 = 0; // initialize the objection function of density function

	// initialize parameters for LSE
	x_pos = 0;
	x_neg = 0;
	y_pos = 0;
	y_neg = 0;
	
	//*****************************************************************************************************//
	//////////////////////
	// wirelength model //
	//////////////////////

	// calculate LSE
	for(int i = 0 ; i < module_num ; i++){
		Module cur = _placement.module(i);
		//cout<<cur.isFixed() <<" "<< cur.centerX() <<" "<< cur.centerY()<<endl;

		

		exp_arr[4 * i] = exp( x[2 * i] / alpha );
		exp_arr[4 * i + 1] = exp( (-1) * ( x[2 * i] / alpha ) );
		exp_arr[4 * i + 2] = exp( x[2 * i + 1] / alpha );
		exp_arr[4 * i + 3] = exp( (-1) * (x[2 * i + 1] / alpha ) );

		

		x_pos += exp_arr[4 * i];
		x_neg += exp_arr[4 * i + 1];
		y_pos += exp_arr[4 * i + 2];
		y_neg += exp_arr[4 * i + 3];

	}

	for(int i = 0 ; i < net_num ; i++){

		f1 += alpha *  ( log(x_pos) + log(x_neg) + log(y_pos) + log(y_neg) );  // LSE wirelength solution
	}
	
	
	// return when it is 1st iteration
	if(beta == 0){
		//cout<< "Return when it is 1st iteration"<<endl;
		return;
	}

	//*****************************************************************************************************//
	//////////////////////////
	// bin density function //
	//////////////////////////

	// initial bin density
	for(int i = 0 ; i < bin_num * bin_num ; i++){
		bin_density[i] = 0;
	}

	for(int i = 0 ; i < bin_num ; i++){

		for(int j = 0 ; j < bin_num ; j++){

			// calculate the coor of center of bin ( x_b , y_b )
			x_b = ( (i + 0.5) * bin_width ) + _placement.boundryLeft();
			y_b = ( (j + 0.5) * bin_height ) + _placement.boundryBottom();

			// try every module in bin[j][i]
			for(int k = 0 ; k < module_num ; k++){

				Module cur = _placement.module(k);
				double cur_width = cur.width(); // x coor of current module
				double cur_height = cur.height(); // y coor of current module
				
				// if not fixed
				if( cur.isFixed() == 0){

					// calculate a , b for x and y
					x_a = 4 / ( (cur_width + bin_width) * ( 2 * cur_width + bin_width) );
					x_b = 4 / ( cur_width * ( 2 * cur_width + bin_width) );
					y_a = 4 / ( (cur_height + bin_height) * ( 2 * cur_height + bin_height) );
					y_b = 4 / ( cur_height * ( 2 * cur_height + bin_height) );

					// calculate the dx = | xi - xb | and dy = | yi - yb |
					double xi = cur.centerX(); // xi = x[2 * k]
					double yi = cur.centerY(); // yi = x[2 * k + 1]
					dx = abs( xi - x_b );
					dy = abs( yi - y_b );

					double cond1_x = (bin_width / 2) + (cur_width / 2);
					double cond2_x = bin_width + (cur_width / 2);
					double cond1_y = (bin_height / 2) + (cur_height / 2);
					double cond2_y = bin_height  + (cur_height / 2);


					// calculate overlap function for x coor
					if ( dx >= 0  && dx <= cond1_x ){
						
						overlap_x = 1 - ( x_a * pow( dx , 2 ) );

					}
					else if ( dx >= cond1_x && dx <= cond2_x){
						
						overlap_x = x_b * pow( ( dx - bin_width - ( cur_width / 2 ) ) , 2);

					}
					else if ( dx >= cond2_x ){

						overlap_x = 0;

					}
					else{
						cout<<"[error] Wrong dx in overlap function."<<endl;
					}


					// calculate overlap function for y coor
					if (dy >= 0 && dy <= cond1_y ){

						overlap_y = 1 - ( y_a * pow( dy , 2 ) );

					}
					else if( dy >= cond1_y && dy <= cond2_y ){

						overlap_y = y_b * pow( ( dy - bin_height - ( cur_height / 2 ) ) , 2);

					}
					else if ( dy >= cond2_y ){

						overlap_y = 0;

					}
					else{
						cout<<"[error] Wrong dy in overlap function."<<endl;
					}

					// calculate density ratio
					density_r = cur.area() / (bin_width * bin_height);
				}

				// calculate bin density for bin[j][i]
				bin_density[ j * bin_num + i ] += density_r * overlap_x * overlap_y;

			}
			

			// calculate f2
			f2 += beta * pow(( bin_density[j * bin_num + i] - target_density) , 2);

		}
	}

	f = f1 + f2;

}

unsigned ExampleFunction::dimension()
{
    return 2 * module_num;
}
