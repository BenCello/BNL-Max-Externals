/*
 
    tempo.large
 
Beat Tracking with a Nonlinear Oscillator by Edward W. Large
External implemented by Jaime Chao from Laurent Bonasse's Max patch
Sept 1th 2011

*/

#include "ext.h"		
#include "ext_obex.h"		
#include <math.h>

#define PI 3.141592654


////////////////////////// object struct
typedef struct _large 
{
	t_object obj;
    double initPeriod; //initial & reset period
	double gamma;       //width of the receptive field
	double eta_phi;     //coefficient monitoring phase
	double eta_p;       //coefficient monitoring periode
    long delta_t;       //time between two inlet's bang
	double phase;       //oscillator phase
	double period;      //oscillator period
	double f;           //phase correction function
    long oldTime;       //instead of static long in large_bang
    char initTime;      //timer init boolean
	long in;            //space for the inlet number used by the proxy
	void *proxy1;       //right inlet
	void *outlet1;      //right outlet
	void *outlet2;      //middle outlet
    void *outlet3;      //left outlet
} t_large;


////////////////////////// function prototypes
//// standard set
void *large_new(t_symbol *s, long argc, t_atom *argv);
void large_free(t_large *x);
void large_bang(t_large *x);
void large_float(t_large *x, double n);
void large_reset(t_large *x);
void large_assist(t_large *x, void *b, long m, long a, char *s);

//// mathematic model set
void updatePhaseEvent (t_large *x);
void updatePhase (t_large *x);
void updatePeriodEvent (t_large *x);
void phaseReset (t_large *x);

//// other
double clipPeriod(double period);

////////////////////////// global class pointer variable
static t_class *large_class;


////////////////////////// standard API functions

int C74_EXPORT main(void)
{	
	t_class *c;
	
	c = class_new("tempo.large", (method)large_new, (method)large_free, (long)sizeof(t_large), 0L, A_GIMME, 0);
    
    post("Beat Tracking with a Nonlinear Oscillator by Edward W. Large");
    post("External implemented by Jaime Chao from Laurent Bonasse's patch");
    post("With corrections from Benjamin Lévy");
    post("Version: Sept 3th 2012");
	
    class_addmethod(c, (method)large_assist, "assist", A_CANT, 0);  
	class_addmethod(c, (method)large_bang, "bang", 0);
	class_addmethod(c, (method)large_float, "float", A_FLOAT, 0);
	class_addmethod(c, (method)large_reset, "reset", 0);

    //clip attribute values
    CLASS_ATTR_DOUBLE(c, "gamma", 0, t_large, gamma);
    CLASS_ATTR_FILTER_CLIP(c, "gamma", 0.1, 10.); 
    CLASS_ATTR_DOUBLE(c, "etaP", 0, t_large, eta_p);
    CLASS_ATTR_FILTER_CLIP(c, "etaP", 0., 1.);    
    CLASS_ATTR_DOUBLE(c, "etaPhi", 0, t_large, eta_phi);
    CLASS_ATTR_FILTER_CLIP(c, "etaPhi", 0., 1.);      

	class_register(CLASS_BOX, c);
	large_class = c;

	return 0;
}

void *large_new(t_symbol *s, long argc, t_atom *argv)
{
	t_large *x = NULL;
	
	if ((x = (t_large *)object_alloc(large_class)))
    {
		x->proxy1 = proxy_new((t_object *)x, 1, &x->in);    //right inlet is a proxy
		
        x->outlet3 = floatout((t_object *)x);
		x->outlet2 = floatout((t_object *)x);
		x->outlet1 = bangout((t_object *)x);
        
        if(argc == 1) { 
            if (atom_gettype(argv) == A_FLOAT) 
                x->initPeriod = clipPeriod(atom_getfloat(argv));
            else {
                object_error((t_object*)x, "Argument must be a float, using default period: 1000ms");
                x->initPeriod = 1000.;
            }
        }
        else
        {
            //object_post((t_object*)x, "Using default period: 1000ms");
            x->initPeriod = 1000.;
        }
        
        large_reset(x);
        
        x->delta_t = 0;
        //x->period = 1.;
	}
    
	return (x);
}

