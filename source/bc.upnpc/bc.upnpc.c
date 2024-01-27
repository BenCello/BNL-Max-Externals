/**
 @file
 bc_upnpc - a max object shell
 jeremy bernstein - jeremy@bootsquad.com
 
 @ingroup	examples
 */

#include "ext.h"							// standard Max include, always required
#include "ext_obex.h"						// required for new style Max object

//// miniUPnP headers
#include "upnpcommands.h"
#include "miniupnpc.h"
#include "upnperrors.h"

#define MAX_CHAR_DESC 128

/// should be in .h
enum {
  NO_QUERY = 0,
  DEVICES,
  IP_INFO,
  LIST,
  ADD,
  REMOVE
};

struct upnp_dev_list {
  struct upnp_dev_list *next;
  char * descURL;
  struct UPNPDev **array;
  size_t count;
  size_t allocated_count;
};

struct upnp_map_list {
  struct upnp_map_list *next;
  char intClient[40];
  char intPort[6];
  char extPort[6];
  char protocol[4];
  char enabled[6];
  char duration[16];
  char description[MAX_CHAR_DESC];
};

extern void add_device(struct upnp_dev_list **list_head, struct UPNPDev * dev);
extern void free_device(struct upnp_dev_list * elt);

////////////////////////// object struct
typedef struct _bc_upnpc
{
  t_object		        ob;			            // the object itself (must be first)
  t_systhread         x_systhread;        // thread reference
  t_systhread_mutex   x_mutex;            // mutual exclusion lock for threadsafety
  void                *x_qelem;           // for message passing between threads
  int                 x_task;             // current task running
  long                x_discover;         // type of UPnP discovery (all/IGD)
  void                *x_outlet;          // outlet 1
  void                *x_outlet2;         // outlet 2
  //// UPnP data
  struct upnp_dev_list  *x_devices;          // devices list
  struct UPNPUrls       x_urls;             // gateway URLs
  struct IGDdatas       x_data;             // gateway data
  t_symbol              *x_IGDaddr;         // gateway IP address on LAN
  t_symbol              *x_extaddr;         // gateway external IP address
  struct upnp_map_list  *x_routes;          // existing route list
  //// local data
  t_symbol              *x_lanaddr;         // my IP address on the LAN
  //// temporary route data
  t_symbol  *tmp_ip;
  long      tmp_iport;
  long      tmp_oport;
  t_symbol  *tmp_proto;
  long      tmp_dur;
  t_symbol  *tmp_desc;
} t_bc_upnpc;


///////////////////////// function prototypes
//// standard set
void *bc_upnpc_new(t_symbol *s, long argc, t_atom *argv);
void bc_upnpc_free(t_bc_upnpc *x);
void bc_upnpc_assist(t_bc_upnpc *x, void *b, long m, long a, char *s);

//// thread handling
void bc_upnpc_stop(t_bc_upnpc *x);
void bc_upnpc_join(t_bc_upnpc *x);
void bc_upnpc_callback(t_bc_upnpc *x);

//// thread callers
void bc_upnpc_devices(t_bc_upnpc *x);
void bc_upnpc_ipinfo(t_bc_upnpc *x);
void bc_upnpc_list(t_bc_upnpc *x);
void bc_upnpc_add(t_bc_upnpc *x, t_symbol *ip, long iport, long oport);
void bc_upnpc_addlist(t_bc_upnpc *x, t_symbol *msg, long argc, t_atom *argv);
void bc_upnpc_remove(t_bc_upnpc *x, long oport);

//// threaded methods
void *bc_upnpc_thdevices(t_bc_upnpc *x);
void *bc_upnpc_thipinfo(t_bc_upnpc *x);
void *bc_upnpc_thlist(t_bc_upnpc *x);
void *bc_upnpc_thadd(t_bc_upnpc *x);
void *bc_upnpc_thremove(t_bc_upnpc *x);

