#include <unistd.h>
#include "Collector_conf.h"
#include "CollectorData.h"
#include "procinfo.h"
#include <errno.h>
#include "Debug_handle.h"
#include "ace_common_macro.h"
#include "ace_common_struct.h"
#include "ace_share_mem_inc.h"
#include "ace_share_kernel_inc.h"
#include <sys/wait.h>
#include <sys/un.h>
#include <stddef.h>
#include <sys/prctl.h>
#include <sys/shm.h>


extern ACE_SHARE_REPORTER_INFO* g_reporter_info;
extern int reporter_shm_get();
extern int reporter_shm_acquire();
extern int reporter_shm_release();
int g_dev_stat_dbg = 0;
int g_new_session_dbg;
int g_cpu_stat_dbg = 0;

namespace collector{

unsigned int *g_ip_conntrack_new = NULL;
extern unsigned int g_connection_num;
void* get_vpp_ifcpus_stat_share_memroy(void)
{
	int key;
	int shmID;
	void* node_ptr;
	size_t shm_len;
	struct shmid_ds shm_info;

	if(-1 == (key = ftok(HYLAB_SHM_IFCPUS_STAT_FILE_NAME, 0xFF))) 
	{ 
		debug_save("%s %s %d --- ftok %s failed", __FUNCTION__,__FILE__,__LINE__,HYLAB_SHM_IFCPUS_STAT_FILE_NAME);
		return NULL;
	} 


	shmID = shmget(key, 0, 0);
	if(-1 == shmID)
	{
		debug_save("%s %s %d --- shmget %s failed", __FUNCTION__,__FILE__,__LINE__,HYLAB_SHM_IFCPUS_STAT_FILE_NAME);
		return NULL;
	}

	node_ptr = shmat(shmID, NULL, 0);

	if(((void*)-1) == node_ptr)
	{
		debug_save("%s %s %d --- shmat %s failed", __FUNCTION__,__FILE__,__LINE__,HYLAB_SHM_IFCPUS_STAT_FILE_NAME);
		return NULL;
	}

	shm_len = sizeof(VPP_IFCPUS_STAT_T);
	shm_len += sizeof(VPP_CPU_PERTHREAD_STAT_T) * VPP_CPU_NUM(node_ptr);
	shm_len += sizeof(VPP_IF_INFO_T) * VPP_IFS_NUM(node_ptr);
	shm_len += sizeof(VPP_IF_PERTHREAD_STAT_T) * VPP_CPU_NUM(node_ptr) * VPP_IFS_NUM(node_ptr);

	if (0 == shmctl(shmID, IPC_STAT, &shm_info))
    {
        if (shm_len == shm_info.shm_segsz)
        {
            debug_save("%s %s %d --- shmctl %s size %lu if %u cpu %u ok", __FUNCTION__,__FILE__,__LINE__,HYLAB_SHM_IFCPUS_STAT_FILE_NAME,shm_len,VPP_IFS_NUM(node_ptr),VPP_CPU_NUM(node_ptr));
        }
        else
        {
            debug_save("%s %s %d --- shmctl %s size cal %lu now %lu failed failed", __FUNCTION__,__FILE__,__LINE__,HYLAB_SHM_IFCPUS_STAT_FILE_NAME,shm_len,shm_info.shm_segsz);
			shmdt(node_ptr);
            return NULL;
        }
    }
	return node_ptr;
}

Cprocinfo::Cprocinfo(CRealtimeMonitor* realtimemonitor)
{
    prealtimemonitor = realtimemonitor;
    last_drop_cache_time = 0;
    vpp_cpu_num = 0;
}

Cprocinfo::~Cprocinfo()
{

}
CLLCT_STATUS Cprocinfo::Init()
{
    char buf[512];
	vpp_cpu_num = 0;
    memset(&proc_info, 0, sizeof(struct proc_info_t));
    FILE *fp;
    if (access("/etc/dev_stat_dbg", 0) == 0)
        g_dev_stat_dbg = 1;
    if (access("/etc/cpu_stat_dbg", 0) == 0)
        g_cpu_stat_dbg = 1;

    int byte_num;
    char* p_tmp;
    char* p_tail;
    char* p_head;
    int cpu_start;
    int cpu_end;
    //BOOT_IMAGE=/bzImage root=LABEL=hyroot ro crashkernel=auto console=ttyS0,115200n8 net.ifnames=0 biosdevname=0 scandelay=10 isolcpus=1,4,5,6,7,8,9,10,11 nohz_full=1,4,5,6,7,8,9,10,11 rcu_nocbs=1,4,5,6,7,8,9,10,11 rhgb domdadm
    fp = fopen("/proc/cmdline", "r");
    if (fp == NULL) {
        debug_save("Cannot open /proc/cmdline");
        goto error_out;
    }
    byte_num = fread( buf, 1, sizeof(buf)-1, fp);
    if (byte_num > 0)
    {
        buf[byte_num] = 0;
    }
    p_tmp = strstr(buf, "isolcpus=");
    if (p_tmp)
    {
    	p_tmp += sizeof("isolcpus=")-1;
    	p_tail = strchr(p_tmp, ' ');
		if (p_tail)
		{
            *p_tail = 0;
            //isolcpus=1,4,5,6,7,8,9,10,11
            if (strchr(p_tmp, ','))
            {
    			for (p_head = p_tmp; p_head <= p_tail && *p_head; ++p_head)
    		    {
    		        if (',' == *p_head)
    		        {
    		            *p_head = 0;
    		            vpp_cpus[vpp_cpu_num++] = atoi(p_tmp);
                        if (vpp_cpu_num >= 32)
                            break;
    		            p_tmp = ++p_head;
    		        }
    		    }
            }
            //isolcpus=1-7
            else if (strchr(p_tmp, '-'))
            {
                for (p_head = p_tmp; p_head <= p_tail && *p_head; ++p_head)
                {
                    if ('-' == *p_head)
                    {
                        *p_head = 0;
                        cpu_start = atoi(p_tmp);
                        cpu_end = atoi(p_head + 1);
                        while(cpu_start <= cpu_end && vpp_cpu_num < 32)
                        {
                            vpp_cpus[vpp_cpu_num++] = cpu_start;
                            cpu_start++;
                        }
                    }
                }
            }
            //isolcpus=1
            else
            {
                cpu_start = atoi(p_tmp);
                vpp_cpus[vpp_cpu_num++] = cpu_start;
            }
		}
		else
		{
			debug_save("wrong /proc/cmdline");
		}
    }
    else
    {
        debug_save("wrong /proc/cmdline");
    }
    
    fclose(fp);
	fp = NULL;
	p_vpp_ifcpus_stat = get_vpp_ifcpus_stat_share_memroy();
	if (NULL == p_vpp_ifcpus_stat)
	{		
		debug_save("%s %s %d --- get_vpp_ifcpus_stat_share_memroy %s failed\n", __FUNCTION__,__FILE__,__LINE__,HYLAB_SHM_IFCPUS_STAT_FILE_NAME);
		goto error_out;
	}
	if (vpp_cpu_num != VPP_CPU_NUM(p_vpp_ifcpus_stat))
	{
		debug_save("%s %s %d --- wrong vpp_cpu_num %u in /proc/cmdline %u in shm %s\n", __FUNCTION__,__FILE__,__LINE__,vpp_cpu_num,VPP_CPU_NUM(p_vpp_ifcpus_stat),HYLAB_SHM_IFCPUS_STAT_FILE_NAME);
	}

    return CLLCT_OK;

error_out:
    if (fp)
        fclose(fp);
    return CLLCT_ERR;
}

void* Cprocinfo::timerd(void*)
{
    time_t now,last=0;

	prctl(PR_SET_NAME, "Cprocinfo_timerd");
    while(1)
    {
        now = time(NULL);
        if(now/REALTIMEMONITOR_PHYIF_SECOND*REALTIMEMONITOR_PHYIF_SECOND != last)
        {
            pCollectorData->procinfo->Termly(now);

            last = now/REALTIMEMONITOR_PHYIF_SECOND *REALTIMEMONITOR_PHYIF_SECOND;
        }
        if(!running_status)
        {
            break;
        }
        sleep(1); /* one second */
    }

    debug_save("%s %d WARNING: thread %s is stop!\n", __FILE__, __LINE__, __FUNCTION__);  
	return NULL;  
}
void Cprocinfo::Termly(time_t now)
{
    float rate;
    phyif_value_t phyif;
    memset(&phyif, 0, sizeof(phyif_value_t));
    read_proc_net_dev(now, &phyif);
    
    resource_value_t resource;
    memset(&resource, 0, sizeof(resource_value_t));
    read_proc_stat(now, &resource);
	
    read_proc_new_session(now, &resource);
    read_proc_meminfo(&resource);
    read_proc_sys_net_ipv4_netfilter_ip_conntrack_count(&resource);
    read_proc_sys_net_ipv6_netfilter_ip_conntrack_count(&resource);
    read_proc_sys_net_ipv4_netfilter_ip_conntrack_max(&resource);
    /* session accelerate aging*/
    rate = (float)(resource.sessions)/(float)(resource.sessions_max);
    resource.sessions_max = read_license_ctrl_number(resource.sessions_max);
    prealtimemonitor->InsertPhyifResource(now, &phyif, &resource);
    proc_session_accelerate_aging((int)(rate*100));
}

unsigned int g_shcd_last_time = 0;
unsigned long g_shcd_pkt_error = 0;
unsigned long g_shcd_pkt_drop = 0;
int g_dev_stat_by_ethtool = 0;

void Cprocinfo::read_proc_net_dev(time_t now,phyif_value_t* pvalue)
{
    char name[IFNAMSIZ] = { 0 };
    char *p;
	U32 calc_kbytes;
    resource_value_t resource;
    unsigned long long rx_bytes, tx_bytes;
    unsigned long if_speed = 0;
    unsigned int if_mtu;
    char if_state[8] = "up";
    char buf[512];
#if 0
    FILE *fp = vpopen("/usr/local/vpp/bin/vppctl show interface", "r");
    if (fp == NULL) {
        debug_save("Cannot open /usr/local/vpp/bin/vppctl show interface errno %d", errno);
        return;
    }


    /* skip headers */
    fgets(buf, sizeof(buf), fp);
/*
                  Name               Idx    State  MTU (L3/IP4/IP6/MPLS)     Counter          Count     
    LAN1                              1      up          9000/0/0/0     rx packets                    42
                                                                        rx bytes                    2520
                                                                        drops                         27
                                                                        punt                          15
                                                                        if state                       up
                                                                        if speed              1000000
*/
    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        if (buf[0] != ' ' && !(buf[0] == 'i' && buf[1] == 'f'))
        {
            if (name[0])
            {
                S32 idx = pCollectorData->get_phyif_idx(name);
                if(-1 != idx)
                {
	                if(proc_info.phyif_info.last_phyif[idx].time)
	                {
	                    int difft = now - proc_info.phyif_info.last_phyif[idx].time;
	                    
	                    /**zhang.huanan uint:KBytes for 4G overflow**/
	                    if (rx_bytes > proc_info.phyif_info.last_phyif[idx].rx_bytes)
	                    {
	                        if (difft > 0)
	                            pvalue->flow[idx].rcvd_byte = (rx_bytes-proc_info.phyif_info.last_phyif[idx].rx_bytes) / 1000 * REALTIMEMONITOR_PHYIF_SECOND / difft;
	                        else
	                            pvalue->flow[idx].rcvd_byte = (rx_bytes-proc_info.phyif_info.last_phyif[idx].rx_bytes) / 1000;
	                    }
	                    else if (rx_bytes == proc_info.phyif_info.last_phyif[idx].rx_bytes)
	                    {
	                        pvalue->flow[idx].rcvd_byte = 0;
	                    }
	                    else
	                    {
	                        if (difft)
	                            calc_kbytes = (0xFFFFFFFFFFFFFFFF - proc_info.phyif_info.last_phyif[idx].rx_bytes + rx_bytes) / 1000 * REALTIMEMONITOR_PHYIF_SECOND / difft;
	                        else
	                            calc_kbytes = (0xFFFFFFFFFFFFFFFF - proc_info.phyif_info.last_phyif[idx].rx_bytes + rx_bytes) / 1000;
							/**excluding the first flow after VPP reboot**/
							if (calc_kbytes > 12500000)/**100Gbps**/
								;
							else
							{
								pvalue->flow[idx].rcvd_byte = calc_kbytes;
							}
	                    }
	                    if (tx_bytes > proc_info.phyif_info.last_phyif[idx].tx_bytes)
	                    {
	                        if (difft)
	                            pvalue->flow[idx].sent_byte = (tx_bytes-proc_info.phyif_info.last_phyif[idx].tx_bytes) / 1000 * REALTIMEMONITOR_PHYIF_SECOND / difft;
	                        else
	                            pvalue->flow[idx].sent_byte = (tx_bytes-proc_info.phyif_info.last_phyif[idx].tx_bytes) / 1000;
	                    }
	                    else if (tx_bytes == proc_info.phyif_info.last_phyif[idx].tx_bytes)
	                    {
	                        pvalue->flow[idx].sent_byte = 0;
	                    }
	                    else
	                    {
	                        if (difft)
	                            calc_kbytes = (0xFFFFFFFFFFFFFFFF - proc_info.phyif_info.last_phyif[idx].tx_bytes + tx_bytes) / 1000 * REALTIMEMONITOR_PHYIF_SECOND / difft;
	                        else
	                            calc_kbytes = (0xFFFFFFFFFFFFFFFF - proc_info.phyif_info.last_phyif[idx].tx_bytes + tx_bytes) / 1000;
							/**excluding the first flow after VPP reboot**/
							if (calc_kbytes > 12500000)/**100Gbps**/
								;
							else
							{
								pvalue->flow[idx].sent_byte = calc_kbytes;
							}
	                    }

	                    if (g_dev_stat_dbg)
	                        ace_printf("%s %d %s difft %d recv %llu send %llu recv %llu send %llu\n",
	                            __FUNCTION__, __LINE__, name, difft, rx_bytes, tx_bytes, pvalue->flow[idx].rcvd_byte, pvalue->flow[idx].sent_byte);
	                }

					proc_info.phyif_info.last_phyif[idx].rx_bytes = rx_bytes;
	                proc_info.phyif_info.last_phyif[idx].tx_bytes = tx_bytes;
	                proc_info.phyif_info.last_phyif[idx].time = now;
                }
				else
				{						
					if (!strncmp(name, "tap", sizeof("tap")-1)
						|| !strncmp(name, "local0", sizeof("local0")-1)
						|| !strncmp(name, "host-v2k4cp", sizeof("host-v2k4cp")-1))
					{
						goto next_parse;
					}

					if (!strncmp(name, "loop", sizeof("loop")-1))
					{
						char tmp_name[32] = {0};
						snprintf(tmp_name, sizeof(tmp_name)-1,
							"%s%s", 
					#ifdef CONFIG_HYTF_FW
							"Br",
					#else
							"Bridge",
					#endif
							&name[4]
						);
						strncpy(name, tmp_name, sizeof(name)-1);
					}
				}
                name[0] = 0;
            }
next_parse:
            sscanf(buf, "%[^ ]", name);
            p = strchr(buf, '/');
            if (p)
            {
                *p = 0;
                sscanf(p - 4, "%u", &if_mtu);
            }
            rx_bytes = 0;
            tx_bytes = 0;
			if_speed = 0;
        }

        p = strstr(buf, "rx bytes");
        if (p) {
            sscanf(p, "rx bytes %llu", &rx_bytes);
            continue;
        }

        p = strstr(buf, "tx bytes");
        if (p) {
            sscanf(p, "tx bytes %llu", &tx_bytes);
            continue;
        }

        p = strstr(buf, "if state");
        if (p) {
            if (strstr(p, "on"))
            {
                strncpy(if_state, "up", sizeof(if_state));
                if_state[sizeof(if_state)-1] = 0;
            }
            else
            {
                strncpy(if_state, "down", sizeof(if_state));
                if_state[sizeof(if_state)-1] = 0;
            }
            continue;
        }

        p = strstr(buf, "if speed");
        if (p) {
            sscanf(p, "if speed %lu", &if_speed);
            continue;
        }
		
    }