void large_free(t_large *x)
{
	object_free(x->proxy1);
}

void large_assist(t_large *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { // inlet
		switch (a) 
		{
			case 0:
				sprintf(s, "Event");
				break;
			case 1:
				sprintf(s, "Refresh");
				break;
			default:
				post("???");
				break;
		}
	} 
	else {	// outlet
		switch (a) 
		{
			case 0:
				sprintf(s, "Pulse");
				break;
			case 1:
				sprintf(s, "Phase");
				break;
            case 2:
				sprintf(s, "Period");
				break;
			default:
				post("???");
				break;
		}			
	}
}

void large_bang(t_large *x)
{
	double mem = x->phase;
    long newTime;
//	static long oldTime;

	switch (proxy_getinlet((t_object *)x))
	{
		case 0: //event : bang in left inlet
            newTime = gettime();
            x->delta_t = newTime - x->oldTime;
            x->oldTime = newTime;
            
            updatePhase(x);
            phaseReset(x);
			updatePhaseEvent(x);
			updatePeriodEvent(x);
            outlet_float(x->outlet3, x->period);
            break;
            
		case 1: //refresh : bang in right inlet
            if(x->initTime) {
                newTime = gettime();
                x->delta_t = newTime - x->oldTime;
                x->oldTime = newTime;
            }
            else { //init timer delta_t
                x->oldTime = gettime();
                x->initTime = 1;
                return;
            }
            updatePhase(x);
			break;
			
		default:
			break;
	}
        
    outlet_float(x->outlet2, x->phase);
    
    //output a pulse in left outlet
	if ( x->phase == 0. || (mem < 0. && x->phase > 0.))
		outlet_bang(x->outlet1);
}

void large_float(t_large *x, double n)
{	
	switch (proxy_getinlet((t_object *)x))
	{
		case 0:
            x->period = clipPeriod(n);
			x->phase = 0.;
            outlet_float(x->outlet3, x->period);
            outlet_float(x->outlet2, x->phase);
            break;
			
		default:
			break;
	}
}

//reset the object
void large_reset(t_large *x)
{
    
    x->period = x->initPeriod;
    x->phase = 0.;
    x->f = 0.;
    x->initTime = 0;

    outlet_float(x->outlet3, x->period);
    outlet_float(x->outlet2, x->phase);
    //post("gamma %f  etaP %f  etaPhi %f", x->gamma, x->eta_p, x->eta_phi);
}


////////////////////////// mathematic model functions

//update phase at next step
void updatePhase (t_large *x)
{
	x->phase += (x->delta_t / x->period);
	x->phase = (x->phase < 0.5) ? x->phase : x->phase - 1.0 ; //clip phase between -0,5 and 0,5
}

//update phase if extern event
void updatePhaseEvent (t_large *x)
{
    x->phase -= x->eta_phi * x->f;
	x->phase = (x->phase < 0.5) ? x->phase : x->phase - 1.0 ; //clip phase between -0,5 and 0,5 
}

//update period if extern event
void updatePeriodEvent (t_large *x)
{
	x->period *= 1 + x->eta_p * x->f;
}

//update phase correction  
void phaseReset (t_large *x)
{
    double nb, sech;
	
    nb = x->gamma * ( cos( 2 * PI * x->phase) - 1);
    sech = 1 - tanh(nb) * tanh(nb);
	x->f = 1/(2 * PI) * sech * sin( 2 * PI * x->phase );
}


/////////////////////////// other

//filter out periods below 30 bpm and above 300 bpm
double clipPeriod(double period)
{
    if(period < 200) 
        return 200.;
    else if(period > 6000) 
        return 6000.;
    else
        return period;
}