//// in/out methods
void bc_upnpc_busy(t_bc_upnpc *x, bool busy);

//// utilities
void bc_upnpc_freedevices(struct upnp_dev_list *list);
void bc_upnpc_freeroutes(struct upnp_map_list *list);

//////////////////////// global class pointer variable
void *bc_upnpc_class;


///////////////////////// standard set methods
void ext_main(void *r)
{
  t_class *c;
  
  c = class_new("bc.upnpc", (method)bc_upnpc_new, (method)bc_upnpc_free, (long)sizeof(t_bc_upnpc),
                0L /* leave NULL!! */, A_GIMME, 0);
  
  class_addmethod(c, (method)bc_upnpc_assist,		"assist",   A_CANT, 0);
  class_addmethod(c, (method)bc_upnpc_devices,  "devices",  0);
  class_addmethod(c, (method)bc_upnpc_ipinfo,  "ipinfo",  0);
  class_addmethod(c, (method)bc_upnpc_list,  "maplist",  0);
//  class_addmethod(c, (method)bc_upnpc_addlist, "add2", A_GIMME, 0);
  class_addmethod(c, (method)bc_upnpc_add, "add", A_SYM, A_LONG, A_LONG, 0);
  class_addmethod(c, (method)bc_upnpc_remove, "remove", A_LONG, 0);
  
  CLASS_ATTR_LONG(c, "discover", 0, t_bc_upnpc, x_discover);
  CLASS_ATTR_ENUMINDEX(c, "discover", 0, "all gateway");
  CLASS_ATTR_LABEL(c, "discover", 0, "UPnP devices");
  
  class_register(CLASS_BOX, c); /* CLASS_NOBOX */
  bc_upnpc_class = c;
}

void bc_upnpc_assist(t_bc_upnpc *x, void *b, long m, long a, char *s)
{
  if (m == ASSIST_INLET) { // inlets
    switch(a) {
      case 0: sprintf(s, "UPnP Messages (devices, ipinfo, etc.)"); break;
    }
  }
  else {	// outlets
    switch(a) {
      case 0: sprintf(s, "UPnP info"); break;
      case 1: sprintf(s, "busy flag"); break;
    }
  }
}

void bc_upnpc_free(t_bc_upnpc *x)
{
  // stop our thread if it is still running
  bc_upnpc_stop(x);
  
  // free our qelem
  if (x->x_qelem)
    qelem_free(x->x_qelem);
  
  // free out mutex
  if (x->x_mutex)
    systhread_mutex_free(x->x_mutex);
  
  // free lists
  bc_upnpc_freedevices(x->x_devices);
  bc_upnpc_freeroutes(x->x_routes);
}

void *bc_upnpc_new(t_symbol *s, long argc, t_atom *argv)
{
  t_bc_upnpc *x = NULL;
  
  // objects
  x = (t_bc_upnpc *)object_alloc(bc_upnpc_class);
  x->x_outlet2 = intout(x);
  x->x_outlet = outlet_new(x,NULL);
  // attributes
  object_attr_setlong(x, gensym("discover"), 1);
  
  // data
  x->x_devices = NULL;
  x->x_routes = NULL;
  x->x_extaddr = gensym("unset");
  x->x_lanaddr = gensym("127.0.0.1");
  x->x_IGDaddr = gensym("unset");
  
  // Threading
  x->x_systhread = NULL;
  systhread_mutex_new(&x->x_mutex,0);
  x->x_qelem = qelem_new(x,(method)bc_upnpc_callback);
  
  // Example
  //    long i;
  //
  //    if ((x != NULL)) {
  //        object_post((t_object *)x, "a new %s object was instantiated: %p", s->s_name, x);
  //        object_post((t_object *)x, "it has %ld arguments", argc);
  //
  //        for (i = 0; i < argc; i++) {
  //            if ((argv + i)->a_type == A_LONG) {
  //                object_post((t_object *)x, "arg %ld: long (%ld)", i, atom_getlong(argv+i));
  //            } else if ((argv + i)->a_type == A_FLOAT) {
  //                object_post((t_object *)x, "arg %ld: float (%f)", i, atom_getfloat(argv+i));
  //            } else if ((argv + i)->a_type == A_SYM) {
  //                object_post((t_object *)x, "arg %ld: symbol (%s)", i, atom_getsym(argv+i)->s_name);
  //            } else {
  //                object_error((t_object *)x, "forbidden argument");
  //            }
  //        }
  //    }
  
  return (x);
}