    vpclose(fp);
#else
	U32 thread_index;
	U32 sw_if_index;
#if 0
	if (g_dev_stat_dbg)
		debug_save("%s %d ifmax %u",
			__FUNCTION__, __LINE__, VPP_IFS_NUM(p_vpp_ifcpus_stat));
#endif
	for (sw_if_index = 0; sw_if_index < VPP_IFS_NUM(p_vpp_ifcpus_stat); sw_if_index++)
	{
		VPP_IF_INFO_T* p_if_info = P_VPP_IF_INFO(p_vpp_ifcpus_stat, sw_if_index);
		if (!p_if_info)
		{
			continue;
		}
#if 0
		if (g_dev_stat_dbg)
			debug_save("%s %d if %u/%u name %s up %u speed %u mtu %u",
				__FUNCTION__, __LINE__, sw_if_index, VPP_IFS_NUM(p_vpp_ifcpus_stat),p_if_info->if_name,p_if_info->if_up,p_if_info->if_speed,p_if_info->if_mtu);
#endif
		if (!p_if_info->if_name[0])
		{
			continue;
		}
		strncpy(name, p_if_info->if_name, sizeof(name));
		name[sizeof(name)-1] = 0;
		if (!strncmp(name, "tap", sizeof("tap")-1)
			|| !strncmp(name, "local0", sizeof("local0")-1)
			|| !strncmp(name, "host-v2k4cp", sizeof("host-v2k4cp")-1))
		{
			continue;
		}
		snprintf(if_state, sizeof(if_state), "%s", p_if_info->if_up?"on":"down");
		if_speed = p_if_info->if_speed;
		if (!strncmp(name, "loop", sizeof("loop")-1))
		{
			char tmp_name[16] = {0};
			snprintf(tmp_name, sizeof(tmp_name),
				"%s%s", 
#ifdef CONFIG_HYTF_FW
				"Br",
#else
				"Bridge",
#endif
				&name[4]
			);
			strncpy(name, tmp_name, sizeof(name));
			name[sizeof(name)-1] = 0;
		}
		if_mtu = p_if_info->if_mtu;
		rx_bytes = 0;
		tx_bytes = 0;
		for (thread_index = 0; thread_index < VPP_CPU_NUM(p_vpp_ifcpus_stat); thread_index++)
		{
          VPP_IF_PERTHREAD_STAT_T* p_if_stat = P_VPP_IF_STAT(p_vpp_ifcpus_stat, thread_index, sw_if_index);
          if (!p_if_stat)
          {
            continue;
          }

		  rx_bytes += p_if_stat->rx_bytes;
		  tx_bytes += p_if_stat->tx_bytes;
		}
#if 0
		if (g_dev_stat_dbg)
			debug_save("%s %d if %u/%u name %s up %u speed %u mtu %u name %s rx %llu tx %llu",
				__FUNCTION__, __LINE__, sw_if_index, VPP_IFS_NUM(p_vpp_ifcpus_stat),p_if_info->if_name,p_if_info->if_up,p_if_info->if_speed,p_if_info->if_mtu,name,rx_bytes,tx_bytes);
#endif
		if (name[0])
		{
			S32 idx = pCollectorData->get_phyif_idx(name);
			if(-1 != idx)
			{
				if(proc_info.phyif_info.last_phyif[idx].time)
				{
					int difft = now - proc_info.phyif_info.last_phyif[idx].time;
					
					/**zhang.huanan uint:KBytes for 4G overflow**/
					if (rx_bytes > proc_info.phyif_info.last_phyif[idx].rx_bytes)
					{
						if (difft > 0)
							pvalue->flow[idx].rcvd_byte = (rx_bytes-proc_info.phyif_info.last_phyif[idx].rx_bytes) / 1000 * REALTIMEMONITOR_PHYIF_SECOND / difft;
						else
							pvalue->flow[idx].rcvd_byte = (rx_bytes-proc_info.phyif_info.last_phyif[idx].rx_bytes) / 1000;
					}
					else if (rx_bytes == proc_info.phyif_info.last_phyif[idx].rx_bytes)
					{
						pvalue->flow[idx].rcvd_byte = 0;
					}
					else
					{
						if (difft)
							calc_kbytes = (0xFFFFFFFFFFFFFFFF - proc_info.phyif_info.last_phyif[idx].rx_bytes + rx_bytes) / 1000 * REALTIMEMONITOR_PHYIF_SECOND / difft;
						else
							calc_kbytes = (0xFFFFFFFFFFFFFFFF - proc_info.phyif_info.last_phyif[idx].rx_bytes + rx_bytes) / 1000;
						/**excluding the first flow after VPP reboot**/
						if (calc_kbytes > 12500000)/**100Gbps**/
							;
						else
						{
							pvalue->flow[idx].rcvd_byte = calc_kbytes;
						}
					}
					if (tx_bytes > proc_info.phyif_info.last_phyif[idx].tx_bytes)
					{
						if (difft)
							pvalue->flow[idx].sent_byte = (tx_bytes-proc_info.phyif_info.last_phyif[idx].tx_bytes) / 1000 * REALTIMEMONITOR_PHYIF_SECOND / difft;
						else
							pvalue->flow[idx].sent_byte = (tx_bytes-proc_info.phyif_info.last_phyif[idx].tx_bytes) / 1000;
					}
					else if (tx_bytes == proc_info.phyif_info.last_phyif[idx].tx_bytes)
					{
						pvalue->flow[idx].sent_byte = 0;
					}
					else
					{
						if (difft)
							calc_kbytes = (0xFFFFFFFFFFFFFFFF - proc_info.phyif_info.last_phyif[idx].tx_bytes + tx_bytes) / 1000 * REALTIMEMONITOR_PHYIF_SECOND / difft;
						else
							calc_kbytes = (0xFFFFFFFFFFFFFFFF - proc_info.phyif_info.last_phyif[idx].tx_bytes + tx_bytes) / 1000;
						/**excluding the first flow after VPP reboot**/
						if (calc_kbytes > 12500000)/**100Gbps**/
							;
						else
						{
							pvalue->flow[idx].sent_byte = calc_kbytes;
						}
					}

					if (g_dev_stat_dbg)
						debug_save("%s %d %s difft %d recv %llu send %llu recv %llu send %llu\n",
							__FUNCTION__, __LINE__, name, difft, rx_bytes, tx_bytes, pvalue->flow[idx].rcvd_byte, pvalue->flow[idx].sent_byte);
				}

				proc_info.phyif_info.last_phyif[idx].rx_bytes = rx_bytes;
				proc_info.phyif_info.last_phyif[idx].tx_bytes = tx_bytes;
				proc_info.phyif_info.last_phyif[idx].time = now;
			}
		}
	}
#endif
}

