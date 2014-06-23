
// STL Includes
#include <iostream>
#include <ctime>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cmath>

#include <Cleaver/TetMesh.h>
#include <Cleaver/Cleaver.h>

#include <Python.h>

const double EPSILON = 1E-20;

typedef double (*ScalarTestingFunction)(const cleaver::BoundingBox &, const cleaver::vec3 &);
typedef cleaver::vec3 (*VectorTestingFunction)(const cleaver::BoundingBox &, const cleaver::vec3 &);


double randf()
{
    return (double)(rand()) / (double) RAND_MAX;
}

void plot2(const std::vector<float> &x, const std::vector<float> &y, const std::vector<float> &z, const std::string &filename)
{
    double plotWidth  = 3.0;  // inches
    double plotHeight = 3.0; // inches

    // create data strings
    std::stringstream x_ss;
    std::stringstream y_ss;
    std::stringstream z_ss;

    double x_max = 16;
    double y_max = 16;
    double max_value = 0;
    for(size_t i=0; i < z.size(); i++)
    {
        if(x[i] > x_max)
            x_max = x[i];
        if(y[i] > y_max)
            y_max = y[i];
        if(z[i] > max_value)
            max_value = z[i];
    }

    x_ss << "x = [";
    y_ss << "y = [";
    z_ss << "z = [";

    for(size_t i=0; i < x.size() - 1; i++)
    {
        x_ss << x[i] << ", ";
        y_ss << y[i] << ", ";
        z_ss << z[i] << ", ";
    }

    x_ss << x[x.size() - 1] << "];" << std::endl;
    y_ss << y[y.size() - 1] << "];" << std::endl;
    z_ss << z[z.size() - 1] << "];" << std::endl;

    std::string x_data_string = x_ss.str();
    std::string y_data_string = y_ss.str();
    std::string z_data_string = z_ss.str();

    //std::cout <<  x_data_string << std::endl;
    //std::cout <<  y_data_string << std::endl;
    //std::cout <<  z_data_string << std::endl;



    PyRun_SimpleString("import matplotlib.pyplot as plt");
    //PyRun_SimpleString("xyc = range(20)");
    PyRun_SimpleString(x_data_string.c_str());
    PyRun_SimpleString(y_data_string.c_str());
    PyRun_SimpleString(z_data_string.c_str());


    PyRun_SimpleString("plt.subplot(111)");

    std::stringstream scatter_ss;
    scatter_ss << "plt.scatter(x, y, c=z, s=8, vmin=0, vmax=" << max_value << ");" << std::endl;
    std::string scatter_string = scatter_ss.str();
    PyRun_SimpleString(scatter_string.c_str());


    PyRun_SimpleString("plt.set_cmap('afmhot');");
    PyRun_SimpleString("plt.colorbar();");

    std::stringstream xlimss;
    xlimss << "plt.xlim(0, " << x_max << ");";
    std::string xlim_string = xlimss.str();
    PyRun_SimpleString(xlim_string.c_str());

    std::stringstream ylimss;
    ylimss << "plt.ylim(0, " << y_max << ");";
    std::string ylim_string = ylimss.str();

    PyRun_SimpleString(ylim_string.c_str());
    PyRun_SimpleString("plt.title('Relative Error');");

    PyRun_SimpleString("fig1 = plt.gcf();");

    // Get Python to save to designated file name
    std::stringstream fnss;
    fnss << "fig1.savefig('" << filename << ".png', dpi=150);";
    std::string saveFigureString = fnss.str();

    PyRun_SimpleString(saveFigureString.c_str());
    PyRun_SimpleString("plt.show();");    
}


