/**************************************
 * bc.virfun - bc.virfun.c
 * Created on 08/10/10 by BenCello
 * Last updated 08/10/10
 **************************************/

///@file bc.virfun.c bc.virfun external code
//@{


#include "ext.h"				// standard Max include, always required
#include "ext_obex.h"			// required for new style Max object
#include "ext_systhread.h"      // thread mechanisms
#include "ext_sysparallel.h"    // parallel tasking

#include "bc.virfun.h"

/**@ingroup yin
 * @nosubgrouping
 * @brief bc.virfun external
 * @details */
typedef struct _bc_virfun 
{
	t_object	ob;			///< Pointer to the object itself
    t_sysparallel_task  *th_compute;    ///< thread reference to compute
    t_systhread_mutex   mutex;      ///< mutual exclusion lock for threadsafety
    bool        f_done;     ///< Flag for termination of the recursion
	long		a_mode;		///< Mode attribute
	float		a_approx;	///< Approximation attribute
//	float		approx;		///< Approximation factor (MIDI)
	float		approxf;	///< Approximation factor (Hz)
    double*		freqs;		///< Frequencies array
    long        nbfreqs;    ///< Number of frequencies in the array
	long		freqnb;		///< Size of currently allocated array
    double       o_virfun;   ///< Virfun value to output
	void		*out;		///< Outlet 0 (Virtual Fundamental)
} t_bc_virfun;

////////////////
// Prototypes //
////////////////

// Standard methodes
void *bc_virfun_new(t_symbol *s, long argc, t_atom *argv);
void bc_virfun_free(t_bc_virfun *x);
void bc_virfun_assist(t_bc_virfun *x, void *b, long m, long a, char *s);

// Input/ouput routines
void bc_virfun_int(t_bc_virfun *x, long n);
void bc_virfun_float(t_bc_virfun *x, double f);
void bc_virfun_list(t_bc_virfun *x, t_symbol *s, long ac, t_atom *av);
void bc_virfun_do(t_bc_virfun *x, t_symbol *s, long ac, t_atom *av);

t_max_err bc_virfun_approx(t_bc_virfun *x, void *attr , long ac, t_atom *av);

// Thread routines
void *threaded_rec_virfun(t_sysparallel_worker *w);

// Global class pointer variable
t_class *bc_virfun_class;

///////////////
// Functions //
///////////////

int C74_EXPORT main(void)
{
	t_class *c;
	
	c = class_new("bc.virfun", (method)bc_virfun_new, (method)bc_virfun_free, (long)sizeof(t_bc_virfun), 0L, A_GIMME, 0);
	
    // credits
	post("bc.virfun v0.1b – 2010: Virtual fundamental calculation");
	post("   design by Gérard Assayag with help from Bennett Smith");
	post("   external by Benjamin Lévy");
	
	// assistance
	class_addmethod(c, (method)bc_virfun_assist,"assist",A_CANT, 0); 
	
	// input methods
	//class_addmethod(c, (method)bc_virfun_bang, "bang", 0);
	class_addmethod(c, (method)bc_virfun_int, "int", A_LONG, 0);
	class_addmethod(c, (method)bc_virfun_float, "float", A_FLOAT, 0);
	class_addmethod(c, (method)bc_virfun_list, "list", A_GIMME, 0);
	//class_addmethod(c, (method)bc_virfun_anything, "anything",A_GIMME, 0);
	//class_addmethod(c, (method)bc_virfun_approx, "ft1", A_FLOAT, 0);
	
	// Mode attribute
	CLASS_ATTR_LONG(c, "mode", 0 , t_bc_virfun, a_mode);
	CLASS_ATTR_ENUMINDEX(c, "mode", 0, "Freq MIDI");
	CLASS_ATTR_SAVE(c, "mode", 0);
	
	// Approximation attribute
	CLASS_ATTR_FLOAT(c, "approx", 0, t_bc_virfun, a_approx);
	CLASS_ATTR_LABEL(c, "approx", 0, "Approximation (MIDI)");
	CLASS_ATTR_FILTER_MIN(c, "approx", 0.);
	CLASS_ATTR_SAVE(c, "approx", 0);
	CLASS_ATTR_ACCESSORS(c,"approx",NULL,bc_virfun_approx);
	
	class_register(CLASS_BOX, c);
	bc_virfun_class = c;
	
	return 0;
}