void Cprocinfo::read_proc_stat(time_t now, resource_value_t * pvalue)
{
    FILE *fp;
    char* p_percent;
    char* p_tail;
    char buf[128];
    char excluding_cpu[8];
    int excluding_idx;
    int cpu_strlen;
    unsigned int total_cpus;
    unsigned long long user_j, nice_j, sys_j, idle_j, iowait_j, irq_j, softirq_j, st_j, used, total;
    unsigned long long used_sum = 0;
    unsigned long long total_sum = 0;
    float other_percentage;
    float vpp_percentage = 0.0;
    int vpp_idle_percentage;
    unsigned int polling_cpus;
	unsigned int thread_index;
	unsigned int poll_totals;
	unsigned int poll_idles;
    fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        debug_save("Cannot open /proc/stat");
        return;
    }
    total_cpus = 0;
    polling_cpus = 0;
	while (0 != fgets(buf, sizeof(buf), fp))
	{
        if (!strncmp(buf, "cpu ", 4))
        {
            continue;
        }
        if (strncmp(buf, "cpu", 3))
        {
            continue;
        }
        //excluding_idx = 0:vpp_main
        if (vpp_cpu_num > 1)
        {
            polling_cpus = vpp_cpu_num - 1;
            for (excluding_idx = 1; excluding_idx < vpp_cpu_num; ++excluding_idx)
            {
                cpu_strlen = snprintf(excluding_cpu, sizeof(excluding_cpu), "cpu%u ", vpp_cpus[excluding_idx]);
                if (!strncmp(buf, excluding_cpu, cpu_strlen))
                {
                    break;
                }
            }
        }
        else if (1 == vpp_cpu_num)
        {
            polling_cpus = vpp_cpu_num;
            for (excluding_idx = 0; excluding_idx < vpp_cpu_num; ++excluding_idx)
            {
                cpu_strlen = snprintf(excluding_cpu, sizeof(excluding_cpu), "cpu%u ", vpp_cpus[excluding_idx]);
                if (!strncmp(buf, excluding_cpu, cpu_strlen))
                {
                    break;
                }
            }
        }
        total_cpus++;
        if (vpp_cpu_num && excluding_idx < vpp_cpu_num)
        {
            continue;
        }
        sscanf(buf, "%*s %llu %llu %llu %llu %llu %llu %llu %llu", &user_j, &nice_j, &sys_j, &idle_j, &iowait_j, &irq_j, &softirq_j, &st_j);
        total = user_j+nice_j+sys_j+idle_j+iowait_j+ irq_j+softirq_j+st_j;
        used = total-idle_j;
        used_sum += used;
        total_sum += total;
	}
    fclose(fp);

