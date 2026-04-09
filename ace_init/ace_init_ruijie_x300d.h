#ifndef __ACE_INIT_RUIJIE_X300D_H
#define __ACE_INIT_RUIJIE_X300D_H

#ifndef ARCH_ARM64
extern int X300D_set_map_port_pre(void);
extern int X300D_set_map_port(void);
extern int X300D_set_map_port_post(void);
extern int X300D_def_hylab_vpp_cmd_conf(void);
extern int X300D_update_hylab_vpp_cmd_conf(void);
#endif

#endif