///@name Standard Max5 methodes
//@{

/**@memberof t_bc_virfun
 * @brief Object instantiation */
void *bc_virfun_new(t_symbol *s, long argc, t_atom *argv)
{
	t_bc_virfun *x = NULL;
	
	if ((x = (t_bc_virfun *)object_alloc(bc_virfun_class)))
	{
		// outlets
		x->out = intout(x);
            
        if (argc)
        {
            if ((argv)->a_type == A_LONG)
                x->a_approx=(float)atom_getlong(argv);
            else if ((argv)->a_type == A_FLOAT)
                x->a_approx=atom_getfloat(argv);
            else
                x->a_approx = 0.5;
        }
        else
            x->a_approx = 0.5;
        x->approxf=midi2freq_approx(x->a_approx);
        
        x->freqnb=0;
        x->freqs=NULL;
        x->f_done = true;
        
        // process attr args, if any
        attr_args_process(x, argc, argv);
            
        // initialize thread elements
        x->th_compute = NULL;
        systhread_mutex_new(&x->mutex,0);
	
    }
	return (x);
}

/**@memberof t_bc_virfun
 * @brief Object destruction */
void bc_virfun_free(t_bc_virfun *x)
{
    if (x->th_compute)
        object_free((t_object *)x->th_compute);

    // free our mutex
    if (x->mutex)
        systhread_mutex_free(x->mutex);
}


/**@memberof t_bc_virfun
 * @brief Inlet/Outlet contextual information when patchin in Max5 */
void bc_virfun_assist(t_bc_virfun *x, void *b, long io, long index, char *s)
{
	switch (io)
	{
		case ASSIST_INLET: // inlets = 1
			switch (index)
		{
			case 0: // leftmost
				sprintf(s, "Ordered list of frequencies (Hz)");
				break;
			case 1:
				sprintf(s, "float: Approximation factor (MIDI)");
				break;
		}
			break;
			
		case ASSIST_OUTLET: // outlets = 2
			switch (index)
		{
			case 0: // leftmost
				sprintf(s, "Virtual Fundamental"); 
		}
			break;
	}
}

//@}

///@name Input/Output routines
//@{

void bc_virfun_list(t_bc_virfun *x, t_symbol *s, long ac, t_atom *av)
{
    if (x->f_done) {
        defer(x,(method)bc_virfun_do,s,ac,av);
    } else {
        object_warn((t_object *)x, "bc.virfun is still busy computing...");
    }
}

/**@memberof t_bc_virfun
 * @brief Compute and return the virtual fondamental*/