#if 0
    fp = vpopen("/usr/local/vpp/bin/vppctl show dpdk cpu summary", "r");
    if (fp == NULL) {
        debug_save("Cannot open /usr/local/vpp/bin/vppctl show dpdk cpu summary errno %d", errno);
    }
    else
    {
        //Number 9 average cpu idles:7999992 totals:8000000 percentage:99%
        if (0 != fgets(buf, sizeof(buf), fp))
        {
            //idle percentage:
            p_percent = strstr(buf, "percentage:");
            if (p_percent)
            {
                p_percent += sizeof("percentage:") - 1;
                p_tail = strchr(p_percent, '%');
                if (p_tail)
                {
                    *p_tail = 0;
                    vpp_idle_percentage = atoi(p_percent);
                    if (vpp_idle_percentage <= 100)
                    {
                        vpp_percentage = 100.0 - vpp_idle_percentage;
                    }
                }
            }
        }
        vpclose(fp);
    }
#else
	poll_totals = 0;
	poll_idles = 0;
	for (thread_index = 0; thread_index < VPP_CPU_NUM(p_vpp_ifcpus_stat); thread_index++)
	{
	  VPP_CPU_PERTHREAD_STAT_T* p_cpu_stat = P_VPP_CPU_STAT(p_vpp_ifcpus_stat, thread_index);
	  if (p_cpu_stat->poll_totals[!p_cpu_stat->poll_cursor] >= 2000000)
	  {
	   poll_totals += p_cpu_stat->poll_totals[!p_cpu_stat->poll_cursor];
	   poll_idles += p_cpu_stat->poll_idles[!p_cpu_stat->poll_cursor];
	  }
	}
	vpp_idle_percentage = 100;
	if (poll_totals > 100)
	{
	  vpp_idle_percentage = poll_idles/(poll_totals/100);
	}
	if (vpp_idle_percentage <= 100)
	{
      vpp_percentage = 100.0 - vpp_idle_percentage;
	}
#endif

    //show dpdk cpu
    //Number 9 average cpu idles:7999984 totals:8000000 percentage:99%
    if(proc_info.resource_info.last_cpu.time)
    {
        other_percentage = (used_sum-proc_info.resource_info.last_cpu.used)*100/1.0/(total_sum-proc_info.resource_info.last_cpu.total);
        //vpp_cpu_num including vpp_main:
        if (total_cpus > polling_cpus && polling_cpus >= 1)
            pvalue->cpu = (other_percentage * (total_cpus - polling_cpus) + vpp_percentage * polling_cpus) / total_cpus;
        else
            pvalue->cpu = other_percentage;
        
        if (g_cpu_stat_dbg)
            ace_printf("%s %d vpp %4.4f(X%u) other %4.4f(X%u) percent %4.4f(X%u)\n",
                __FUNCTION__, __LINE__, vpp_percentage, polling_cpus, other_percentage, (total_cpus - polling_cpus), pvalue->cpu, total_cpus);
        //pvalue->cpu = (used-proc_info.resource_info.last_cpu.used)*100*5/16.0/(total-proc_info.resource_info.last_cpu.total);
    }
    proc_info.resource_info.last_cpu.time = now;
    proc_info.resource_info.last_cpu.total = total_sum;
    proc_info.resource_info.last_cpu.used = used_sum;
/*char str[100];
sprintf(str, "cpu: %3.1f, %u",pvalue->cpu,now);
debug_save(str);*/
}

void Cprocinfo::read_proc_new_session(time_t now, resource_value_t * pvalue)
{
#if 1
    if (!g_ip_conntrack_new)
        return;

    unsigned int count = *g_ip_conntrack_new;

    if (count == 0)//maybe vpp is restart
    {
        proc_info.resource_info.session_info.time = now;
        proc_info.resource_info.session_info.count = 0;
        return;
    }
    
    if (!proc_info.resource_info.session_info.time)
    {
        //system("echo 0 > /proc/sys/net/ipv4/netfilter/ip_conntrack_new");
        proc_info.resource_info.session_info.time = now;
        proc_info.resource_info.session_info.count = count;
        pvalue->new_sessions = 0;
        return;
    }
    
    if (proc_info.resource_info.session_info.count > count)
    {
        pvalue->new_sessions = (0xFFFFFFFF - proc_info.resource_info.session_info.count) + count;
    }
    else
    {
        pvalue->new_sessions = count - proc_info.resource_info.session_info.count;
    }

    if (g_new_session_dbg)
        ace_printf("%s %d count %u %u %u\n",
            __FUNCTION__, __LINE__, pvalue->new_sessions, count, proc_info.resource_info.session_info.count);
    
    proc_info.resource_info.session_info.time = now;
    proc_info.resource_info.session_info.count = count;   
#else
    unsigned int count = 0;
    unsigned int count_ipv6 = 0;

#if SPINLOCK_ALIGN_SIZE == 4
    FILE *fp = fopen("/proc/sys/net/netfilter/nf_conntrack_new", "r");
#else
    FILE *fp = fopen("/proc/sys/net/ipv4/netfilter/ip_conntrack_new", "r");
#endif
    if (fp == NULL) {
        debug_save("Cannot open /proc/sys/net/ipv4/netfilter/ip_conntrack_new");
        return;
    }
    
    fscanf(fp, "%u", &count);
    fclose(fp);

#if SPINLOCK_ALIGN_SIZE == 4
    fp = fopen("/proc/sys/net/netfilter/nf_conntrack_ipv6_new", "r");
#else
    fp = fopen("/proc/sys/net/ipv4/netfilter/ip_conntrack_ipv6_new", "r");
#endif
    if (fp == NULL) {
        debug_save("Cannot open /proc/sys/net/ipv4/netfilter/ip_conntrack_ipv6_new");
        return;
    }
    
    fscanf(fp, "%u", &count_ipv6);
    fclose(fp);
    
    if (!proc_info.resource_info.session_info.time)
    {
        //system("echo 0 > /proc/sys/net/ipv4/netfilter/ip_conntrack_new");
        proc_info.resource_info.session_info.time = now;
        proc_info.resource_info.session_info.count = count;
        proc_info.resource_info.session_info.count_ipv6 = count_ipv6;
        pvalue->new_sessions = 0;
        return;
    }
    
    if (proc_info.resource_info.session_info.count > count)
    {
        pvalue->new_sessions = (0xFFFFFFFF - proc_info.resource_info.session_info.count) + count;
    }
    else
    {
        pvalue->new_sessions = count - proc_info.resource_info.session_info.count;
    }

    if (proc_info.resource_info.session_info.count_ipv6 > count_ipv6)
    {
        pvalue->new_sessions_ipv6 = (0xFFFFFFFF - proc_info.resource_info.session_info.count_ipv6) + count_ipv6;
    }
    else
    {
        pvalue->new_sessions_ipv6 = count_ipv6 - proc_info.resource_info.session_info.count_ipv6;
    }

    if (g_new_session_dbg)
        ace_printf("%s %d count %u %u %u %u %u %u\n",
            __FUNCTION__, __LINE__, pvalue->new_sessions, count, proc_info.resource_info.session_info.count,
            pvalue->new_sessions_ipv6, count_ipv6, proc_info.resource_info.session_info.count_ipv6);
    
    proc_info.resource_info.session_info.time = now;
    proc_info.resource_info.session_info.count = count;
    proc_info.resource_info.session_info.count_ipv6 = count_ipv6;
#endif
}