///////////////////////// thread handling methods

void bc_upnpc_stop(t_bc_upnpc *x)
{
  unsigned int ret;
  
  if (x->x_systhread) {
    systhread_join(x->x_systhread, &ret);   // wait for the thread to stop
    x->x_systhread = NULL;
  }
}

void bc_upnpc_join(t_bc_upnpc *x)
{
  unsigned int ret;
  
  if (x->x_systhread) {
    systhread_join(x->x_systhread, &ret);   // wait for the thread to stop
    x->x_systhread = NULL;
  }
}

void bc_upnpc_callback(t_bc_upnpc *x)
{
  unsigned int ret = 0;
  // first get back the thred
  systhread_join(x->x_systhread, &ret);   // wait for the thread to stop
  x->x_systhread = NULL;
  //  post("upnpDiscover() error code=%d\n", ret);
  
  // tell the outside world it returned
  bc_upnpc_busy(x, 0);
  
  if (ret)
  {
    t_atom errnum;
    atom_setlong(&errnum, (int)ret);
    outlet_anything(x->x_outlet, gensym("error"), 1, &errnum);
  }
  
  t_atom a[7];
  
  switch (x->x_task) {
    case 0: break;
    case DEVICES:
      if (ret == UPNPCOMMAND_SUCCESS)
      {
        struct upnp_dev_list * device;
        if(x->x_devices)
        {
          // remove http:// and everything after the IP
          char qolon = ':';
          char *start = x->x_urls.controlURL_6FC+7;
          char *found = strchr(start, qolon);
          size_t len = found - start;
          char *ipaddr = malloc(len + 1);
          memcpy(ipaddr, start, len);
          ipaddr[len] = '\0';
          
          x->x_IGDaddr = gensym(ipaddr);
          for(device = x->x_devices; device != NULL ; device = device->next)
          {
            atom_setsym(a, gensym(device->descURL));
            outlet_anything(x->x_outlet, gensym("device"), 1, a);
          }
        }
      }
      break;
    case IP_INFO:
      if (ret == UPNPCOMMAND_SUCCESS)
      {
        atom_setsym(a, gensym("local"));
        atom_setsym(a+1, x->x_lanaddr);
        outlet_anything(x->x_outlet, gensym("ip"), 2, a);
        atom_setsym(a, gensym("gateway"));
        atom_setsym(a+1, x->x_IGDaddr);
        outlet_anything(x->x_outlet, gensym("ip"), 2, a);
        atom_setsym(a, gensym("external"));
        atom_setsym(a+1, x->x_extaddr);
        outlet_anything(x->x_outlet, gensym("ip"), 2, a);
      }
      break;
    case LIST:
    {
      struct upnp_map_list *mapping = x->x_routes;
      if (ret == UPNPCOMMAND_SUCCESS)
      {
        while(mapping != NULL)
        {
          atom_setlong(a, strtol(mapping->extPort, NULL, 10));
          atom_setsym(a+1, gensym(mapping->intClient));
          atom_setlong(a+2, strtol(mapping->intPort, NULL, 10));
          atom_setsym(a+3, gensym(mapping->protocol));
          atom_setlong(a+4, strtol(mapping->enabled, NULL, 10));
          atom_setlong(a+5, strtol(mapping->duration, NULL, 10));
          atom_setsym(a+6, gensym(mapping->description));
          outlet_anything(x->x_outlet, gensym("map"), 7, a);
          mapping = mapping->next;
        }
      }
      break;
    }
  }
}