void bc_virfun_do(t_bc_virfun *x, t_symbol *s, long ac, t_atom *av)
{
	int i;
    double virfun = 0.;
    x->nbfreqs = ac;
	if (x->freqnb<x->nbfreqs)
	{
		if (x->freqs)
			free(x->freqs);
		x->freqs=(double*)malloc(x->nbfreqs*sizeof(double));
		x->freqnb=x->nbfreqs;
	}
	
	if (x->a_mode)
	{
		for (i=0; i<ac; i++,av++)
		{
			if (atom_gettype(av)==A_LONG)
				x->freqs[i]=midi2freq((float)atom_getlong(av));
			else if (atom_gettype(av)==A_FLOAT)
				x->freqs[i]=midi2freq(atom_getfloat(av));
			else
				object_error((t_object*)x, "wrong argument type");
		}
        // Unthreaded version
		//virfun = rec_virfun(x->freqs, x->freqs+x->nbfreqs, 0.1, x->freqs[0]*(1.0+x->approxf), x->approxf);
        
        // Threaded version
        if (x->th_compute == NULL) {
            x->th_compute = sysparallel_task_new(x, (method) threaded_rec_virfun, 1);
            x->th_compute->flags = SYSPARALLEL_PRIORITY_TASK_LOCAL;
        }
        x->f_done = false;
        sysparallel_task_execute(x->th_compute);
        
        systhread_mutex_lock(x->mutex);
        virfun = x->o_virfun;  // shared data
        systhread_mutex_unlock(x->mutex);
        
        // Output
        outlet_float(x->out, (round(freq2midi(virfun)/x->a_approx))*x->a_approx);
        x->f_done = true;
	}
	else
	{
		for (i=0; i<ac; i++,av++)
		{
			if (atom_gettype(av)==A_LONG)
				x->freqs[i]=atom_getlong(av);
			else if (atom_gettype(av)==A_FLOAT)
				x->freqs[i]=atom_getfloat(av);
			else
				object_error((t_object*)x, "wrong argument type");
		}
        // Unthreaded version
		//virfun = rec_virfun(x->freqs, x->freqs+x->nbfreqs, 0.1, x->freqs[0]*(1.0+x->approxf), x->approxf);
        
        // Threaded version
        if (x->th_compute == NULL) {
            x->th_compute = sysparallel_task_new(x, (method) threaded_rec_virfun, 1);
            x->th_compute->flags = SYSPARALLEL_PRIORITY_TASK_LOCAL;
        }
        x->f_done = false;
        sysparallel_task_execute(x->th_compute);
        
        systhread_mutex_lock(x->mutex);
        virfun = x->o_virfun;  // shared data
        systhread_mutex_unlock(x->mutex);
        
        // Output
        outlet_float(x->out, virfun);
        x->f_done = true;
	}
 }

/**@memberof t_bc_virfun
 * @brief Compute and return the virtual fondamental*/
void bc_virfun_int(t_bc_virfun *x, long n)
{
	t_max_err err;
	t_atom av;
	t_symbol *s_list = gensym("list");
	err = atom_setlong(&av, n);
	if (!err)
		bc_virfun_list(x, s_list, 1, &av);
	///@remarks Of course, virtual fundamental of one pitch is the pitch itself
	
}

/**@memberof t_bc_virfun
 * @brief Compute and return the virtual fondamental*/
void bc_virfun_float(t_bc_virfun *x, double f)
{
	t_max_err err;
	t_atom av;
	t_symbol *s_list = gensym("list");
	err = atom_setfloat(&av, f);
	if (!err)
		bc_virfun_list(x, s_list, 1, &av);
	///@remarks Of course, virtual fundamental of one pitch is the pitch itself
}
//@}

///@name Internal routines
//@{

/**@memberof t_bc_virfun
 * @brief Set the approximation (tolerance) of the calculation*/
t_max_err bc_virfun_approx(t_bc_virfun *x, void *attr , long ac, t_atom *av)
{
	if (ac&&av) {
		x->a_approx = atom_getfloat(av);
		x->approxf=midi2freq_approx(x->a_approx);
	} else {
		x->a_approx = 0.5;
	}
	return MAX_ERR_NONE;
}



/**@memberof t_bc_virfun
 * @brief Execute the recursive algorithm in another thread*/
void *threaded_rec_virfun(t_sysparallel_worker *w)
{
    t_bc_virfun *x = (t_bc_virfun *)w->data;
    float virfun;
    
    virfun = rec_virfun(x->freqs, &x->freqs[x->nbfreqs-1], 0.1, x->freqs[0]*(1.0+x->approxf), x->approxf);
    
    // Set the output value
    systhread_mutex_lock(x->mutex);
    x->o_virfun = virfun;  // shared data
    systhread_mutex_unlock(x->mutex);
    
    return NULL;
}

//@}