void Cprocinfo::read_proc_meminfo(resource_value_t * pvalue)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        debug_save("Cannot open /proc/meminfo");
        return;
    }
    char buf[80];
    unsigned long total = 0, used = 0, mfree = 0, shared = 0, buffers = 0, cached = 0, available = 0;

    fscanf(fp, "MemTotal: %lu %s\n", &total, buf);
    fscanf(fp, "MemFree: %lu %s\n", &mfree, buf);
	fscanf(fp, "MemAvailable: %lu %s\n", &available, buf);
    fscanf(fp, "Buffers: %lu %s\n", &buffers, buf);
    fscanf(fp, "Cached: %lu %s\n", &cached, buf);

    fclose(fp);
    used = total - mfree - buffers - cached;
    pvalue->memory = used*100/1.0/total;

#if 0
    if (mfree < cached)
    {
        if (!last_drop_cache_time || difftime(time(NULL), last_drop_cache_time) > 600)
        {
            system("sync && echo 3 > /proc/sys/vm/drop_caches && echo 2 > /proc/sys/vm/drop_caches");
            last_drop_cache_time = time(NULL);
        }
    }
#else
	if (!last_drop_cache_time || difftime(time(NULL), last_drop_cache_time) > 600)
	{
		fp = fopen("/usr/mysys/cached2free_policy", "r");
		if (fp == NULL) {
			if (mfree < cached)
			{
				debug_save("%s %d : exec sync && echo 3 > /proc/sys/vm/drop_caches && echo 2 > /proc/sys/vm/drop_caches", __FUNCTION__,__LINE__);
				hylab_system("sync && echo 3 > /proc/sys/vm/drop_caches && echo 2 > /proc/sys/vm/drop_caches");
			}
		}
		else
		{
			char cache_cmd[128];
			unsigned long read_long;
			unsigned long drop_caches = 0, swappiness = 60, vfs_cache_pressure = 100, min_free_kbytes = total/50;
			fscanf(fp, "drop_caches: %lu\n", &drop_caches);
			fscanf(fp, "swappiness: %lu\n", &swappiness);
			fscanf(fp, "vfs_cache_pressure: %lu\n", &vfs_cache_pressure);		
			fscanf(fp, "min_free_kbytes: %lu\n", &min_free_kbytes);
			fclose(fp);
			if (drop_caches && mfree < cached)
			{
				snprintf(cache_cmd, sizeof(cache_cmd), "sync && sleep 1 && echo %lu > /proc/sys/vm/drop_caches", drop_caches);
				debug_save("%s %d : exec %s", __FUNCTION__,__LINE__,cache_cmd);
				hylab_system(cache_cmd);
			}
			if (60 != swappiness)
			{
				if (mfree < cached)
				{
					fp = fopen("/proc/sys/vm/swappiness", "r");
					if (fp)
					{
						fscanf(fp, "%lu\n", &read_long);
						fclose(fp);
						if (read_long != swappiness)
						{
							snprintf(cache_cmd, sizeof(cache_cmd), "echo %lu > /proc/sys/vm/swappiness", swappiness);
							debug_save("%s %d : exec %s", __FUNCTION__,__LINE__,cache_cmd);
							hylab_system(cache_cmd);
						}
					}
				}
			}

			if (vfs_cache_pressure)
			{
				if (mfree < cached)
				{
					vfs_cache_pressure += mfree ? ((cached*100)/mfree) : 500;
				}
				if (vfs_cache_pressure > 600)
				{
					vfs_cache_pressure = 600;
				}
				fp = fopen("/proc/sys/vm/vfs_cache_pressure", "r");
				if (fp)
				{
					fscanf(fp, "%lu\n", &read_long);
					fclose(fp);
					if (read_long != vfs_cache_pressure)
					{
						snprintf(cache_cmd, sizeof(cache_cmd), "echo %lu > /proc/sys/vm/vfs_cache_pressure", vfs_cache_pressure);
						debug_save("%s %d : exec %s", __FUNCTION__,__LINE__,cache_cmd);
						hylab_system(cache_cmd);
					}
				}
			}

			if (min_free_kbytes)
			{
				if (mfree < cached)
				{
					fp = fopen("/proc/sys/vm/min_free_kbytes", "r");
					if (fp)
					{
						fscanf(fp, "%lu\n", &read_long);
						fclose(fp);
						if (read_long != min_free_kbytes)
						{
							snprintf(cache_cmd, sizeof(cache_cmd), "echo %lu > /proc/sys/vm/min_free_kbytes", min_free_kbytes);
							debug_save("%s %d : exec %s", __FUNCTION__,__LINE__,cache_cmd);
							hylab_system(cache_cmd);
						}
					}
				}
			}
		}
		last_drop_cache_time = time(NULL);
	}
#endif
}
extern unsigned int g_current_connection_num;
extern unsigned int g_current_connection_ipv6_num;
extern unsigned int g_license_connection_num;
extern unsigned int g_license_online_user_num;
void Cprocinfo::read_proc_sys_net_ipv4_netfilter_ip_conntrack_count(resource_value_t * pvalue)
{
    pvalue->sessions = g_current_connection_num;
}
void Cprocinfo::read_proc_sys_net_ipv6_netfilter_ip_conntrack_count(resource_value_t * pvalue)
{
    pvalue->sessions_ipv6 = g_current_connection_ipv6_num;
}
void Cprocinfo::read_proc_sys_net_ipv4_netfilter_ip_conntrack_max(resource_value_t * pvalue)
{
    pvalue->sessions_max = g_connection_num * HYTF_CONNECTION_HASH_NEW_BUCKET;
}

#if 1
static unsigned int hylab_session_number_parse(char* session_string)
{
    unsigned int session_number;
    unsigned int unit_value;
    char* p_tmp;
    int this_value;
    int str_len = strlen(session_string);
    if (str_len < 2)
    {
        return 0;
    }
    str_len--;
    if ('K' == session_string[str_len] || 'k' == session_string[str_len])
    {
        unit_value = 1000;
    }
    else if ('M' == session_string[str_len] || 'm' == session_string[str_len])
    {
        unit_value = 1000000;
    }
    else
    {
        return 0;
    }
    session_string[str_len] = 0;
    session_number = 0;
    p_tmp = strchr(session_string, '.');
    if (p_tmp)
    {
        *p_tmp = 0;
        ++p_tmp;
        if (1 != strlen(p_tmp))
        {
            return 0;
        }
        this_value = atoi(p_tmp);
        if (this_value < 0)
        {
            return 0;
        }
        session_number += (this_value*unit_value)/10;
    }
    this_value = atoi(session_string);

    if (this_value <= 0)
    {
        return 0;
    }
    session_number += (this_value*unit_value);
    return session_number;
}

static int license_nac_is_version2()
{
        static int license_version = -1;
        if (-1 != license_version)
        {
                return license_version;
        }
        if (!access(HYLAB_NAC_LICENSE_V2_FILE, F_OK))
        {
                license_version = 1;
        }
        else
        {
                license_version = 0;
        }
        return license_version;
}

#endif

