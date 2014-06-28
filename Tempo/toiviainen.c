/*
 
 tempo.toiviainen
 
 Beat Tracking with a Nonlinear Oscillator by P.Toiviainen
 External implemented by Jaime Chao from Laurent Bonasse's Max patch
 Sept 1th 2011
 
 */

#include "ext.h"	
#include "ext_obex.h"	
#include <math.h> 

#define PI 3.14159
#define CARRE(x) (x * x)
#define CUBE(x) (x * x * x)

////////////////////////// object struct
typedef struct _toiviainen 
{
	t_object obj;
    double gamma;       //width of the receptive field
	double alpha;       //coefficient monotoring adaptive speed
	double eta_l;       //coefficient monitoring long term adaptive
	double eta_s;       //coefficient monitoring short term adaptive
    double epsilon_t;   //time between two inlet's bang divide per period
	double phase;       //oscillator phase
	double period;      //oscillator period
	double F;           //phase correction function
    double G;           //adaptive function
    double H;           //adaptive function
    double phase_event; //phase saved at extern event
    double period_event;//period saved at extern event
    char initTime;      //timer init boolean
	long in;            //space for the inlet number used by the proxy
	void *proxy1;       //right inlet
	void *outlet1;      //right outlet
	void *outlet2;      //middle outlet
    void *outlet3;      //left outlet
} t_toiviainen;

///////////////////////// function prototypes
//// standard set
void *toiviainen_new(t_symbol *s, long argc, t_atom *argv);
void toiviainen_free(t_toiviainen *x);
void toiviainen_bang(t_toiviainen *x);
void toiviainen_float(t_toiviainen *x, double n);
void toiviainen_reset(t_toiviainen *x, t_symbol *s, long argc, t_atom *argv);
void toiviainen_assist(t_toiviainen *x, void *b, long m, long a, char *s);

///// mathematic model set 
void updatePhase (t_toiviainen *x);
void updatePeriod (t_toiviainen *x);
void updateF (t_toiviainen *x);
void updateH (t_toiviainen *x);
void updateG (t_toiviainen *x);

//// other
double clipPeriod(double period);

//////////////////////// global class pointer variable
static t_class *toiviainen_class;


////////////////////////// standard API functions

int main(void)
{	
	t_class *c;
	
	c = class_new("tempo.toiviainen", (method)toiviainen_new, (method)toiviainen_free, (long)sizeof(t_toiviainen), 0L, A_GIMME, 0);
	
    class_addmethod(c, (method)toiviainen_assist, "assist", A_CANT, 0);  
	class_addmethod(c, (method)toiviainen_bang, "bang", 0);
	class_addmethod(c, (method)toiviainen_float, "float", A_FLOAT, 0);
	class_addmethod(c, (method)toiviainen_reset, "reset", A_GIMME, 0);
    
    //clip incoming parameters
    CLASS_ATTR_DOUBLE(c, "gamma", 0, t_toiviainen, gamma);
    CLASS_ATTR_FILTER_CLIP(c, "gamma", 0.1, 10.);
    CLASS_ATTR_DOUBLE(c, "alpha", 0, t_toiviainen, alpha);
    CLASS_ATTR_FILTER_CLIP(c, "alpha", 0.5, 15.);
    CLASS_ATTR_DOUBLE(c, "etaL", 0, t_toiviainen, eta_l);
    CLASS_ATTR_FILTER_CLIP(c, "etaL", 0., 1.);    
    CLASS_ATTR_DOUBLE(c, "etaS", 0, t_toiviainen, eta_s);
    CLASS_ATTR_FILTER_CLIP(c, "etaS", 0., 1.);  

	class_register(CLASS_BOX, c);
	toiviainen_class = c;

	return 0;
}

void *toiviainen_new(t_symbol *s, long argc, t_atom *argv)
{
	t_toiviainen *x = NULL;
	
	if ((x = (t_toiviainen *)object_alloc(toiviainen_class))) {
		
		x->proxy1 = proxy_new((t_object *)x, 1, &x->in);
		
        x->outlet3 = floatout((t_object *)x);
		x->outlet2 = floatout((t_object *)x);
		x->outlet1 = bangout((t_object *)x);
        
        object_post((t_object *)x, "Beat Tracking with a Nonlinear Oscillator by P. Toiviainen");
        post("External implemented by Jaime Chao from Laurent Bonasse's patch");
        post("Version: Sept 1th 2011");
	}
	return (x);
}

