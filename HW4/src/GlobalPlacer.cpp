#include <iostream>
#include "GlobalPlacer.h"
#include "ExampleFunction.h"
#include "NumericalOptimizer.h"

using namespace std;

GlobalPlacer::GlobalPlacer(Placement &placement)
	:_placement(placement)
{

}

void GlobalPlacer::randomPlace(vector<double> &x){
	double w = _placement.boundryRight() - _placement.boundryLeft();
    double h = _placement.boundryTop() - _placement.boundryBottom();
    for (size_t i = 0; i < _placement.numModules(); ++i)
    {
        double wx = _placement.module(i).width(),
               hx = _placement.module(i).height();
        double px = (int)rand() % (int)(w - wx) + _placement.boundryLeft();
        double py = (int)rand() % (int)(h - hx) + _placement.boundryBottom();
        _placement.module(i).setPosition(px, py);

        x[2 * i] = px;            // write back to solution vector
        x[(2 * i) + 1] = py;      // write back to solution vector
    }
}


void GlobalPlacer::place()
{
	//*****************************************************************************************************//
    //initial example function
	ExampleFunction ef(_placement); 

    //*****************************************************************************************************//

    vector<double> solution_v(ef.dimension());  // create the solution vector
	srand(1234567);
	randomPlace(solution_v);  //generate the value of solution vectors for modules

    //*****************************************************************************************************//

    NumericalOptimizer no(ef);

    int right = _placement.boundryRight();
    int left = _placement.boundryLeft();
    int top = _placement.boundryTop();
    int bottom = _placement.boundryBottom();
    int Width = right - left;
    int Height = top - bottom;

    for(int i = 0 ; i < 2 ; i++){
        cout<<i<<" th round"<<endl<<endl;

        ef.beta += 2000; // beta increase 2000 every round
        no.setX(solution_v);
        no.setNumIteration( 40 ); // user-specified parameter
	    //no.setNumIteration( _placement.numModules() / 500); // user-specified parameter
        
        no.setStepSizeBound( max(Width , Height) * 7); // user-specified parameter
        no.solve();             // Conjugate Gradient solver


        // check all of modules are in the boundary
        int module_num = _placement.numModules();
        for(int j = 0 ; j < module_num ; j++){
            double module_x = solution_v[2 * j];
            double module_y = solution_v[2 * j + 1];
            double module_width = _placement.module(j).width();
            double module_height = _placement.module(j).height();

            int flag = 0;
            
            // if x coor is out of boundary
            if( module_x + module_width > right){
                module_x = right - module_width;
                flag = 1;
            } 
            else if (module_x - module_width > left){
                module_x = left + module_width;
                flag = 1;
            }

            // if y coor is out of boundary
            if( module_y + module_height > top){
                module_y = top - module_height;
                flag = 1;
            }
            else if( module_y - module_height < bottom){
                module_y = bottom + module_height;
                flag = 1;
            }

            // store the changed coor back to solution vector

            if( flag == 1){
                _placement.module(j).setCenterPosition(module_x , module_y);
                solution_v[2 * j] = module_x;
                solution_v[2 * j + 1] = module_y;
            }
            

        }

    }


	

    //*****************************************************************************************************//


	//cout << "Current solution:" << endl;
    //for (unsigned i = 0; i < no.dimension(); i++)
    //{
    //    cout << "x[" << i << "] = " << no.x(i) << endl;
    //}
    //cout << "Objective: " << no.objective() << endl;
	
	
	/* @@@ TODO 
	 * 1. Understand above example and modify ExampleFunction.cpp to implement the analytical placement
	 * 2. You can choose LSE or WA as the wirelength model, the former is easier to calculate the gradient
     * 3. For the bin density model, you could refer to the lecture notes
     * 4. You should first calculate the form of wirelength model and bin density model and the forms of their gradients ON YOUR OWN 
	 * 5. Replace the value of f in evaluateF() by the form like "f = alpha*WL() + beta*BinDensity()"
	 * 6. Replace the form of g[] in evaluateG() by the form like "g = grad(WL()) + grad(BinDensity())"
	 * 7. Set the initial vector x in main(), set step size, set #iteration, and call the solver like above example
	 * */
	
}


void GlobalPlacer::plotPlacementResult( const string outfilename, bool isPrompt )
{
    ofstream outfile(outfilename.c_str(), ios::out);
    outfile << " " << endl;
    outfile << "set title \"wirelength = " << _placement.computeHpwl() << "\"" << endl;
    outfile << "set size ratio 1" << endl;
    outfile << "set nokey" << endl
            << endl;
    outfile << "plot[:][:] '-' w l lt 3 lw 2, '-' w l lt 1" << endl
            << endl;
    outfile << "# bounding box" << endl;
    plotBoxPLT(outfile, _placement.boundryLeft(), _placement.boundryBottom(), _placement.boundryRight(), _placement.boundryTop());
    outfile << "EOF" << endl;
    outfile << "# modules" << endl
            << "0.00, 0.00" << endl
            << endl;
    for (size_t i = 0; i < _placement.numModules(); ++i)
    {
        Module &module = _placement.module(i);
        plotBoxPLT(outfile, module.x(), module.y(), module.x() + module.width(), module.y() + module.height());
    }
    outfile << "EOF" << endl;
    outfile << "pause -1 'Press any key to close.'" << endl;
    outfile.close();

    if (isPrompt)
    {
        char cmd[200];
        sprintf(cmd, "gnuplot %s", outfilename.c_str());
        if (!system(cmd))
        {
            cout << "Fail to execute: \"" << cmd << "\"." << endl;
        }
    }
}

void GlobalPlacer::plotBoxPLT( ofstream& stream, double x1, double y1, double x2, double y2 )
{
    stream << x1 << ", " << y1 << endl
           << x2 << ", " << y1 << endl
           << x2 << ", " << y2 << endl
           << x1 << ", " << y2 << endl
           << x1 << ", " << y1 << endl
           << endl;
}