#define READ_LICENSE_NUMBER_TIME 60
unsigned int Cprocinfo::read_license_ctrl_number(unsigned int kernel_session)
{
    static int config_session = 0;
	static time_t last_call_time = 0;
#if 0
    static char* session_string[] =
    {
        "32M",
        "64M",
        "64K",
        "128K",
        "256K",
        "512K",
        "700K",
        "1M",
        "1.5M",
        "2M",
        "3M",
        "4M",
        "8M",
        "16M",
    };
    static unsigned int session_number[] =
    {
        32000 * 1000,
        64000 * 1000,
        64 * 1000,
        128 * 1000,
        256 * 1000,
        512 * 1000,
        700 * 1000,
        1000 * 1000,
        1500 * 1000,
        2000 * 1000,
        3000 * 1000,
        4000 * 1000,
        8000 * 1000,
        16000 * 1000,
    };
#endif
    FILE *fp;
    char buf[128];
    int session_index;
    int online_user_num;
    char* p_head, *p_tail;

    // Get the current time
    time_t current_time = time(NULL);
	
    // If the last call time is not zero and the difference between the current time
    // and the last call time is less than 5 minutes
    if (last_call_time != 0 && (current_time - last_call_time) < READ_LICENSE_NUMBER_TIME) {
		if (config_session)
		{
			return config_session;
		}
    }

    // Update the last call time
    last_call_time = current_time;

    #if 0
    if (config_session)
    {
        return config_session;
    }
    #endif

#if defined(CONFIG_HYTF_FW) || defined(ARCH_ARM64) || defined(aiTai) || defined(Hillstone)
    fp = fopen("/usr/.lic/.system", "r");
#else
    fp = fopen(license_nac_is_version2()?"/usr/.lic/.system":"/usr/.lic/.ace_fsec.dat", "r");
#endif
    if (!fp)
    {
        if (!config_session)
        {
            return kernel_session;
        }
        return config_session;
    }
    online_user_num = 0;
    memset(buf, 0x00, sizeof(buf));
    while( 0 != fgets( buf, sizeof(buf), fp ))
    {
        if (strcasestr(buf, "Session") && strchr(buf, '='))
        {
#if 0
            for (session_index = 0; session_index < sizeof(session_number)/sizeof(session_number[0]); ++session_index)
            {
                if (strcasestr(buf + sizeof("Session=") - 1, session_string[session_index]))
                {
                    config_session = session_number[session_index];
                    break;
                }
            }
#else
            p_head = strchr(buf, '"');
            p_tail = NULL;
            if (p_head){
                ++p_head;
                p_tail = strchr(p_head, '"');
            }
            if (p_tail){
                *p_tail = 0;
                config_session = hylab_session_number_parse(p_head);
            }
#endif
        }
        else if (strcasestr(buf, "Online User Number") && strchr(buf, '='))
        {
            p_head = strchr(buf, '"');
            p_tail = NULL;
            if (p_head){
                ++p_head;
                p_tail = strchr(p_head, '"');
            }
            if (p_tail){
                *p_tail = 0;
                online_user_num = atoi(p_head);
            }

        }
        memset(buf, 0x00, sizeof(buf));
    }
    fclose(fp);
    if (!config_session/* || config_session > kernel_session*/)
    {
        config_session = kernel_session;
    }
    g_license_connection_num = config_session;
    g_license_online_user_num = online_user_num;

    return config_session;
}