void plot3(int width, int height, const std::vector<float> &values, const std::string &plotname)
{    

    PyRun_SimpleString("import numpy as np");
    PyRun_SimpleString("import matplotlib.pyplot as plt");
    PyRun_SimpleString("import matplotlib.cm as cm");

    std::stringstream array_init_ss;
    array_init_ss << "X = np.zeros([" << width << "," << height << "]);" << std::endl;
    std::string array_init_string = array_init_ss.str();
    PyRun_SimpleString(array_init_string.c_str());

    std::stringstream value_ss;
    int idx = 0;
    for(int j=0; j < height; j++)
    {
        for(int i=0; i < width; i++)
        {
            value_ss << "X[" << i << "][" << j << "] = " << values[idx++] << std::endl;
            PyRun_SimpleString(value_ss.str().c_str());
            value_ss.str("");
            value_ss.clear();
        }
    }

    //PyRun_SimpleString("X[0][0] = 1.75");  // this line verifies origin is bottom left

    PyRun_SimpleString("fig, ax = plt.subplots()");
    PyRun_SimpleString("imgplot = ax.imshow(X, cmap=cm.gist_heat, origin='lower', interpolation='nearest', vmin=0., vmax=1.)");

    PyRun_SimpleString("fig.colorbar(imgplot);");
    PyRun_SimpleString("ax.set_title('Relative Error')");

    PyRun_SimpleString("numrows, numcols = X.shape");

    std::string format_coord_function_string = "def format_coord(x, y):\n"
            "    col = int(x+0.5);\n"
            "    row = int(y+0.5);\n"
            "    if col>=0 and col<numcols and row>=0 and row<numrows:\n"
            "        z = X[row,col]; \n"
            "        return 'x=%1.4f, y=%1.4f, z=%1.4f'%(x, y, z); \n"
            "    else:\n"
            "        return 'x=%1.4f, y=%1.4f'%(x, y); \n\n"
            "ax.format_coord = format_coord";

    //std::cout << format_coord_function_string << std::endl;
    PyRun_SimpleString(format_coord_function_string.c_str());


    //PyRun_SimpleString("plt.show()");

    //PyRun_SimpleString("plt.set_cmap('afmhot');");
    //PyRun_SimpleString("plt.colorbar();");

    // Get Python to save to designated file name
    PyRun_SimpleString("fig1 = plt.gcf();");

    std::stringstream ftss;
    ftss << "fig1.suptitle('" << plotname << "', fontsize=20);";
    std::string figureTitleString = ftss.str();
    PyRun_SimpleString(figureTitleString.c_str());

    std::stringstream fnss;
    fnss << "fig1.savefig('" << plotname << ".png', dpi=150);";
    std::string saveFigureString = fnss.str();

    PyRun_SimpleString(saveFigureString.c_str());

    PyRun_SimpleString("plt.show()");
}

ScalarTestingFunction analyticFunction;
VectorTestingFunction analyticGradient;