///////////////////////// threaded methods

void *bc_upnpc_thdevices(t_bc_upnpc *x)
{
  struct UPNPDev * devlist = 0;
  struct UPNPDev * dev;
  struct upnp_dev_list * sorted_list = NULL;
  struct UPNPUrls urls;
  struct IGDdatas data;
  char lanaddr[64] = "unset";
  const char * multicastif = 0;
  const char * minissdpdpath = 0;
  int localport = UPNP_LOCAL_PORT_ANY;
  //    int retcode = 0;
  int i,error = 0;
  int ipv6 = 0;
  unsigned char ttl = 2;    /* defaulting to 2 */
  
  switch(x->x_discover)
  {
    case 0:
      devlist = upnpDiscoverAll(2000, multicastif, minissdpdpath, localport, ipv6, ttl, &error);
      break;
    case 1:
      devlist = upnpDiscover(2000, multicastif, minissdpdpath, localport, ipv6, ttl, &error);
  }
  
  if(devlist)
  {
    for(dev = devlist, i = 1; dev != NULL; dev = dev->pNext, i++)
      add_device(&sorted_list, dev);
    
    i = 1;
    i = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
    if (i == 1)
    {
      struct upnp_dev_list * old_list = x->x_devices;
      systhread_mutex_lock(x->x_mutex);
      x->x_devices  = sorted_list;
      x->x_urls = urls;
      x->x_data = data;
      x->x_lanaddr = gensym(lanaddr);
      systhread_mutex_unlock(x->x_mutex);
      bc_upnpc_freedevices(old_list);
    }
  }
  
  // free memory
  freeUPNPDevlist(devlist);
  
//  post("upnpDiscover() error code=%d\n", error);
  qelem_set(x->x_qelem);    // notify main thread
  systhread_exit(error);    // then kill thread
  return NULL;
}

void *bc_upnpc_thipinfo(t_bc_upnpc *x)
{
  struct UPNPUrls urls = x->x_urls;
  struct IGDdatas data = x->x_data;
  
  char externalIPAddress[40];
  int ret;
  ret = UPNP_GetExternalIPAddress(urls.controlURL,
                                  data.first.servicetype,
                                  externalIPAddress);
  if(ret == UPNPCOMMAND_SUCCESS)
  {
    systhread_mutex_lock(x->x_mutex);
    x->x_extaddr = gensym(externalIPAddress);
    systhread_mutex_unlock(x->x_mutex);
  }
  
//  post("UPNP_GetExternalIPAddress() error code=%d\n", ret);
  qelem_set(x->x_qelem);    // notify main thread
  systhread_exit(ret);    // then kill thread
  return NULL;
}