int hytf_cmd_exchange(unsigned short cmd_dport, long reply_type,
	void* send_buf, int send_len, void* recv_buf, int recv_len, unsigned int reply_timeout_ms)
{
	fd_set read_fset;
	socklen_t peer_len;
	struct sockaddr_in server_addr;
	int pkt_len;
	int cmd_result = 0;
	int sock_fd = -1;
	struct timeval tv = {(reply_timeout_ms/1000), (reply_timeout_ms%1000)*1000};
	/*创建socket*/  
	if((sock_fd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{  
		debug_save("%s %s %d --- socket dport %d reply_type %d cmd_result %d send_len %d errno %d\r\n", __FUNCTION__,__FILE__,__LINE__,cmd_dport,reply_type,sock_fd,send_len,errno);
		cmd_result = -3;
		goto error_out;
	}
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(cmd_dport);
	server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	pkt_len = sendto(sock_fd, send_buf, send_len, 0, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in));
	if (0 > pkt_len)
	{
		cmd_result = -4;
		debug_save("%s %s %d --- sendto dport %d reply_type %d cmd_result %d send_len %d errno %d\r\n", __FUNCTION__,__FILE__,__LINE__,cmd_dport,reply_type,pkt_len,send_len,errno);
		goto error_out;
	}
//	ace_printf("%s %s %d --- sendto cmd_result %d\r\n", __FUNCTION__,__FILE__,__LINE__,pkt_len);

	if (NULL == recv_buf)
	{
		goto error_out;
	}

	FD_ZERO(&read_fset);
	FD_SET (sock_fd, &read_fset);

	/*接收响应信息*/ 
	if(0 < select((sock_fd + 1), &read_fset, NULL, NULL, &tv))
	{
		peer_len = sizeof(struct sockaddr_in);
		pkt_len = recvfrom(sock_fd, recv_buf, recv_len, 0, (struct sockaddr*)&server_addr, &peer_len);
		if (0 > pkt_len)
		{
			cmd_result = -2;
			debug_save("%s %s %d --- recvfrom dport %d reply_type %d cmd_result %d send_len %d errno %d\r\n", __FUNCTION__,__FILE__,__LINE__,cmd_dport,reply_type,pkt_len,send_len,errno);
		}
//		ace_printf("%s %s %d --- recvfrom cmd_result %d\r\n", __FUNCTION__,__FILE__,__LINE__,pkt_len);
	}
	else
	{
		cmd_result = -1;
		debug_save("%s %s %d --- reply_timeout %u dport %d reply_type %d cmd_result %d send_len %d errno %d\r\n", __FUNCTION__,__FILE__,__LINE__,reply_timeout_ms,cmd_dport,reply_type,pkt_len,send_len,errno);
	}

error_out:
	if (sock_fd >= 0)
	{
		close (sock_fd);
	}
	return cmd_result;
}

int hylab_cmd_unix_exchange(const char* cmd_path, long reply_type,
    void* send_buf, int send_len, void* recv_buf, int recv_len, unsigned int reply_timeout_ms)
{
    fd_set read_fset;
    socklen_t peer_len;
    struct sockaddr_un server_addr;
	int size;
	int len;
    int pkt_len;
	char my_path[sizeof(server_addr.sun_path)];
    int cmd_result = 0;
    int sock_fd = -1;
    struct timeval tv = {(reply_timeout_ms/1000), (reply_timeout_ms%1000)*1000};

	my_path[0] = 0;
    if((sock_fd = socket(AF_UNIX,SOCK_DGRAM,0)) < 0)
    {  
        debug_save("%s %s %d --- failed to socket errno %d cmd_path %s\r\n", __FUNCTION__,__FILE__,__LINE__,errno,cmd_path);
        cmd_result = -1;
        goto error_out;
    }

#if 0
	size = send_len;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	if (ret < 0) {
		debug_save ("%s %s %d --- setsockopt rcvbuf %d(%s) failed", __FUNCTION__,__FILE__,__LINE__,errno,strerror(errno));
        cmd_result = -3;
        goto error_out;
	}
	
	ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
	if (ret < 0) {
		debug_save ("%s %s %d --- setsockopt rcvbuf %d(%s) failed", __FUNCTION__,__FILE__,__LINE__,errno,strerror(errno));
        cmd_result = -3;
        goto error_out;
	}
#endif

	memset(&server_addr, 0, sizeof(struct sockaddr_un));
	server_addr.sun_family = AF_UNIX;
	
	if (recv_buf && recv_len)
	{
		size = snprintf(my_path, sizeof(my_path), "/tmp/.unix_p%u_t%u", getpid(),pthread_self());
		unlink(my_path);
		strncpy(server_addr.sun_path, my_path, sizeof(server_addr.sun_path));
		server_addr.sun_path[sizeof(server_addr.sun_path)-1] = 0;
		len = offsetof (struct sockaddr_un, sun_path) + size;

		if (bind (sock_fd, (struct sockaddr *)&server_addr, len) < 0) {
			debug_save ("%s %s %d --- bind rcvbuf %d(%s) failed", __FUNCTION__,__FILE__,__LINE__,errno,strerror(errno));
	        cmd_result = -2;
	        goto error_out;
		}
	}

	size = snprintf(server_addr.sun_path, sizeof(server_addr.sun_path), "%s", cmd_path);
	len = offsetof (struct sockaddr_un, sun_path) + size;

    pkt_len = sendto(sock_fd, send_buf, send_len, 0, (struct sockaddr*)&server_addr, len);
    if (0 > pkt_len)
    {
        cmd_result = -3;
        debug_save("%s %s %d --- sendto cmd_result %d errno %d cmd_path %s\r\n", __FUNCTION__,__FILE__,__LINE__,pkt_len,errno,cmd_path);
        goto error_out;
    }
/*	debug_save("%s %s %d --- sendto cmd_result %d\r\n", __FUNCTION__,__FILE__,__LINE__,pkt_len);*/

    if (recv_len && recv_buf)
    {
        FD_ZERO(&read_fset);
        FD_SET (sock_fd, &read_fset);

        if(0 < select((sock_fd + 1), &read_fset, NULL, NULL, &tv))
        {
            peer_len = sizeof(struct sockaddr_un);
			server_addr.sun_path[0] = 0;
            pkt_len = recvfrom(sock_fd, recv_buf, recv_len, 0, (struct sockaddr*)&server_addr, &peer_len);
            if (0 > pkt_len)
            {
                cmd_result = -4;
            }
/*          	debug_save("%s %s %d --- recvfrom cmd_result %d from %s\r\n", __FUNCTION__,__FILE__,__LINE__,pkt_len,server_addr.sun_path);*/
        }
        else
        {
            cmd_result = -1;
            debug_save("%s %s %d --- reply_timeout errno %d cmd_path %s\r\n", __FUNCTION__,__FILE__,__LINE__,errno,cmd_path);
        }
    }

error_out:
    if (sock_fd >= 0)
    {
        close (sock_fd);
    }
	if (my_path[0])
	{
		unlink(my_path);
	}
    return cmd_result;
}

void nl_header_init (struct nlmsghdr* p_h, int size, int type, int local_pid)
{
    static int seq_num = 1;
    
    memset (p_h, 0, sizeof(struct nlmsghdr));
    p_h->nlmsg_len = NLMSG_LENGTH(size);
    p_h->nlmsg_type = type;
    p_h->nlmsg_pid = local_pid;
    p_h->nlmsg_seq = seq_num++;
}

int ace_kernel_session_accelerate_aging_config(int nl_fd, int cmd_id, NL_SESSION_ACCELERATE_AGING_SWAP* cfg_data, int local_id)
{
    NL_SESSION_ACCELERATE_AGING_SWAP msg;
    struct _nl_ack_message nl_result;
    memset(&msg, 0, sizeof(msg));
    memset(&nl_result, 0, sizeof(nl_result));
/*	debug_save("%s %s %d --- tcp_timeout %u udp_timeout %u tcp_syn_timeout %u udp_unreplied_timeout %u",__FUNCTION__,__FILE__,__LINE__,
		cfg_data->tcp_timeout, cfg_data->udp_timeout, cfg_data->tcp_syn_timeout, cfg_data->udp_unreplied_timeout);
*/
    memcpy(&msg, cfg_data, sizeof(NL_SESSION_ACCELERATE_AGING_SWAP));
/*
	debug_save("%s %s %d --- tcp_timeout %u udp_timeout %u tcp_syn_timeout %u udp_unreplied_timeout %u",__FUNCTION__,__FILE__,__LINE__,
		msg.tcp_timeout, msg.udp_timeout, msg.tcp_syn_timeout, msg.udp_unreplied_timeout);
*/
    nl_header_init (&msg.hdr, sizeof(NL_SESSION_ACCELERATE_AGING_SWAP) - sizeof(struct nlmsghdr), cmd_id, 0);
    
	if (0 == hylab_cmd_unix_exchange(UNIX_SOCK_PATH_FAKE_KERNEL, cmd_id, (char*)&msg, msg.hdr.nlmsg_len, 
        (char*)&nl_result, sizeof(struct _nl_ack_message), 1500))
    {
        return 0;
    }    

    return (-1);
}

void hylab_wait_vpp_receiver_ready(void)
{
    NL_SESSION_ACCELERATE_AGING_SWAP msg;
    struct _nl_ack_message nl_result;
    while(1)
    {
        if (access("/run/vpp/.vpp_init_finish", 0) == 0)
            break;

        ace_printf("Collector is waitting for vpp");
        sleep(1);
    }

    memset(&msg, 0, sizeof(msg));
    memset(&nl_result, 0, sizeof(nl_result));
    nl_header_init (&msg.hdr, sizeof(NL_SESSION_ACCELERATE_AGING_SWAP) - sizeof(struct nlmsghdr), HYTF_CHECK_VPP_READY, 0);
    while(0 != hylab_cmd_unix_exchange(UNIX_SOCK_PATH_FAKE_KERNEL, HYTF_CHECK_VPP_READY, (char*)&msg, msg.hdr.nlmsg_len, 
        (char*)&nl_result, sizeof(struct _nl_ack_message), 5000))
    {
        sleep(1);
    }
}

void Cprocinfo::proc_session_accelerate_aging(int level)
{
    unsigned int timeout_value = 1;
    unsigned int accelerate_multiple = 1;
    int sync = 0;
    NL_SESSION_ACCELERATE_AGING_SWAP msg = { 0 };

	#define PROC_PRE_FIX "/proc/sys/net/netfilter/nf"
    
    if(!g_reporter_info)
    {
        if(0 != reporter_shm_get ())
            return;
        if(!g_reporter_info)
            return;
    }
    
    if(0 != reporter_shm_acquire())
        return;

    accelerate_multiple = g_reporter_info->session_aging_conf.accelerate_multiple;
    if (!accelerate_multiple)
        accelerate_multiple = 1;
    if(0 == g_reporter_info->session_aging_conf.have_setted)
    {
        if(level >= g_reporter_info->session_aging_conf.high_level
            && 0 == g_reporter_info->session_aging_conf.level_flag)
        {
            if (accelerate_multiple > 1)
            {
                timeout_value = (g_reporter_info->session_aging_conf.tcp_timeout/accelerate_multiple);
                if (!timeout_value)
                    timeout_value = 1;
                msg.tcp_timeout = timeout_value;

                timeout_value = (g_reporter_info->session_aging_conf.udp_timeout/accelerate_multiple);
                if (!timeout_value)
                    timeout_value = 1;
                msg.udp_timeout = timeout_value;

#if 1
                timeout_value = (g_reporter_info->session_aging_conf.icmp_timeout/accelerate_multiple);
                if (!timeout_value)
                    timeout_value = 1;
                msg.icmp_timeout = timeout_value;

                timeout_value = (g_reporter_info->session_aging_conf.other_timeout/accelerate_multiple);
                if (!timeout_value)
                    timeout_value = 1;
                msg.other_timeout = timeout_value;
#endif

                timeout_value = (g_reporter_info->session_aging_conf.tcp_syn_timeout/accelerate_multiple);
                if (!timeout_value)
                    timeout_value = 1;
                msg.tcp_syn_timeout = timeout_value;

                timeout_value = (g_reporter_info->session_aging_conf.udp_unreplied_timeout/accelerate_multiple);
                if (!timeout_value)
                    timeout_value = 1;
                msg.udp_unreplied_timeout = timeout_value;

/*				debug_save("%s %s %d --- have_setted %d level_flag %d accelerate_multiple %d high_level %d low_level %d"
					" tcp_timeout %u udp_timeout %u icmp_timeout %u other_timeout %u tcp_syn_timeout %u udp_unreplied_timeout %u",__FUNCTION__,__FILE__,__LINE__,
					g_reporter_info->session_aging_conf.have_setted,
					g_reporter_info->session_aging_conf.level_flag,
					g_reporter_info->session_aging_conf.accelerate_multiple,
					g_reporter_info->session_aging_conf.high_level,
					g_reporter_info->session_aging_conf.low_level,
					g_reporter_info->session_aging_conf.tcp_timeout,
					g_reporter_info->session_aging_conf.udp_timeout,
					g_reporter_info->session_aging_conf.icmp_timeout,
					g_reporter_info->session_aging_conf.other_timeout,
					g_reporter_info->session_aging_conf.tcp_syn_timeout,
					g_reporter_info->session_aging_conf.udp_unreplied_timeout);

				debug_save("%s %s %d --- level %u accelerate_multiple %u tcp_timeout %u udp_timeout %u tcp_syn_timeout %u udp_unreplied_timeout %u",__FUNCTION__,__FILE__,__LINE__,
					level, accelerate_multiple, msg.tcp_timeout, msg.udp_timeout, msg.tcp_syn_timeout, msg.udp_unreplied_timeout);*/
                sync = 1;
            }
            g_reporter_info->session_aging_conf.level_flag = 1;
        }
        else if(level <= g_reporter_info->session_aging_conf.low_level
            && 1 == g_reporter_info->session_aging_conf.level_flag)
        {
            timeout_value = (g_reporter_info->session_aging_conf.tcp_timeout);
            if (!timeout_value)
                timeout_value = 1;
            msg.tcp_timeout = timeout_value;

            timeout_value = (g_reporter_info->session_aging_conf.udp_timeout);
            if (!timeout_value)
                timeout_value = 1;
            msg.udp_timeout = timeout_value;

#if 1
            timeout_value = (g_reporter_info->session_aging_conf.icmp_timeout);
            if (!timeout_value)
                timeout_value = 1;
            msg.icmp_timeout = timeout_value;

            timeout_value = (g_reporter_info->session_aging_conf.other_timeout);
            if (!timeout_value)
                timeout_value = 1;
            msg.other_timeout = timeout_value;
#endif

            timeout_value = (g_reporter_info->session_aging_conf.tcp_syn_timeout);
            if (!timeout_value)
                timeout_value = 1;
            msg.tcp_syn_timeout = timeout_value;
            
            timeout_value = (g_reporter_info->session_aging_conf.udp_unreplied_timeout);
            if (!timeout_value)
                timeout_value = 1;
            msg.udp_unreplied_timeout = timeout_value;
            sync = 1;
/*			debug_save("%s %s %d --- have_setted %d level_flag %d accelerate_multiple %d high_level %d low_level %d"
				" tcp_timeout %u udp_timeout %u icmp_timeout %u other_timeout %u tcp_syn_timeout %u udp_unreplied_timeout %u",__FUNCTION__,__FILE__,__LINE__,
				g_reporter_info->session_aging_conf.have_setted,
				g_reporter_info->session_aging_conf.level_flag,
				g_reporter_info->session_aging_conf.accelerate_multiple,
				g_reporter_info->session_aging_conf.high_level,
				g_reporter_info->session_aging_conf.low_level,
				g_reporter_info->session_aging_conf.tcp_timeout,
				g_reporter_info->session_aging_conf.udp_timeout,
				g_reporter_info->session_aging_conf.icmp_timeout,
				g_reporter_info->session_aging_conf.other_timeout,
				g_reporter_info->session_aging_conf.tcp_syn_timeout,
				g_reporter_info->session_aging_conf.udp_unreplied_timeout);
			
			debug_save("%s %s %d --- level %u accelerate_multiple %u tcp_timeout %u udp_timeout %u tcp_syn_timeout %u udp_unreplied_timeout %u",__FUNCTION__,__FILE__,__LINE__,
				level, accelerate_multiple, msg.tcp_timeout, msg.udp_timeout, msg.tcp_syn_timeout, msg.udp_unreplied_timeout);
*/
            g_reporter_info->session_aging_conf.level_flag = 0;
        }
    }
    else
    {
        if(level >= g_reporter_info->session_aging_conf.high_level)
        {
            if (accelerate_multiple > 1)
            {
                timeout_value = (g_reporter_info->session_aging_conf.tcp_timeout/accelerate_multiple);
                if (!timeout_value)
                    timeout_value = 1;
                msg.tcp_timeout = timeout_value;

                timeout_value = (g_reporter_info->session_aging_conf.udp_timeout/accelerate_multiple);
                if (!timeout_value)
                    timeout_value = 1;
                msg.udp_timeout = timeout_value;

#if 1
                timeout_value = (g_reporter_info->session_aging_conf.icmp_timeout/accelerate_multiple);
                if (!timeout_value)
                    timeout_value = 1;
                msg.icmp_timeout = timeout_value;

                timeout_value = (g_reporter_info->session_aging_conf.other_timeout/accelerate_multiple);
                if (!timeout_value)
                    timeout_value = 1;
                msg.other_timeout = timeout_value;
#endif

                timeout_value = (g_reporter_info->session_aging_conf.tcp_syn_timeout/accelerate_multiple);
                if (!timeout_value)
                    timeout_value = 1;
                msg.tcp_syn_timeout = timeout_value;

                timeout_value = (g_reporter_info->session_aging_conf.udp_unreplied_timeout/accelerate_multiple);
                if (!timeout_value)
                    timeout_value = 1;
                msg.udp_unreplied_timeout = timeout_value;
/*				debug_save("%s %s %d --- have_setted %d level_flag %d accelerate_multiple %d high_level %d low_level %d"
					" tcp_timeout %u udp_timeout %u icmp_timeout %u other_timeout %u tcp_syn_timeout %u udp_unreplied_timeout %u",__FUNCTION__,__FILE__,__LINE__,
					g_reporter_info->session_aging_conf.have_setted,
					g_reporter_info->session_aging_conf.level_flag,
					g_reporter_info->session_aging_conf.accelerate_multiple,
					g_reporter_info->session_aging_conf.high_level,
					g_reporter_info->session_aging_conf.low_level,
					g_reporter_info->session_aging_conf.tcp_timeout,
					g_reporter_info->session_aging_conf.udp_timeout,
					g_reporter_info->session_aging_conf.icmp_timeout,
					g_reporter_info->session_aging_conf.other_timeout,
					g_reporter_info->session_aging_conf.tcp_syn_timeout,
					g_reporter_info->session_aging_conf.udp_unreplied_timeout);

				debug_save("%s %s %d --- level %u accelerate_multiple %u tcp_timeout %u udp_timeout %u tcp_syn_timeout %u udp_unreplied_timeout %u",__FUNCTION__,__FILE__,__LINE__,
					level, accelerate_multiple, msg.tcp_timeout, msg.udp_timeout, msg.tcp_syn_timeout, msg.udp_unreplied_timeout);
*/
                sync = 1;
            }

            g_reporter_info->session_aging_conf.level_flag = 1;
        }
        else if(level <= g_reporter_info->session_aging_conf.low_level)
        {
            timeout_value = (g_reporter_info->session_aging_conf.tcp_timeout);
            if (!timeout_value)
                timeout_value = 1;
            msg.tcp_timeout = timeout_value;
            
            timeout_value = (g_reporter_info->session_aging_conf.udp_timeout);
            if (!timeout_value)
                timeout_value = 1;
            msg.udp_timeout = timeout_value;

#if 1
            timeout_value = (g_reporter_info->session_aging_conf.icmp_timeout);
            if (!timeout_value)
                timeout_value = 1;
            msg.icmp_timeout = timeout_value;

            timeout_value = (g_reporter_info->session_aging_conf.other_timeout);
            if (!timeout_value)
                timeout_value = 1;
            msg.other_timeout = timeout_value;
#endif

            timeout_value = (g_reporter_info->session_aging_conf.tcp_syn_timeout);
            if (!timeout_value)
                timeout_value = 1;
            msg.tcp_syn_timeout = timeout_value;
            
            timeout_value = (g_reporter_info->session_aging_conf.udp_unreplied_timeout);
            if (!timeout_value)
                timeout_value = 1;
            msg.udp_unreplied_timeout = timeout_value;
/*			debug_save("%s %s %d --- have_setted %d level_flag %d accelerate_multiple %d high_level %d low_level %d"
				" tcp_timeout %u udp_timeout %u icmp_timeout %u other_timeout %u tcp_syn_timeout %u udp_unreplied_timeout %u",__FUNCTION__,__FILE__,__LINE__,
				g_reporter_info->session_aging_conf.have_setted,
				g_reporter_info->session_aging_conf.level_flag,
				g_reporter_info->session_aging_conf.accelerate_multiple,
				g_reporter_info->session_aging_conf.high_level,
				g_reporter_info->session_aging_conf.low_level,
				g_reporter_info->session_aging_conf.tcp_timeout,
				g_reporter_info->session_aging_conf.udp_timeout,
				g_reporter_info->session_aging_conf.icmp_timeout,
				g_reporter_info->session_aging_conf.other_timeout,
				g_reporter_info->session_aging_conf.tcp_syn_timeout,
				g_reporter_info->session_aging_conf.udp_unreplied_timeout);
			
			debug_save("%s %s %d --- level %u accelerate_multiple %u tcp_timeout %u udp_timeout %u tcp_syn_timeout %u udp_unreplied_timeout %u",__FUNCTION__,__FILE__,__LINE__,
				level, accelerate_multiple, msg.tcp_timeout, msg.udp_timeout, msg.tcp_syn_timeout, msg.udp_unreplied_timeout);
*/
            sync = 1;
            g_reporter_info->session_aging_conf.level_flag = 0;
        }
        g_reporter_info->session_aging_conf.have_setted = 0;
    }
    reporter_shm_release();
    if (sync == 1)
        ace_kernel_session_accelerate_aging_config(0, ACEFW_SET_SESSION_ACCELERATE_AGING, &msg, 0);

}
}