void toiviainen_free(t_toiviainen *x)
{
	object_free(x->proxy1);
}

void toiviainen_assist(t_toiviainen *x, void *b, long m, long a, char *s)
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
				sprintf(s, "Tempo");
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

void toiviainen_bang(t_toiviainen *x)
{
	double mem = x->phase;
    long newTime;
	static long eventTime;
    
	switch (proxy_getinlet((t_object *)x))
	{
        case 0: //event : bang in left inlet
            eventTime = gettime();
            
            x->period_event = x->period;
            updatePeriod(x);
            x->phase_event = x->phase;
            updateF(x);
            break;
            
        case 1: //refresh : bang in right inlet
            if(x->initTime) {
                newTime = gettime();
                x->epsilon_t = (newTime - eventTime) / x->period_event;
            }
            else { //init timer
                eventTime = gettime();
                x->initTime = 1;
            }
            updateH(x);
            updateG(x);
            updatePeriod(x);
            updatePhase(x);
			break;
			
		default:
			break;
	}
    
    outlet_float(x->outlet3, x->period);
    outlet_float(x->outlet2, x->phase);

    //output a pulse in left outlet
	if ( x->phase == 0.0 || (mem < 0.0 && x->phase > 0.0))
		outlet_bang(x->outlet1);
}

void toiviainen_float(t_toiviainen *x, double n)
{	
	switch (proxy_getinlet((t_object *)x)) 
	{
		case 0:
            x->period_event = clipPeriod(n);
            x->phase = -0.00001;
            outlet_float(x->outlet3, (long)x->period);
            break;
			
		default:
			break;
	}
}

void toiviainen_reset(t_toiviainen *x, t_symbol *s, long argc, t_atom *argv)
{
    if(argc == 1) { 
        if (atom_gettype(argv) == A_FLOAT) 
            x->period_event = clipPeriod(atom_getfloat(argv));
        else {
            post("%s argument must be a float", s->s_name);
            x->period_event = 1000.;
        }
    }
    else
        x->period_event = 1000.;
    
    x->phase_event = -0.00001;
    x->phase = 0.;
    updateF(x);
    x->epsilon_t = 0.;
    updatePeriod(x);
    x->initTime = 0;
    
    outlet_float(x->outlet3, x->period);
    outlet_float(x->outlet2, x->phase);
    post("gamma: %f  alpha: %f  eta_s: %f  eta_l: %f", x->gamma, x->alpha, x->eta_s, x->eta_l);
}


////////////////////////// mathematic model functions

//update phase at next step
void updatePhase (t_toiviainen *x)
{
	double nb;
    int ajout;
    nb = x->phase_event + x->epsilon_t + x->F * (x->eta_s * x->G + x->eta_l * x->H);
    post("AV %f  nb %f",x->phase, nb);
    ajout = (nb < 0.5) ? 1 : 0; //clip phase between -0,5 and 0,5
    
	x->phase = ajout + fmod(nb-0.5, 1.0) - 0.5; //(nb - (int)nb) - 0.5;
    post("AP %f  nb %f",x->phase, nb);

}

//update period at next step
void updatePeriod (t_toiviainen *x)
{
    x->period = x->period_event / (1 + x->eta_l * x->F * x->G);
} 
 
//update adaptive function
void updateG (t_toiviainen *x)
{
	x->G = 1 - (CARRE(x->alpha) / 2. * CARRE(x->epsilon_t) + x->alpha * x->epsilon_t + 1) * exp(-x->alpha * x->epsilon_t);
}

//update adaptive function
void updateH (t_toiviainen *x)
{
	x->H = x->epsilon_t + (x->alpha / 2. * CARRE(x->epsilon_t) + 2 * x->epsilon_t + 3 / x->alpha) * exp(-x->alpha * x->epsilon_t) - 3 / x->alpha; 
}

//update phase correction 
void updateF (t_toiviainen *x)
{
    double nb;
    nb = 1 + tanh(x->gamma * ( cos( 2 * PI * x->phase_event) - 1));
	x->F = nb * (nb - 2) * sin( 2 * PI * x->phase_event );
}


/////////////////////////// other

//filter out periods below 30 bpm and above 300 bpm
double clipPeriod(double period)
{
    if(period < 200) 
        return 200.;
    else if(period > 2000) 
        return 2000.;
    else
        return period;
}