void *bc_upnpc_thlist(t_bc_upnpc *x)
{
  struct UPNPUrls urls = x->x_urls;
  struct IGDdatas data = x->x_data;

  int ret = 0;
  int i = 0;
  char index[6];
  char intClient[40];
  char intPort[6];
  char extPort[6];
  char protocol[4];
  char desc[MAX_CHAR_DESC];
  char enabled[6];
  char rHost[64];
  char duration[16];
  
  struct upnp_map_list *previous = NULL;
  
  struct upnp_map_list *old_routes = x->x_routes;
  systhread_mutex_lock(x->x_mutex);
  x->x_routes = NULL;
  systhread_mutex_unlock(x->x_mutex);
  bc_upnpc_freeroutes(old_routes);
  
  do {
    snprintf(index, 6, "%d", i);
    intClient[0] = '\0';
    intPort[0] = '\0';
    extPort[0] = '\0';
    protocol[0] = '\0';
    desc[0] = '\0';
    enabled[0] = '\0';
    rHost[0] = '\0';
    duration[0] = '\0';
    
    ret = UPNP_GetGenericPortMappingEntry(urls.controlURL,
                                        data.first.servicetype,
                                        index,
                                        extPort, intClient, intPort,
                                        protocol, desc, enabled,
                                        rHost, duration);
    if(ret == 0)
    {
//      post("%2d %s %5s->%s:%-5s '%s' '%s' %s\n",
//             i, protocol, extPort, intClient, intPort,
//             desc, rHost, duration);
      struct upnp_map_list *mapping;
      mapping = malloc(sizeof(struct upnp_map_list));
      if (mapping == NULL)
        post("failed to malloc", (unsigned long)sizeof(struct upnp_map_list));
      else
      {
        mapping->next = NULL;
        strncpy(mapping->intClient, intClient, 40);
        strncpy(mapping->intPort, intPort, 6);
        strncpy(mapping->extPort, extPort, 6);
        strncpy(mapping->protocol, protocol, 4);
        strncpy(mapping->enabled, enabled, 6);
        strncpy(mapping->duration, duration, 16);
        strncpy(mapping->description, desc, MAX_CHAR_DESC);
        if (previous == NULL)
        {
          systhread_mutex_lock(x->x_mutex);
          x->x_routes = mapping;
          systhread_mutex_unlock(x->x_mutex);
        }
        else
          previous->next = mapping;
        previous = mapping;
      }
    }
    i++;
  } while (ret == 0);
  
  //  post("UPNP_GetExternalIPAddress() error code=%d\n", ret);
  qelem_set(x->x_qelem);    // notify main thread
  systhread_exit(ret==713?0:ret);    // then kill thread
  return NULL;
}

void *bc_upnpc_thadd(t_bc_upnpc *x)
{
  struct UPNPUrls urls = x->x_urls;
  struct IGDdatas data = x->x_data;
  
  int retUDP = 0;
  int retTCP = 0;
  char iport[10];
  char eport[10];
  snprintf(iport, 10, "%ld", x->tmp_iport);
  snprintf(eport, 10, "%ld", x->tmp_oport);
  
  retUDP = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                          eport, iport, x->tmp_ip->s_name,
                          x->tmp_desc->s_name, "UDP",
                          NULL, 0);
  //  post("UPNP_AddPortMapping() for UDP error code=%d\n", retUDP);
  retTCP = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                            eport, iport, x->tmp_ip->s_name,
                            x->tmp_desc->s_name, "TCP",
                            NULL, 0);
  //  post("UPNP_AddPortMapping() for TCP error code=%d\n", retTCP);
  
  qelem_set(x->x_qelem);    // notify main thread
  systhread_exit(retUDP+retTCP);    // then kill thread
  return NULL;
}

void *bc_upnpc_thremove(t_bc_upnpc *x)
{
  struct UPNPUrls urls = x->x_urls;
  struct IGDdatas data = x->x_data;
  
  int retUDP = 0;
  int retTCP = 0;
  char eport[10];
  snprintf(eport, 10, "%ld", x->tmp_oport);
  
  retUDP = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype,
                                  eport, "UDP", NULL);
//  post("UPNP_DeletePortMapping() for UDP error code=%d\n", retUDP);
  retTCP = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype,
                                  eport, "TCP", NULL);
//  post("UPNP_DeletePortMapping() for TCP error code=%d\n", retTCP);
  
  object_post((t_object *)x, "%d UPnP redirection(s) removed", (retUDP == 0)+(retTCP == 0));
  qelem_set(x->x_qelem);    // notify main thread
  systhread_exit(retUDP*retTCP);    // then kill thread
  return NULL;
}

///////////////////////// callers methods

void bc_upnpc_devices(t_bc_upnpc *x)
{
  x->x_task = DEVICES;
  
  // create new thread + begin execution
  if (x->x_systhread == NULL) {
    object_post((t_object *)x, "starting UPnP discovery");
    systhread_create((method) bc_upnpc_thdevices, x, 0, 0, 0, &x->x_systhread);
    bc_upnpc_busy(x, 1);
  }
  else
    bc_upnpc_busy(x, 1);
}