double Linear0_Function(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{
    double r = pos.x;

    return r;
}

cleaver::vec3 Linear0_Gradient(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{

    //cleaver::vec3 dir = (bounds.center() - pos);
    cleaver::vec3 dir(pos.x, 0, 0);
    dir += cleaver::vec3(EPSILON, EPSILON, EPSILON);
    double r = L2(dir);
    dir = dir / r;

    cleaver::vec3 force = dir;
    //cleaver::vec3 force = 2*-0.02*r*dir;

    return force;
}


double Linear1_Function(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{
    cleaver::vec3 center(100, 100, 100);

    //double r = L2(pos - bounds.center()) + EPSILON;
    double r = L2(pos - center) + EPSILON;

    //double u = 0.08*r - 1000;
    double u = 0.02*r*r;

    // add 1E-4 relative random noise
//    float noise = (randf()-0.5)*1E-8;     // effectively +- 1E-3
//    u = (1 + noise)*u;

    return u;
}

cleaver::vec3 Linear1_Gradient(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{
    cleaver::vec3 center(100, 100, 100);

    //cleaver::vec3 dir = (bounds.center() - pos);
    cleaver::vec3 dir = (center - pos);
    dir += cleaver::vec3(EPSILON, EPSILON, EPSILON);
    double r = L2(dir);
    dir = dir / r;

    //cleaver::vec3 force = -0.08*dir;
    cleaver::vec3 force = 2*-0.02*r*dir;

    return force;
}

double Linear2_Function(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{
    cleaver::vec3 center(100, 100, 100);

    double r = std::abs(pos.x - center.x);

    double u = 0.08*r - 1000;

    return u;
}

cleaver::vec3 Linear2_Gradient(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{
    cleaver::vec3 center(100, 100, 100);
    center = bounds.center();

    cleaver::vec3 dir;
    dir.x = (center.x - pos.x);
    dir.y = dir.z = 0;
    dir += cleaver::vec3(EPSILON, EPSILON, EPSILON);
    double r = L2(dir);
    dir = dir / r;

    cleaver::vec3 force = -0.08*dir;
    //cleaver::vec3 force = 2*-0.02*r*dir;

    return force;
}


cleaver::FloatField* createFieldFromFunction(ScalarTestingFunction valueAt, const cleaver::BoundingBox &bounds)
{
    int w = bounds.size.x;
    int h = bounds.size.y;
    int d = bounds.size.z;
    float *data = new float[w*h*d];

    unsigned int index = 0;
    for(int k=0; k < d; k++)
    {
        for(int j=0; j < h; j++)
        {
            for(int i=0; i < w; i++)
            {
                data[index] = valueAt(bounds, cleaver::vec3(i+0.5, j+0.5, k+0.5));
                std::cout << data[index] << " ";
                index++;
            }
        }
    }

    cleaver::FloatField *field = new cleaver::FloatField(data, w, h, d);

    return field;
}

std::vector<cleaver::FloatField*> createGradientFieldsFromFunction(VectorTestingFunction valueAt, const cleaver::BoundingBox &bounds)
{
    int w = bounds.size.x;
    int h = bounds.size.y;
    int d = bounds.size.z;

    float *x_data = new float[w*h*d];
    float *y_data = new float[w*h*d];
    float *z_data = new float[w*h*d];

    unsigned int index = 0;
    for(int k=0; k < d; k++)
    {
        for(int j=0; j < h; j++)
        {
            for(int i=0; i < w; i++)
            {
                cleaver::vec3 value = valueAt(bounds, cleaver::vec3(i+0.5, j+0.5, k+0.5));
                x_data[index] = value.x;
                y_data[index] = value.y;
                z_data[index] = value.z;
                index++;
            }
        }
    }

    cleaver::FloatField *x_field = new cleaver::FloatField(x_data, w, h, d);
    cleaver::FloatField *y_field = new cleaver::FloatField(y_data, w, h, d);
    cleaver::FloatField *z_field = new cleaver::FloatField(z_data, w, h, d);

    std::vector<cleaver::FloatField*> gradient;
    gradient.push_back(x_field);
    gradient.push_back(y_field);
    gradient.push_back(z_field);

    return gradient;
}

enum SamplingMode { RegularSampling, RandomSampling };
enum InterpolationMode { Nearest, Bilinear, Bicubic };

void evaluateErrorBetweenReferenceAndTestFunction(VectorTestingFunction testFunction, VectorTestingFunction referenceFunction,
                                                  const cleaver::BoundingBox &bounds, SamplingMode samplingMode, unsigned int numberOfSamples,
                                                  const std::string &plotname)
{
    double min_abs_error = 10e16;
    double max_abs_error = 0;
    double avg_abs_error = 0;

    double min_rel_error = 1;
    double max_rel_error = 0;
    double avg_rel_error = 0;

    double n = 1;

    std::vector<float> x_samples(numberOfSamples*numberOfSamples, 0);
    std::vector<float> y_samples(numberOfSamples*numberOfSamples, 0);
    std::vector<float> error_values(numberOfSamples*numberOfSamples, 0);

    if(samplingMode == RegularSampling)
    {
        double size = std::min(bounds.size.x / (float)numberOfSamples, bounds.size.y / (float)numberOfSamples);
        double x_interval = bounds.size.x / (float)numberOfSamples;
        double y_interval = bounds.size.y / (float)numberOfSamples;


        {
            for(int y_count = 0; y_count < numberOfSamples; y_count++)
            {
                for(int x_count = 0; x_count < numberOfSamples; x_count++)
                {
                    double x = 0.5*size + x_count*size;
                    double y = 0.5*size + y_count*size;
                    double z = 1.0;

                    // apply debug scaling
                    //x = 0.9*x + 0.05*bounds.size.x;
                    //y = 0.9*y + 0.05*bounds.size.y;
                    //

                    cleaver::vec3 val =       referenceFunction(bounds, cleaver::vec3(x,y,z));
                    cleaver::vec3 val_approx =     testFunction(bounds, cleaver::vec3(x,y,z));

                    double absolute_error = L2(val - val_approx);
                    double relative_error = absolute_error / L2(val);

                    //--- update min and max errors
                    if(absolute_error < min_abs_error)
                        min_abs_error = absolute_error;
                    if(absolute_error > max_abs_error)
                        max_abs_error = absolute_error;

                    if(relative_error < min_rel_error)
                        min_rel_error = relative_error;
                    if(relative_error > max_rel_error)
                        max_rel_error = relative_error;


                    //--- update average errors
                    //avg_abs_error = absolute_error
                    if(n > 1)
                    {
                        avg_abs_error = ((n-1)/n)*avg_abs_error + absolute_error/n;
                        avg_rel_error = ((n-1)/n)*avg_rel_error + relative_error/n;
                    }
                    else
                    {
                        avg_abs_error = absolute_error;
                        avg_rel_error = relative_error;
                    }

                    // save data for plotting
                    x_samples[n-1] = x;
                    y_samples[n-1] = y;
                    error_values[n-1] = relative_error;

                    n++;
                }
            }
        }
        // done regular interval iterations
    }
    else if(samplingMode == RandomSampling)
    {
        while(n <= numberOfSamples)
        {
            // choose random location in volume
            double x = randf() * bounds.size.x;
            double y = randf() * bounds.size.y;
            double z = randf() * bounds.size.z;

            cleaver::vec3 val =       referenceFunction(bounds, cleaver::vec3(x,y,z));
            cleaver::vec3 val_approx =     testFunction(bounds, cleaver::vec3(x,y,z));

            double absolute_error = L2(val - val_approx);
            double relative_error = absolute_error / L2(val);

            //--- update min and max errors
            if(absolute_error < min_abs_error)
                min_abs_error = absolute_error;
            if(absolute_error > max_abs_error)
                max_abs_error = absolute_error;

            if(relative_error < min_rel_error)
                min_rel_error = relative_error;
            if(relative_error > max_rel_error)
                max_rel_error = relative_error;


            //--- update average errors
            //avg_abs_error = absolute_error
            if(n > 1)
            {
                avg_abs_error = ((n-1)/n)*avg_abs_error + absolute_error/n;
                avg_rel_error = ((n-1)/n)*avg_rel_error + relative_error/n;
            }
            else
            {
                avg_abs_error = absolute_error;
                avg_rel_error = relative_error;
            }

            // save data for plotting
            x_samples[n-1] = x;
            y_samples[n-1] = y;
            error_values[n-1] = relative_error;


            n++;
        }
    }

//    std::cout << "Absolute Errors" << std::endl;
//    std::cout << "\tmin: " << min_abs_error << std::endl;
//    std::cout << "\tmax: " << max_abs_error << std::endl;
//    std::cout << "\tavg: " << avg_abs_error << std::endl;
    std::cout << "Relative Errors" << std::endl;
    std::cout << "\tmin: " << min_rel_error * 100 << "%" << std::endl;
    std::cout << "\tmax: " << max_rel_error * 100 << "%" << std::endl;
    std::cout << "\tavg: " << avg_rel_error * 100 << "%" << std::endl;

    std::cout << "\tPlotting Data..." << std::endl;

    //plot2(x_samples, y_samples, error_values, plotname);
    plot3(numberOfSamples, numberOfSamples, error_values, plotname);
}


//============================================================================================================================
//  evaluateErrorBetweenReferenceAndTestFunction()
//
// This method takes two function pointers:
//   1) to the function being evaluated
//   2) to the ground truth function (to test against)
// It also takes
//   - the bounds of the functino space,
//   - the sampling mode (e.g., regular, random)
//   - number of samples to take
//============================================================================================================================
void evaluateErrorBetweenReferenceAndTestFunction(ScalarTestingFunction testFunction, ScalarTestingFunction referenceFunction,
                                                  const cleaver::BoundingBox &bounds, SamplingMode samplingMode, unsigned int numberOfSamples, const std::string &plotname)

{
    double min_abs_error = 10e16;
    double max_abs_error = 0;
    double avg_abs_error = 0;

    double min_rel_error = 1;
    double max_rel_error = 0;
    double avg_rel_error = 0;

    double n = 1;

    std::vector<float> x_samples(numberOfSamples*numberOfSamples, 0);
    std::vector<float> y_samples(numberOfSamples*numberOfSamples, 0);
    std::vector<float> error_values(numberOfSamples*numberOfSamples, 0);

    // Iterate over grid at regular interval
    if(samplingMode == RegularSampling)
    {
        double size = std::min(bounds.size.x / (float)numberOfSamples, bounds.size.y / (float)numberOfSamples);
        double x_interval = bounds.size.x / (float)numberOfSamples;
        double y_interval = bounds.size.y / (float)numberOfSamples;


        {
            for(int y_count = 0; y_count < numberOfSamples; y_count++)
            {
                for(int x_count = 0; x_count < numberOfSamples; x_count++)
                {
                    double x = 0.5*size + x_count*size;
                    double y = 0.5*size + y_count*size;
                    double z = 1.0;

                    cleaver::vec3 coordinate = cleaver::vec3(x,y,z);

                    double val =       referenceFunction(bounds, coordinate);
                    double val_approx =     testFunction(bounds, coordinate);

                    double absolute_error = std::abs(val - val_approx);
                    double relative_error = absolute_error / std::abs(val);

                    //--- update min and max errors
                    if(absolute_error < min_abs_error)
                        min_abs_error = absolute_error;
                    if(absolute_error > max_abs_error)
                        max_abs_error = absolute_error;

                    if(relative_error < min_rel_error)
                        min_rel_error = relative_error;
                    if(relative_error > max_rel_error)
                        max_rel_error = relative_error;


                    //--- update average errors
                    //avg_abs_error = absolute_error
                    if(n > 1)
                    {
                        avg_abs_error = ((n-1)/n)*avg_abs_error + absolute_error/n;
                        avg_rel_error = ((n-1)/n)*avg_rel_error + relative_error/n;
                    }
                    else
                    {
                        avg_abs_error = absolute_error;
                        avg_rel_error = relative_error;
                    }

                    // save data for plotting
                    x_samples[n-1] = x;
                    y_samples[n-1] = y;
                    error_values[n-1] = relative_error;

                    n++;
                }
            }
        }
        // done regular interval iterations
    }
    else if(samplingMode == RandomSampling)
    {
        while(n <= numberOfSamples)
        {
            // choose random location in volume
            double x = randf() * bounds.size.x;
            double y = randf() * bounds.size.y;
            //double z = randf() * bounds.size.z;

            double z = 0.5;

            cleaver::vec3 coordinate(x,y,z);

            double val =       referenceFunction(bounds, coordinate);
            double val_approx =     testFunction(bounds, coordinate);

            double absolute_error = std::abs(val - val_approx);
            double relative_error = absolute_error / std::abs(val);

            //--- update min and max errors
            if(absolute_error < min_abs_error)
                min_abs_error = absolute_error;
            if(absolute_error > max_abs_error)
                max_abs_error = absolute_error;

            if(relative_error < min_rel_error)
                min_rel_error = relative_error;
            if(relative_error > max_rel_error)
                max_rel_error = relative_error;


            //--- update average errors
            //avg_abs_error = absolute_error
            if(n > 1)
            {
                avg_abs_error = ((n-1)/n)*avg_abs_error + absolute_error/n;
                avg_rel_error = ((n-1)/n)*avg_rel_error + relative_error/n;
            }
            else
            {
                avg_abs_error = absolute_error;
                avg_rel_error = relative_error;
            }

            // save data for plotting
            x_samples[n] = x;
            y_samples[n] = y;
            error_values[n] = relative_error;

            n++;
        }
    }

//    std::cout << "Absolute Errors" << std::endl;
//    std::cout << "\tmin: " << min_abs_error << std::endl;
//    std::cout << "\tmax: " << max_abs_error << std::endl;
//    std::cout << "\tavg: " << avg_abs_error << std::endl;
    std::cout << "Relative Errors" << std::endl;
    std::cout << "\tmin: " << min_rel_error * 100 << "%" << std::endl;
    std::cout << "\tmax: " << max_rel_error * 100 << "%" << std::endl;
    std::cout << "\tavg: " << avg_rel_error * 100 << "%" << std::endl;

    std::cout << "\tPlotting Data..." << std::endl;

    //plot2(x_samples, y_samples, error_values, plotname);
    plot3(numberOfSamples, numberOfSamples, error_values, plotname);
}

double interpolatedFunction(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{
    static cleaver::FloatField *interpolatedField = createFieldFromFunction(analyticFunction, bounds);

    return interpolatedField->valueAt(pos);
}

double tricubic_interpolated_function(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{
    static cleaver::FloatField *interpolatedField = createFieldFromFunction(analyticFunction, bounds);

    return interpolatedField->tricubicValueAt(pos);
}

double cached_tricubic_interpolated_function(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{
    static cleaver::FloatField *interpolatedField = createFieldFromFunction(analyticFunction, bounds);

    return interpolatedField->cachedTricubicValueAt(pos.x, pos.y, pos.z);
}

cleaver::vec3 cached_tricubic_Gradient(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{
    static cleaver::FloatField *interpolatedField = createFieldFromFunction(analyticFunction, bounds);

    return interpolatedField->cachedTricubicGradientAt(pos.x, pos.y, pos.z);
}

cleaver::vec3 analyticGradientOfInterpolatedFunction(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{
    static cleaver::FloatField *interpolatedField = createFieldFromFunction(analyticFunction, bounds);

    cleaver::vec3 gradient = interpolatedField->gradientAt(pos);

    std::cout << "analytic gradient.x = " << gradient.x << "  vs reference = " << analyticGradient(bounds, pos).x << std::endl;

    return gradient;
}


cleaver::vec3 interpolatedGradient(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{
    static std::vector<cleaver::FloatField*> interpolatedField = createGradientFieldsFromFunction(analyticGradient, bounds);

    cleaver::vec3 gradient;
    gradient.x = interpolatedField[0]->valueAt(pos);
    gradient.y = interpolatedField[1]->valueAt(pos);
    gradient.z = interpolatedField[2]->valueAt(pos);

    return gradient;
}

double convolutionFunction(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{
    static cleaver::FloatField *interpolatedField = createFieldFromFunction(analyticFunction, bounds);

    return interpolatedField->convolutionValueAt(pos);
}

cleaver::vec3 convolutionGradient(const cleaver::BoundingBox &bounds, const cleaver::vec3 &pos)
{
    static cleaver::FloatField *interpolatedField = createFieldFromFunction(analyticFunction, bounds);

    return interpolatedField->convolutionGradientAt(pos);
}

//=============================================================================================================================================
// plotDatasetToFile()
//
// This method uses matplotlib to plot the corresponding x,y data
//=============================================================================================================================================
void plotDatasetToFile(const std::vector<float> &x, const std::vector<float> &y, const std::vector<float> &values, const std::string &filename)
{
    Py_Initialize();                       // Start the Python Interpreter
    PyRun_SimpleString("print 'Python Initialized'");

    PyRun_SimpleString("import matplotlib");
    PyRun_SimpleString("from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas");
    PyRun_SimpleString("from matplotlib.figure import Figure");
    PyRun_SimpleString("import matplotlib.pyplot as plt");
    PyRun_SimpleString("import matplotlib.mlab as mlab");

    PyRun_SimpleString("x = [1,2,3,4]");
    PyRun_SimpleString("y = [3,4,8,6]");
    PyRun_SimpleString("val = [10, 200, 140, 85]");
    //PyRun_SimpleString("color = [str(item/255.) for item in val]");

    PyRun_SimpleString("fig = Figure(figsize=(3,3));");                // Create a figure with size 3 x 3 inches
    PyRun_SimpleString("canvas = FigureCanvas(fig);");                 // Create a canvas and add the figure to it
    PyRun_SimpleString("ax = fig.add_subplot(1, 1, 1);");              // Create a subplot
    PyRun_SimpleString("ax.set_title('Error Plot');");                 // Set the Title
    PyRun_SimpleString("ax.set_xlabel('x label', fontsize=8);");       // Set X axis label
    PyRun_SimpleString("ax.set_ylabel('y label', fontsize=8);");       // Set Y axis label

    PyRun_SimpleString("ax.grid(True, linestyle='-', color='0.75');"); // Display Grid
    PyRun_SimpleString("ax.scatter(x, y, s=4, c=val, cmap=plt.cm.coolwarm);"); // Generate the scatterplot

    PyRun_SimpleString("canvas.print_figure('errortest.png', dpi=800)"); // Save the plot to file

    /*
    PyRun_SimpleString("matplotlib.pyplot.ion()");       // need this so code wont' HAULT
    PyRun_SimpleString("x = [1,2,3,4]");
    PyRun_SimpleString("y = [3,4,8,6]");
    PyRun_SimpleString("matplotlib.pyplot.scatter(x,y)");
    PyRun_SimpleString("matplotlib.pyplot.show()");
    PyRun_SimpleString("matplotlib.pyplot.show()");
    PyRun_SimpleString("matplotlib.pyplot.show()");
    */

    Py_Finalize();                         // Finish the Python Interpreter
    //--------------------------------------------------------------------------------
}


// Entry Point
int main(int argc,	char* argv[])
{
    // Create Domain Size
    cleaver::BoundingBox bounds(cleaver::vec3::zero, cleaver::vec3(16, 16, 2));

    // Set The Testing Functions
    analyticFunction = &Linear1_Function;
    analyticGradient = &Linear1_Gradient;

    Py_Initialize();                       // Start the Python Interpreter


    //------------------------------------------------------------------------------------------------
    // Test 1 - Analytic Potential vs. Linear Interpolation Function
    //------------------------------------------------------------------------------------------------
    std::cout << "Testing Errors for Trilinear Interpolation Function Data..." << std::endl;
    evaluateErrorBetweenReferenceAndTestFunction(interpolatedFunction, analyticFunction, bounds, RegularSampling, 124, "Interpolated Function vs Analytic Function");
    std::cout << std::endl;

    //------------------------------------------------------------------------------------------------
    // Test 2 - Analytic Gradient of Analytic Function vs. Analytic Gradient of Interpolation Function
    //------------------------------------------------------------------------------------------------
    std::cout << "Testing Errors for Analytic Gradient of Interpolated Data..." << std::endl;
    evaluateErrorBetweenReferenceAndTestFunction(analyticGradientOfInterpolatedFunction, analyticGradient, bounds, RegularSampling, 64, "Analytic Gradient of Interpolation vs Analytic Gradient");
    std::cout << std::endl;

    //------------------------------------------------------------------------------------------------
    // Test 3 - Analytic Gradient of Analytic Function vs. Interpolation of Sampled Analytic Gradient
    //------------------------------------------------------------------------------------------------
    std::cout << "Testing Errors for Interpolated Gradient Data.." << std::endl;
    evaluateErrorBetweenReferenceAndTestFunction(interpolatedGradient, analyticGradient, bounds, RegularSampling, 64, "Interpolated Gradient vs Analytic Gradient");
    std::cout << std::endl;

    //------------------------------------------------------------------------------------------------
    // Test 4 - Analytic Potential vs. Tricubic Interpolation Function
    //------------------------------------------------------------------------------------------------
    std::cout << "Testing Errors for Tricubic Interpolation Function" << std::endl;
    evaluateErrorBetweenReferenceAndTestFunction(tricubic_interpolated_function, analyticFunction, bounds, RegularSampling, 124, "Tricubic Function vs Analytic Function");
    std::cout << std::endl;

    //------------------------------------------------------------------------------------------------
    // Test 5 - Tricubic Interpolation Function vs Cached Interpolation Function
    //------------------------------------------------------------------------------------------------
    std::cout << "Testing Errors for Cached Tricubic Interpolation Function" << std::endl;
    evaluateErrorBetweenReferenceAndTestFunction(cached_tricubic_interpolated_function, tricubic_interpolated_function, bounds, RegularSampling, 124, "Cached Tricubic vs Tricubic Function");
    std::cout << std::endl;

    //------------------------------------------------------------------------------------------------
    // Test 6 - Analytic Fucntion vs Convolution Function
    //------------------------------------------------------------------------------------------------
    std::cout << "Testing Errors for Convolution Function" << std::endl;
    evaluateErrorBetweenReferenceAndTestFunction(convolutionFunction, analyticFunction, bounds, RegularSampling, 124, "Convolution vs Analytic Function");
    std::cout << std::endl;

    //------------------------------------------------------------------------------------------------
    // Test 7 - Tricubic Gradient Function vs Analytic Gradient
    //------------------------------------------------------------------------------------------------
    std::cout << "Tricubic Gradient Function vs Analytic Gradient" << std::endl;
    evaluateErrorBetweenReferenceAndTestFunction(cached_tricubic_Gradient, analyticGradient, bounds, RegularSampling, 64, "Cached Tricubic Gradient vs Analytic Gradient");
    std::cout << std::endl;

    //------------------------------------------------------------------------------------------------
    // Test 8 - Convolution Gradient Function vs Analytic Gradient
    //------------------------------------------------------------------------------------------------
    std::cout << "Convolution Gradient Function vs Analytic Gradient" << std::endl;
    evaluateErrorBetweenReferenceAndTestFunction(convolutionGradient, analyticGradient, bounds, RegularSampling, 64, "Convolution vs Analytic Gradient");
    std::cout << std::endl;

    Py_Finalize();                         // Finish the Python Interpreter

    //int dummy;
    //std::cin >> dummy;

    return 0;
}
