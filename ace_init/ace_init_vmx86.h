#ifndef __ACE_INIT_VMGUEST_H
#define __ACE_INIT_VMGUEST_H

extern int vmx86_set_map_port_pre(void);
extern int vmx86_set_map_port(void);
extern int vmx86_set_map_port_post(void);
extern int vmx86_def_hylab_vpp_cmd_conf(void);
extern int vmx86_update_hylab_vpp_cmd_conf(void);

#endif