void bc_upnpc_ipinfo(t_bc_upnpc *x)
{
  if (x->x_IGDaddr == gensym("unset"))
  {
    object_post((t_object *)x, "discover UPnP devices beforehand\nuse 'devices' message");
    return ;
  }
  
  x->x_task = IP_INFO;
  
  // create new thread + begin execution
  if (x->x_systhread == NULL) {
    object_post((t_object *)x, "gathering IP info");
    systhread_create((method) bc_upnpc_thipinfo, x, 0, 0, 0, &x->x_systhread);
    bc_upnpc_busy(x, 1);
  }
  else
    bc_upnpc_busy(x, 1);
}

void bc_upnpc_list(t_bc_upnpc *x)
{
  if (x->x_IGDaddr == gensym("unset"))
  {
    object_post((t_object *)x, "discover UPnP devices beforehand\nuse 'devices' message");
    return ;
  }
  
  x->x_task = LIST;
  
  // create new thread + begin execution
  if (x->x_systhread == NULL) {
    object_post((t_object *)x, "listing existing UPnP redirections");
    systhread_create((method) bc_upnpc_thlist, x, 0, 0, 0, &x->x_systhread);
    bc_upnpc_busy(x, 1);
  }
  else
    bc_upnpc_busy(x, 1);
}

void bc_upnpc_add(t_bc_upnpc *x, t_symbol *ip, long iport, long oport)
{
  if (x->x_IGDaddr == gensym("unset"))
  {
    object_post((t_object *)x, "discover UPnP devices beforehand\nuse 'devices' message");
    return ;
  }
  
  x->x_task = ADD;
  x->tmp_ip = ip;
  x->tmp_iport = iport;
  x->tmp_oport = oport;
  x->tmp_desc = gensym("Max [bc.upnpc] object with libminiupnpc");
  
  // create new thread + begin execution
  if (x->x_systhread == NULL) {
    object_post((t_object *)x, "creating UPnP redirection");
    systhread_create((method) bc_upnpc_thadd, x, 0, 0, 0, &x->x_systhread);
    bc_upnpc_busy(x, 1);
  }
  else
    bc_upnpc_busy(x, 1);
}

void bc_upnpc_addlist(t_bc_upnpc *x, t_symbol *msg, long argc, t_atom *argv)
{
  
}

void bc_upnpc_remove(t_bc_upnpc *x, long oport)
{
  if (x->x_IGDaddr == gensym("unset"))
  {
    object_post((t_object *)x, "discover UPnP devices beforehand\nuse 'devices' message");
    return ;
  }
  
  x->x_task = REMOVE;
  x->tmp_oport = oport;
  
  // create new thread + begin execution
  if (x->x_systhread == NULL) {
//    object_post((t_object *)x, "removing UPnP redirection");
    systhread_create((method) bc_upnpc_thremove, x, 0, 0, 0, &x->x_systhread);
    bc_upnpc_busy(x, 1);
  }
  else
    bc_upnpc_busy(x, 1);
}

///////////////////////// in/out methods

void bc_upnpc_busy(t_bc_upnpc *x, bool busy)
{
  if (busy)
    outlet_int(x->x_outlet2, 1);
  else
    outlet_int(x->x_outlet2, 0);
}

///////////////////////// utilities methods

void bc_upnpc_freedevices(struct upnp_dev_list *list)
{
  struct upnp_dev_list * next;
  while(list != NULL) {
    next = list->next;
    free_device(list);
    list = next;
  }
}

void bc_upnpc_freeroutes(struct upnp_map_list *list)
{
  struct upnp_map_list *next;
  while(list != NULL)
  {
    next = list->next;
    free(list);
    list = next;
  }
}

