#ifndef EXAMPLEFUNCTION_H
#define EXAMPLEFUNCTION_H
#include "Placement.h"
#include "NumericalOptimizerInterface.h"

class ExampleFunction : public NumericalOptimizerInterface
{
public:
    ExampleFunction(Placement &placement);
	Placement &_placement;
	
    void evaluateFG(const vector<double> &x, double &f, vector<double> &g);
    void evaluateF(const vector<double> &x, double &f);
    unsigned dimension();
	
	
	double Width; // the width of the placement
	double Height; // the heigh of the placement
	double Area; // the total area of the placement
	int module_num; // number of modules in the placement
	int net_num; // number of net in the placement

	//*****************************************************************************************************//
	double alpha; // Smoothing parameter
	double *exp_arr; // 4 exp results per module

	// parameters of LSE
	double x_pos; 
	double x_neg;
	double y_pos;
	double y_neg;
	
	//*****************************************************************************************************//
	double beta; // for density
	int bin_num; // number of bins in the edge
	double density_r; // density ratio
	double *bin_density;
	double target_density;

	double bin_width; // width of a bin
	double bin_height; // height of a bin

	double bin_center_x; // x coor of center of bin
	double bin_center_y; // y coor of center of bin

	double dx; // dx = |xi - xb|
	double dy; // dy = |yi - yb|

	// parameters for overlap function 
	double x_a;
	double x_b;
	double y_a;
	double y_b;

	// results of overlap function
	double overlap_x;
	double overlap_y; 
	double *overlap_g; // 2 grad (x , y) for a module 


};
#endif // EXAMPLEFUNCTION_H
