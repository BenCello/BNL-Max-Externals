#include "ext.h"
#include "ext_strings.h"
#include <string.h>
#include <CoreFoundation/CoreFoundation.h>

//--------------------------------------------------------------------------
/* ---- aalert.c ---
 
 Aqua-Style Standard Alert box
 
 version 0.8 - 05 november 2015
 
 (cc) 2015 Michael Egger, [ a n y m a ]
 me@anyma.ch
 
 licensed under GNU GPL (http://www.gnu.org )
 -- */

//--------------------------------------------------------------------------
// Definitions

#define MAX_ARGS  5


typedef struct _aalert
{
    t_object	a_ob;
    t_atom		a_list[MAX_ARGS];				// array of Atoms: Arguments to aalert
    int			a_size;							// number of Atoms in the list
    void		*a_out1,*a_out2,*a_out3;		// the three outlets
} t_aalert;

void *aalert_class;

void aalert_bang(t_aalert *x);
void aalert_showbox(t_aalert *x);
void aalert_assist(t_aalert *x, void *b, long m, long a, char *s);
void *aalert_new(t_symbol *s,int argc, t_atom *argv);
void aalert_set(t_aalert *x, t_symbol *s, int argc, t_atom *argv);

//--------------------------------------------------------------------------
// Main Entry

int C74_EXPORT main()
{
    setup((t_messlist **)&aalert_class, (method)aalert_new, 0L, (short)sizeof(t_aalert), 0L, A_GIMME, 0);
    
    addbang((method)aalert_bang);
    addmess((method)aalert_assist,"assist",A_CANT,0);
    addmess ((method)aalert_set, "set", A_GIMME, 0);
    addmess ((method)aalert_set, "list", A_GIMME, 0);
    
    // welcome message
    post("aalert version 0.6 - Maclike dialog box © 2016 Michael Egger");
    post("aalert64 for Intel & Silicon Macs by Benjamin Lévy - 2023");
    
}

//--------------------------------------------------------------------------
// Initialize object

void *aalert_new(t_symbol *s,int argc, t_atom *argv)
{
    t_aalert *x;
    
    x = (t_aalert *)newobject(aalert_class);
    
    // make outlets
    x->a_out3 = bangout((t_object *)x);
    x->a_out2 = bangout((t_object *)x);
    x->a_out1 = bangout((t_object *)x);
    
    // Set up default values
    x->a_list[0].a_type = A_SYM;
    x->a_list[0].a_w.w_sym = gensym("Message");
    atom_setlong(x->a_list + 1,0);
    x->a_list[2].a_type = A_SYM;
    x->a_list[2].a_w.w_sym = gensym("OK");
    atom_setlong(x->a_list + 3,0);
    atom_setlong(x->a_list + 4,0);
    
    
    // Add typed in object parameters
    if (argc>0) aalert_set(x, gensym("set"), argc, argv);
    
    return x;
}


//--------------------------------------------------------------------------
// BANG - shows alert box
void aalert_bang(t_aalert *x)
{
    defer_low(x,(method)aalert_showbox,0,0,0);
    //defer(x,(method)aalert_showbox,0,0,0);
    //aalert_showbox(x);
}

void aalert_showbox(t_aalert *x)
{
    CFTimeInterval timeout;
    CFOptionFlags flags;
    CFURLRef iconURL;
    CFURLRef soundURL;
    CFURLRef localizationURL;
    CFStringRef alertHeader;
    CFStringRef alertMessage;
    CFStringRef defaultButtonTitle;
    CFStringRef alternateButtonTitle;
    CFStringRef otherButtonTitle;
    unsigned long itemHit;
    
    long cvtbuflen = 0; // length of buffer on output


    
    timeout = 1000;
    flags = kCFUserNotificationNoteAlertLevel;
    iconURL = NULL;
    soundURL = NULL;
    localizationURL = NULL;

    alertHeader = CFStringCreateWithCharacters(NULL, charset_utf8tounicode ( x->a_list[0].a_w.w_sym->s_name, &cvtbuflen ), cvtbuflen);

    // add explanation text if defined
    if (x->a_list[1].a_type == A_SYM)
        alertMessage = CFStringCreateWithCharacters(NULL, charset_utf8tounicode ( x->a_list[1].a_w.w_sym->s_name, &cvtbuflen ), cvtbuflen);
    else
        alertMessage = NULL;
    
    // set default button text if defined
    if (x->a_list[2].a_type == A_SYM)
    {
        defaultButtonTitle = CFStringCreateWithCharacters(NULL, charset_utf8tounicode ( x->a_list[2].a_w.w_sym->s_name, &cvtbuflen ), cvtbuflen);;
    }
    else
        defaultButtonTitle = NULL;
    
    // set cancel button text if defined
    if (x->a_list[4].a_type == A_SYM)
    {
        alternateButtonTitle = CFStringCreateWithCharacters(NULL, charset_utf8tounicode ( x->a_list[4].a_w.w_sym->s_name, &cvtbuflen ), cvtbuflen);;
   }
    else
        alternateButtonTitle = NULL;
    
    // set other button text if defined
    if (x->a_list[3].a_type == A_SYM)
    {
        otherButtonTitle = CFStringCreateWithCharacters(NULL, charset_utf8tounicode ( x->a_list[3].a_w.w_sym->s_name, &cvtbuflen ), cvtbuflen);;
   } else
       otherButtonTitle = NULL;

    
    CFUserNotificationDisplayAlert( timeout,  flags,  iconURL,  soundURL,  localizationURL,  alertHeader,  alertMessage,  defaultButtonTitle,  alternateButtonTitle,  otherButtonTitle, &itemHit);
    
   // post("Hit %d\n",itemHit);
    // see which button has been pressed and bang the appropriate outlet
    switch (itemHit)
    {
        case 1:
            outlet_bang(x->a_out3);
            break;
        case 2:
            outlet_bang(x->a_out2);
            break;
        case 3:         //escape key
            if (otherButtonTitle)  outlet_bang(x->a_out2);
            else  aalert_bang(x);
            break;
        default:
            outlet_bang(x->a_out1);
            break;
    }
    
    
}



//--------------------------------------------------------------------------
// Change alert text & buttons

void aalert_set(t_aalert *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    x->a_size = argc;
    if (x->a_size > MAX_ARGS) x->a_size = MAX_ARGS;
    for(i = 0; i < x->a_size; i++)
    {
        
        // type check each argument
        if (argv[i].a_type == A_SYM)
        {
            atom_setsym(x->a_list + i, argv[i].a_w.w_sym);
        } else
        {
            switch (i)
            {
                    // don't change message if no message given
                case 0:
                    break;
                    
                default:
                    atom_setlong(x->a_list + i, argv[i].a_w.w_long);
                    break;
            }
        }
    } 
}  


//--------------------------------------------------------------------------
// Assist outlets in patcher

void aalert_assist(t_aalert *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET)
        sprintf(s,"bang: show dialog box/set: change text");
    else 
    {
        switch (a) 
        {	
            case 0:
                sprintf(s,"Bang if default button clicked (ok)");
                break;
            case 1:
                sprintf(s,"Bang if cancel button clicked");
                break;
            case 2:
                sprintf(s,"Bang if leftmost button clicked");
                break;
        }
    }
    
}
