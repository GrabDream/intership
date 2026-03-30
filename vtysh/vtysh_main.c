// SPDX-License-Identifier: GPL-2.0-or-later
/* Virtual terminal interface shell.
 * Copyright (C) 2000 Kunihiro Ishiguro
 */

#include <zebra.h>

#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <setjmp.h>
#include <pwd.h>
#include <sys/file.h>
#include <unistd.h>
#include <linux/netlink.h>

/* readline carries some ancient definitions around */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#include <readline/readline.h>
#include <readline/history.h>
#pragma GCC diagnostic pop

/*
 * The append_history function only appears in newer versions
 * of the readline library it appears like.  Since we don't
 * need this just silently ignore the code on these
 * ancient platforms.
 */
#if !defined HAVE_APPEND_HISTORY
#define append_history(A, B)
#endif

#include <lib/version.h>
#include "getopt.h"
#include "command.h"
#include "memory.h"
#include "linklist.h"
#include "libfrr.h"
#include "ferr.h"
#include "lib_errors.h"

#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"

extern int do_termios_oldt;

/* VTY shell program name. */
char *progname;

/* SUID mode */
static uid_t elevuid, realuid;
static gid_t elevgid, realgid;

#define VTYSH_CONFIG_NAME "vtysh.conf"
#define FRR_CONFIG_NAME "frr.conf"

/* Configuration file name and directory. */
static char vtysh_config[MAXPATHLEN * 3];
char frr_config[MAXPATHLEN * 3];
char vtydir[MAXPATHLEN];
static char history_file[MAXPATHLEN];

/* Flag for indicate executing child command. */
int execute_flag = 0;

/* Flag to indicate if in user/unprivileged mode. */
int user_mode;

/* Master of threads. */
struct event_loop *master;

/* Command logging */
FILE *logfile;

static void vtysh_rl_callback(char *line_read)
{
	HIST_ENTRY *last;

	rl_callback_handler_remove();

	if (!line_read) {
		//vtysh_loop_exited = true;		
		vty_out(vty, "\n");
		goto next;
	}

	/* If the line has any text in it, save it on the history. But only if
	 * last command in history isn't the same one.
	 */
	if (*line_read) {
		using_history();
		last = previous_history();
		if (!last || strcmp(last->line, line_read) != 0) {
			add_history(line_read);
			append_history(1, history_file);
		}
	}

	vtysh_execute(line_read);

next:
	
	if (!vtysh_loop_exited)
		rl_callback_handler_install(vtysh_prompt(), vtysh_rl_callback);

	if(line_read)
		free(line_read);
}

/* SIGTSTP handler.  This function care user's ^Z input. */
static void sigtstp(int sig)
{
	if(do_termios_oldt){	
		write(STDOUT_FILENO, "\r\n", strlen("\r\n"));
		return;
	}
	
	rl_callback_handler_remove();

	/* Execute "end" command. */
	vtysh_execute("end");

	if (!vtysh_loop_exited)
		rl_callback_handler_install(vtysh_prompt(), vtysh_rl_callback);

	/* Initialize readline. */
	rl_initialize();
	printf("\n");
	rl_forced_update_display();
}

/* SIGINT handler.  This function care user's ^Z input.  */
static void sigint(int sig)
{
	if(do_termios_oldt){
		write(STDOUT_FILENO, "\r\n", strlen("\r\n"));
		return;
	}

	/* Check this process is not child process. */
	if (!execute_flag) {
		rl_initialize();
		printf("\n");
		rl_forced_update_display();
	}
}

/* Signale wrapper for vtysh. We don't use sigevent because
 * vtysh doesn't use threads. TODO */
static void vtysh_signal_set(int signo, void (*func)(int))
{
	struct sigaction sig;
	struct sigaction osig;

	sig.sa_handler = func;
	sigemptyset(&sig.sa_mask);
	sig.sa_flags = 0;
#ifdef SA_RESTART
	sig.sa_flags |= SA_RESTART;
#endif /* SA_RESTART */

	sigaction(signo, &sig, &osig);
}

/* Initialization of signal handles. */
static void vtysh_signal_init(void)
{
	vtysh_signal_set(SIGINT, sigint);
	vtysh_signal_set(SIGTSTP, sigtstp);
	vtysh_signal_set(SIGPIPE, SIG_IGN);
}

/* Help information display. */
static void usage(int status)
{
	if (status != 0)
		fprintf(stderr, "Try `%s --help' for more information.\n",
			progname);
	else
		printf("Usage : %s [OPTION...]\n\n"
		       "Integrated shell for FRR (version " FRR_VERSION
		       "). \n"
		       "Configured with:\n    " FRR_CONFIG_ARGS
		       "\n\n"
		       "-b, --boot               Execute boot startup configuration\n"
		       "-c, --command            Execute argument as command\n"
		       "-d, --daemon             Connect only to the specified daemon\n"
		       "-f, --inputfile          Execute commands from specific file and exit\n"
		       "-E, --echo               Echo prompt and command in -c mode\n"
		       "-C, --dryrun             Check configuration for validity and exit\n"
		       "-m, --markfile           Mark input file with context end\n"
		       "    --vty_socket         Override vty socket path\n"
		       "    --config_dir         Override config directory path\n"
		       "-N  --pathspace          Insert prefix into config & socket paths\n"
		       "-u  --user               Run as an unprivileged user\n"
		       "-w, --writeconfig        Write integrated config (frr.conf) and exit\n"
		       "-H, --histfile           Override history file\n"
		       "-t, --timestamp          Print a timestamp before going to shell or reading the configuration\n"
		       "    --no-fork            Don't fork clients to handle daemons (slower for large configs)\n"
		       "-h, --help               Display this help and exit\n\n"
		       "Note that multiple commands may be executed from the command\n"
		       "line by passing multiple -c args, or by embedding linefeed\n"
		       "characters in one or more of the commands.\n\n"
		       "Report bugs to %s\n",
		       progname, FRR_BUG_ADDRESS);

	exit(status);
}

/* VTY shell options, we use GNU getopt library. */
#define OPTION_VTYSOCK 1000
#define OPTION_CONFDIR 1001
#define OPTION_NOFORK 1002
struct option longopts[] = {
	{"boot", no_argument, NULL, 'b'},
	/* For compatibility with older zebra/quagga versions */
	{"eval", required_argument, NULL, 'e'},
	{"command", required_argument, NULL, 'c'},
	{"daemon", required_argument, NULL, 'd'},
	{"vty_socket", required_argument, NULL, OPTION_VTYSOCK},
	{"config_dir", required_argument, NULL, OPTION_CONFDIR},
	{"inputfile", required_argument, NULL, 'f'},
	{"histfile", required_argument, NULL, 'H'},
	{"echo", no_argument, NULL, 'E'},
	{"dryrun", no_argument, NULL, 'C'},
	{"help", no_argument, NULL, 'h'},
	{"noerror", no_argument, NULL, 'n'},
	{"mark", no_argument, NULL, 'm'},
	{"writeconfig", no_argument, NULL, 'w'},
	{"pathspace", required_argument, NULL, 'N'},
	{"user", no_argument, NULL, 'u'},
	{"timestamp", no_argument, NULL, 't'},
	{"no-fork", no_argument, NULL, OPTION_NOFORK},
	{0}};

bool vtysh_loop_exited;

static struct event *vtysh_rl_read_thread;

static void vtysh_rl_read(struct event *thread)
{
	bool *suppress_warnings = EVENT_ARG(thread);

	event_add_read(master, vtysh_rl_read, suppress_warnings, STDIN_FILENO,
		       &vtysh_rl_read_thread);
	rl_callback_read_char();
}

/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
static void vtysh_rl_run(void)
{
	struct event thread;
	bool suppress_warnings = true;

	master = event_master_create(NULL);

	rl_callback_handler_install(vtysh_prompt(), vtysh_rl_callback);
	event_add_read(master, vtysh_rl_read, &suppress_warnings, STDIN_FILENO,
		       &vtysh_rl_read_thread);

	while (!vtysh_loop_exited && event_fetch(master, &thread))
		event_call(&thread);

	if (!vtysh_loop_exited)
		rl_callback_handler_remove();

	event_master_free(master);
}

static void log_it(const char *line)
{
	time_t t = time(NULL);
	struct tm tmp;
	const char *user = getenv("USER");
	char tod[64];

	localtime_r(&t, &tmp);
	if (!user)
		user = "boot";

	strftime(tod, sizeof(tod), "%Y%m%d-%H:%M.%S", &tmp);

	fprintf(logfile, "%s:%s %s\n", tod, user, line);
}

static int flock_fd;

static void vtysh_flock_config(const char *flock_file)
{
	int count = 0;

	flock_fd = open(flock_file, O_RDONLY, 0644);
	if (flock_fd < 0) {
		fprintf(stderr, "Unable to create lock file: %s, %s\n",
			flock_file, safe_strerror(errno));
		return;
	}

	while (count < 400 && (flock(flock_fd, LOCK_EX | LOCK_NB) < 0)) {
		count++;
		usleep(500000);
	}

	if (count >= 400)
		fprintf(stderr,
			"Flock of %s failed, continuing this may cause issues\n",
			flock_file);
}

static void vtysh_unflock_config(void)
{
	flock(flock_fd, LOCK_UN);
	close(flock_fd);
}

void suid_on(void)
{
	if (elevuid != realuid && seteuid(elevuid)) {
		perror("seteuid(on)");
		exit(1);
	}
	if (elevgid != realgid && setegid(elevgid)) {
		perror("setegid(on)");
		exit(1);
	}
}

void suid_off(void)
{
	if (elevuid != realuid && seteuid(realuid)) {
		perror("seteuid(off)");
		exit(1);
	}
	if (elevgid != realgid && setegid(realgid)) {
		perror("setegid(off)");
		exit(1);
	}
}

#define VTYSH_NM_DAEMON 0

#if VTYSH_NM_DAEMON
int vtysh_nm_main(void);
void vtysh_nm_status_config_dump(void);
int vtysh_nm_process(void);
int vtysh_nm_vpp_update(void);
int vtysh_nm_client_check(void);
extern int vtysh_reconnect(struct vtysh_client *vclient);
void sigint_nm(int sig);
static void signal_set_nm (int signo, void (*func)(int));
void signal_init_nm(void);
int log_printf(const char * fmt, ...);
long get_file_size(const char * filename);
const char* getcurdate(long * ioSec);

int vtysh_nm_debug = 0;
#define MAX_LOGFILE_SIZE (2000*1024)
#define TIME_LEN 100
#define SYSAPPLOG            "/var/log/frr_nm"

void sigint_nm(int sig)
{
	log_printf("%s %d signal is %d\n", __FUNCTION__, __LINE__, sig);
    exit(0);
}

/* Signale wrapper. */
static void signal_set_nm (int signo, void (*func)(int))
{
  struct sigaction sig;
  struct sigaction osig;

  sig.sa_handler = func;
  sigemptyset (&sig.sa_mask);
  sig.sa_flags = 0;
#ifdef SA_RESTART
  sig.sa_flags |= SA_RESTART;
#endif /* SA_RESTART */


  sigaddset(&sig.sa_mask, SIGQUIT |SIGTERM ); //(1)
  sig.sa_flags = SA_RESETHAND | SA_NODEFER; //(2)
 
  sigaction (signo, &sig, &osig);
}

/* Initialization of signal handles. */
void signal_init_nm(void)
{
    signal_set_nm(SIGINT, sigint_nm);
    signal_set_nm(SIGTSTP, sigint_nm);
    signal_set_nm(SIGKILL, sigint_nm);
    signal_set_nm(SIGTERM, sigint_nm);
    signal_set_nm(SIGUSR1, sigint_nm);
    signal_set_nm(SIGSEGV, sigint_nm);
    signal_set_nm(SIGPIPE, sigint_nm);
    signal_set_nm(SIGFPE, sigint_nm);
    signal_set_nm(SIGABRT, sigint_nm);
}

int daemon (int nochdir, int noclose)
{
    pid_t pid;

    pid = fork ();

    /* In case of fork is error. */
    if (pid < 0)
    {
        perror ("fork");
        return -1;
    }

    /* In case of this is parent process. */
    if (pid != 0)
    {
        exit (0);
    }

    /* Become session leader and get pid. */
    pid = setsid();

    if (pid < -1)
    {
        perror ("setsid");
        return -1;
    }

    /* Change directory to root. */
    if (! nochdir)
    {
        chdir ("/");
    }

    /* File descriptor close. */
    if (! noclose)
    {
        int fd;

        fd = open("/dev/null",O_RDWR,0);
        if (fd != -1)
        {
            dup2 (fd, STDIN_FILENO);
            dup2 (fd, STDOUT_FILENO);
            dup2 (fd, STDERR_FILENO);
            if (fd > 2)
            {
                close (fd);
            }
        }
    }

    umask (0027);
    return 0;
}

const char* getcurdate(long * ioSec) 
{ 
    static time_t m_time; 
    struct tm pNow; 
    static char tmpDate[TIME_LEN] = { 0 }; 

    memset(&m_time, 0, sizeof(time_t));
    m_time = time( NULL ); 
    if (NULL != ioSec) 
    { 
        *ioSec = m_time; 
    } 

    localtime_r(&m_time,&pNow); 
    memset(tmpDate, 0, sizeof(tmpDate)); 
    sprintf( tmpDate, "%4d-%02d-%02d %02d:%02d:%02d", 
            pNow.tm_year + 1900, pNow.tm_mon + 1, pNow.tm_mday, 
            pNow.tm_hour, pNow.tm_min, pNow.tm_sec); 

    return tmpDate; 
}

long get_file_size(const char * filename )
{
    struct stat f_stat = { 0 };

    if( stat( filename, &f_stat ) == -1 )
    {
        return -1;
    }

   return (long)f_stat.st_size;
}

int log_printf(const char * fmt, ...) 
{ 
    char curtime[TIME_LEN]    = { 0 }; 
    FILE *fp                  = NULL;
    long ace_time             = 0;
    long file_size            = 0;
    int  n                    = 0;
    va_list ap; 

    strcpy( curtime, getcurdate( &ace_time) ); 
    file_size = get_file_size(SYSAPPLOG);

    if ( file_size >= MAX_LOGFILE_SIZE )
    {
        fp = fopen( SYSAPPLOG, "w+" );        
    }
    else
    {   
        fp = fopen( SYSAPPLOG, "a+" ); 
    }

    if ( NULL == fp )
    {
        fp = fopen( SYSAPPLOG, "w+");
    }

    if ( NULL == fp ) 
    {
        return -1;
    }

    fprintf( fp, " %s : ",  curtime ); 

    va_start( ap, fmt );
    n = vfprintf( fp, fmt, ap );
    va_end(ap); 

    fprintf( fp, "\n" );
    fflush( fp );
    fclose( fp );
    return n;
}

void nl_header_init( struct nlmsghdr* p_h,int size,int type );

static int vtysh_nm_socket = -1;
static int rip_nm_status = 0;
static int ripng_nm_status = 0;
static int ospf_nm_status = 0;
static int ospf6_nm_status = 0;
static int bgp_nm_status = 0;

static int hylab_cmd_unix_exchange(const char* cmd_path, long reply_type,
    void* send_buf, int send_len, void* recv_buf, int recv_len, unsigned int reply_timeout_ms)
{
    fd_set read_fset;
    socklen_t peer_len;
    struct sockaddr_un server_addr;
	int size;
	int len;
    int pkt_recv_len = 0, pkt_send_len;
	char my_path[sizeof(server_addr.sun_path)];
    int cmd_result = 0;
    int sock_fd = -1;
    struct timeval tv = {(reply_timeout_ms/1000), (reply_timeout_ms%1000)*1000};

	my_path[0] = 0;
    if((sock_fd = socket(AF_UNIX,SOCK_DGRAM,0)) < 0)
    {  
        log_printf("failed to socket reply_type %ld errno %d cmd_path %s\r\n", reply_type,errno,cmd_path);
        cmd_result = -1;
        goto error_out;
    }

#if 1
    int ret;

    // cat /proc/sys/net/core/rmem_default 
    // cat /proc/sys/net/core/wmem_default 
    if (send_len >= 262144) {
    	size = send_len + 1024;
    	ret = setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    	if (ret < 0) {
    		log_printf("setsockopt rcvbuf %d(%s) failed", errno,strerror(errno));
            cmd_result = -5;
            goto error_out;
    	}
    	
    	ret = setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
    	if (ret < 0) {
    		log_printf("setsockopt rcvbuf %d(%s) failed", errno,strerror(errno));
            cmd_result = -6;
            goto error_out;
    	}
    }
#endif

	memset(&server_addr, 0, sizeof(struct sockaddr_un));
	server_addr.sun_family = AF_UNIX;

	if (recv_buf && recv_len)
	{
		size = snprintf(my_path, sizeof(my_path), "/tmp/.unix_p%u", getpid());
		unlink(my_path);
		strncpy(server_addr.sun_path, my_path, sizeof(server_addr.sun_path));
		server_addr.sun_path[sizeof(server_addr.sun_path)-1] = 0;
		len = offsetof (struct sockaddr_un, sun_path) + size;

		if (bind (sock_fd, (struct sockaddr *)&server_addr, len) < 0) {
			log_printf("bind reply_type %ld rcvbuf %d(%s) failed", reply_type,errno,strerror(errno));
	        cmd_result = -2;
	        goto error_out;
		}
	}

	size = snprintf(server_addr.sun_path, sizeof(server_addr.sun_path), "%s", cmd_path);
	len = offsetof (struct sockaddr_un, sun_path) + size;

    pkt_send_len = sendto(sock_fd, send_buf, send_len, 0, (struct sockaddr*)&server_addr, len);
    if (0 > pkt_send_len)
    {
        cmd_result = -3;
        log_printf("sendto reply_type %ld cmd_result %d errno %d cmd_path %s\r\n", reply_type,pkt_send_len,errno,cmd_path);
        goto error_out;
    }
  //phpext_debug_with_file_line("sendto cmd_result %d\r\n", pkt_len);

    if (recv_len && recv_buf)
    {
        FD_ZERO(&read_fset);
        FD_SET (sock_fd, &read_fset);

        if(0 < select((sock_fd + 1), &read_fset, NULL, NULL, &tv))
        {
            peer_len = sizeof(struct sockaddr_un);
			server_addr.sun_path[0] = 0;
            pkt_recv_len = recvfrom(sock_fd, recv_buf, recv_len, 0, (struct sockaddr*)&server_addr, &peer_len);
            if (0 > pkt_recv_len)
            {
                cmd_result = -4;
            }
//          	phpext_debug_with_file_line("recvfrom reply_type %ld cmd_result %d from %s\r\n", reply_type,pkt_len,server_addr.sun_path);
        }
        else
        {
            cmd_result = -1;
            log_printf("reply_timeout %ums reply_type %ld errno %d(%s) cmd_path %s send %d %d recv %d %d\r\n", 
                reply_timeout_ms,reply_type,errno,strerror(errno),cmd_path,
                send_len, pkt_send_len, recv_len, pkt_recv_len);
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

static void nl_header_init( struct nlmsghdr* p_h,int size,int type )
{
    static int  seq_num = 1;

    memset( p_h, 0, sizeof( struct nlmsghdr ) );
    p_h->nlmsg_len = NLMSG_LENGTH( size );
    p_h->nlmsg_type = type;
    p_h->nlmsg_pid = getpid();
    p_h->nlmsg_seq = seq_num++;
}

void vtysh_nm_status_config_dump(void)
{
    vty_out(vty, "!rip-state-%s\n", rip_nm_status?"enable":"disable");
    vty_out(vty, "!ripng-state-%s\n", ripng_nm_status?"enable":"disable");
    vty_out(vty, "!ospf-state-%s\n", ospf_nm_status?"enable":"disable");
    vty_out(vty, "!ospf6-state-%s\n", ospf6_nm_status?"enable":"disable");
    vty_out(vty, "!bgp-state-%s\n", bgp_nm_status?"enable":"disable");
}

static int vtysh_nm_init(void)
{
	int	sockfd;
	struct sockaddr_in socknam;
	if ((sockfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;

	socknam.sin_family = AF_INET;
	socknam.sin_port = htons(12602);
	socknam.sin_addr.s_addr = inet_addr("127.0.0.1");	

	if (bind (sockfd, (struct sockaddr *)&socknam, sizeof(socknam)) < 0)
		goto bad;

	vtysh_nm_socket = sockfd;
	return 0;
bad:
	close (sockfd);
	return -1;
}

int vtysh_nm_vpp_update(void)
{
#if 0
    struct _nl_ack_message  nl_result   = { 0 };
    NL_DYNAMIC_ROUTE_STATUS_SWAP config_msg  = { 0 };

    if (rip_nm_status) {
        system("echo 1 > /tmp/frr/rip-state");
    } else {
        system("echo 0 > /tmp/frr/rip-state");
    }

    if (ripng_nm_status) {
        system("echo 1 > /tmp/frr/ripng-state");
    } else {
        system("echo 0 > /tmp/frr/ripng-state");
    }

    if (ospf_nm_status) {
        system("echo 1 > /tmp/frr/ospf-state");
    } else {
        system("echo 0 > /tmp/frr/ospf-state");
    }

    if (ospf6_nm_status) {
        system("echo 1 > /tmp/frr/ospf6-state");
    } else {
        system("echo 0 > /tmp/frr/ospf6-state");
    }

    if (bgp_nm_status) {
        system("echo 1 > /tmp/frr/bgp-state");
    } else {
        system("echo 0 > /tmp/frr/bgp-state");
    }

    nl_header_init( &config_msg.hdr,
                    sizeof( NL_DYNAMIC_ROUTE_STATUS_SWAP ) - sizeof(struct nlmsghdr),
                    HYTF_DYNAMIC_ROUTE_STATUS_SET );
    config_msg.rip_status = rip_nm_status;
    config_msg.ripng_status = ripng_nm_status;
    config_msg.ospf_status = ospf_nm_status;
    config_msg.ospf6_status = ospf6_nm_status;
    config_msg.bgp_status = bgp_nm_status;
    return hylab_cmd_unix_exchange(UNIX_SOCK_PATH_FAKE_KERNEL, HYTF_DYNAMIC_ROUTE_STATUS_SET, &config_msg, 
        config_msg.hdr.nlmsg_len, (char *) &nl_result, sizeof( struct _nl_ack_message ), 1500);
#endif
return 0;
}

int vtysh_nm_client_check(void)
{
    unsigned int i;

    for (i = 0; i < array_size(vtysh_client); i++) {
        if (!vtysh_client[i].name) {
            break;
        }
        //printf("%s %d vtysh_client %s fd %d\n", __FUNCTION__, __LINE__, vtysh_client[i].name, vtysh_client[i].fd);
        if (vtysh_client[i].fd < 0) {
            vtysh_reconnect(&vtysh_client[i]);
        }
    }

    return 0;
}

int vtysh_nm_process(void)
{
    int result;
	int sock;
	int nbytes;
	unsigned int addr_len;
	struct zebra_nm_config nm_cmd;
	struct zebra_nm_config nm_reply;
	struct sockaddr_in client_addr;
    char *tmp_str = NULL;
	FILE *fp;
    int node_saved;

    vtysh_nm_init();
	sock = vtysh_nm_socket;
    vtysh_nm_vpp_update();

	while (1) {
        if (0 == access("/tmp/vtysh_nm_debug", F_OK))
            vtysh_nm_debug = 1;
        else
            vtysh_nm_debug = 0;
        
        memset(&nm_cmd, 0x00, sizeof(struct zebra_nm_config));
        addr_len = sizeof(struct sockaddr_in);
        nbytes = recvfrom(sock, &nm_cmd, sizeof(struct zebra_nm_config), 
            0, (struct sockaddr*)&client_addr, &addr_len);
        
        if (nbytes > 0) {
            if (inet_addr("127.0.0.1") != client_addr.sin_addr.s_addr) {
                result = -1;
            } else {
                result = 0;
            }
        } else {
            result = -1;
            log_printf("%s %d failed recvfrom len %d sock %d errno %d(%s)\n", __FUNCTION__, __LINE__, nbytes, sock, errno, strerror(errno));
        }
        
        if (0 == result) {
            if (vtysh_nm_debug) {
                log_printf("%s %d config_cmd %s node_type %d nbytes %d\n", __FUNCTION__, __LINE__, nm_cmd.config_cmd, nm_cmd.cmd_type, nbytes);
            }

            vtysh_nm_client_check();
            nm_reply = nm_cmd;
            switch(nm_cmd.cmd_type) {
                case 100:
                    nm_reply.cmd_result = 0;
                    if (0 == strncmp(nm_cmd.config_cmd, "rip-state-enable", (sizeof("rip-state-enable") - 1))) {
                        rip_nm_status = 1;
                    } else if (0 == strncmp(nm_cmd.config_cmd, "rip-state-disable", (sizeof("rip-state-disable") - 1))) {
                        rip_nm_status = 0;
                    } else if (0 == strncmp(nm_cmd.config_cmd, "ripng-state-enable", (sizeof("rip-state-enable") - 1))) {
                        ripng_nm_status = 1;
                    } else if (0 == strncmp(nm_cmd.config_cmd, "ripng-state-disable", (sizeof("rip-state-disable") - 1))) {
                        ripng_nm_status = 0;
                    } else if (0 == strncmp(nm_cmd.config_cmd, "ospf-state-enable", (sizeof("ospf-state-enable") - 1))) {
                        ospf_nm_status = 1;
                    } else if (0 == strncmp(nm_cmd.config_cmd, "ospf-state-disable", (sizeof("ospf-state-disable") - 1))) {
                        ospf_nm_status = 0;
                    } else if (0 == strncmp(nm_cmd.config_cmd, "ospf6-state-enable", (sizeof("ospf6-state-enable") - 1))) {
                        ospf6_nm_status = 1;
                    } else if (0 == strncmp(nm_cmd.config_cmd, "ospf6-state-disable", (sizeof("ospf6-state-disable") - 1))) {
                        ospf6_nm_status = 0;
                    } else if (0 == strncmp(nm_cmd.config_cmd, "bgp-state-enable", (sizeof("bgp-state-enable") - 1))) {
                        bgp_nm_status = 1;
                    } else if (0 == strncmp(nm_cmd.config_cmd, "bgp-state-disable", (sizeof("bgp-state-disable") - 1))) {
                        bgp_nm_status = 0;
                    } else {
                        nm_reply.cmd_result = 2;
                    }
                    vtysh_nm_vpp_update();
                    break;

                case 101:
                    nm_reply.cmd_result = 0;
                    if (0 == strncmp(nm_cmd.config_cmd, "rip-state-get", sizeof("rip-state-get") - 1)) {
                        memset(nm_reply.config_cmd, 0x00, sizeof(nm_reply.config_cmd));
                        sprintf(nm_reply.config_cmd, "%s", rip_nm_status ? "enable" : "disable");
                    } else if (0 == strncmp(nm_cmd.config_cmd, "ripng-state-get", sizeof("ripng-state-get") - 1)) {
                        memset(nm_reply.config_cmd, 0x00, sizeof(nm_reply.config_cmd));
                        sprintf(nm_reply.config_cmd, "%s", ripng_nm_status ? "enable" : "disable");
                    } else if (0 == strncmp(nm_cmd.config_cmd, "ospf-state-get", sizeof("ospf-state-get") - 1)) {
                        memset(nm_reply.config_cmd, 0x00, sizeof(nm_reply.config_cmd));
                        sprintf(nm_reply.config_cmd, "%s", ospf_nm_status ? "enable" : "disable");
                    } else if (0 == strncmp(nm_cmd.config_cmd, "ospf6-state-get", sizeof("ospf6-state-get") - 1)) {
                        memset(nm_reply.config_cmd, 0x00, sizeof(nm_reply.config_cmd));
                        sprintf(nm_reply.config_cmd, "%s", ospf6_nm_status ? "enable" : "disable");
                    } else if (0 == strncmp(nm_cmd.config_cmd, "bgp-state-get", sizeof("bgp-state-get") - 1)) {
                        memset(nm_reply.config_cmd, 0x00, sizeof(nm_reply.config_cmd));
                        sprintf(nm_reply.config_cmd, "%s", bgp_nm_status ? "enable" : "disable");
                    } else {
                        nm_reply.cmd_result = 2;
                    }
                    break;

                default:
                    node_saved = vty->node;
                    vty->of_saved = vty->of;
                    fp = fopen(nm_cmd.reply_file, "w");
                    if (fp) {
                        vty->of = fp;
                    }

                    if (0 != strncmp(nm_cmd.config_cmd, "show ", 5)
                        && 0 != strncmp(nm_cmd.config_cmd, "write ", 6)) {
                        vtysh_execute("configure terminal");
                    }

                    tmp_str = strtok(nm_cmd.config_cmd, ";");
                    while (tmp_str) {
                        if (vtysh_nm_debug) {
                            log_printf("%s %d node %d cmd %s fd %d wfd %d type %d reply_file %s fp %p\n", __FUNCTION__, __LINE__, 
                                vty->node, tmp_str, vty->fd, vty->wfd, vty->type,
                                nm_cmd.reply_file, fp);
                        }
                        result = vtysh_execute_no_pager(tmp_str);
                        //result = vty_nm_exec(nm_cmd.config_cmd, nm_cmd.reply_file, nm_cmd.cmd_type);
                        if (CMD_SUCCESS == result) {
                            nm_reply.cmd_result = 0;
                        } else {
                            nm_reply.cmd_result = 2;
                            log_printf("%s %d failed to vtysh_execute_no_pager %s node_type %d result %d\n", __FUNCTION__, __LINE__, nm_cmd.config_cmd, nm_cmd.cmd_type, result);
                        }
                        tmp_str = strtok(NULL, ";");
                    }
                    vty->of = vty->of_saved;

                    // restore node tree
                    while (node_saved != vty->node) {
                        vtysh_execute("exit");
                    }
                    //printf("%s %d node_saved %d vty->node %d\n", __FUNCTION__, __LINE__, node_saved, vty->node);
                    if (fp) {
                        fclose(fp);
                    }

                    break;
            }
            nbytes = sendto(sock, &nm_reply, sizeof(struct zebra_nm_config), 0, (struct sockaddr*)&client_addr, addr_len);
            if (nbytes != sizeof(struct zebra_nm_config)) {
                log_printf("%s %d failed to sendto len %d errno %d(%s)\n", __FUNCTION__, __LINE__, nbytes, errno, strerror(errno));
            }
        }
	}
}

int vtysh_nm_main(void)
{
	const char *daemon_name = NULL;
#if 0
	int no_error = 0;
#endif
	char sysconfdir[MAXPATHLEN];
	const char *pathspace_arg = NULL;
	char pathspace[MAXPATHLEN] = "";
	FILE* confp;
	char nm_cmd[512];
    
    system("mkdir /tmp/frr");

	signal_init_nm();

	user_mode = 0;		/* may be set in options processing */

	strlcpy(sysconfdir, frr_sysconfdir, sizeof(sysconfdir));

	strlcpy(vtydir, frr_runstatedir, sizeof(vtydir));

	snprintf(vtysh_config, sizeof(vtysh_config), "%s%s%s", sysconfdir,
		 pathspace, VTYSH_CONFIG_NAME);
	snprintf(frr_config, sizeof(frr_config), "%s%s%s", sysconfdir,
		 pathspace, FRR_CONFIG_NAME);

	if (pathspace_arg) {
		strlcat(vtydir, "/", sizeof(vtydir));
		strlcat(vtydir, pathspace_arg, sizeof(vtydir));
	}

    // restore from current running config
    if (0 == access("/tmp/frr/rip-state", 0)) {
        confp = popen("cat /tmp/frr/rip-state", "r");   
        if (confp) {
            fgets(nm_cmd, sizeof(nm_cmd), confp);
            pclose(confp);
            rip_nm_status = atoi(nm_cmd);
        }

        confp = popen("cat /tmp/frr/ripng-state", "r");   
        if (confp) {
            fgets(nm_cmd, sizeof(nm_cmd), confp);
            pclose(confp);
            ripng_nm_status = atoi(nm_cmd);
        }
        
        confp = popen("cat /tmp/frr/ospf-state", "r");   
        if (confp) {
            fgets(nm_cmd, sizeof(nm_cmd), confp);
            pclose(confp);
            ospf_nm_status = atoi(nm_cmd);
        }
        
        confp = popen("cat /tmp/frr/ospf6-state", "r");   
        if (confp) {
            fgets(nm_cmd, sizeof(nm_cmd), confp);
            pclose(confp);
            ospf6_nm_status = atoi(nm_cmd);
        }
        
        confp = popen("cat /tmp/frr/bgp-state", "r");   
        if (confp) {
            fgets(nm_cmd, sizeof(nm_cmd), confp);
            pclose(confp);
            bgp_nm_status = atoi(nm_cmd);
        }
    } else {
        // restore from saved config
        confp = fopen (frr_config, "r");
        if (NULL != confp) {
            memset(nm_cmd, 0x00, sizeof(nm_cmd));
            while (fgets (nm_cmd, sizeof(nm_cmd), confp)) {
                if (NULL != strstr(nm_cmd, "rip-state-enable")) {
                    rip_nm_status = 1;
                } else if (NULL != strstr(nm_cmd, "rip-state-disable")) {
                    rip_nm_status = 0;
                } else if (NULL != strstr(nm_cmd, "ripng-state-enable")) {
                    ripng_nm_status = 1;
                } else if (NULL != strstr(nm_cmd, "ripng-state-disable")) {
                    ripng_nm_status = 0;
                } else if (NULL != strstr(nm_cmd, "ospf-state-enable")) {
                    ospf_nm_status = 1;
                } else if (NULL != strstr(nm_cmd, "ospf-state-disable")) {
                    ospf_nm_status = 0;
                } else if (NULL != strstr(nm_cmd, "ospf6-state-enable")) {
                    ospf6_nm_status = 1;
                } else if (NULL != strstr(nm_cmd, "ospf6-state-disable")) {
                    ospf6_nm_status = 0;
                } else if (NULL != strstr(nm_cmd, "bgp-state-enable")) {
                    bgp_nm_status = 1;
                } else if (NULL != strstr(nm_cmd, "bgp-state-disable")) {
                    bgp_nm_status = 0;
                }
                memset(nm_cmd, 0x00, sizeof(nm_cmd));   
            }
            fclose(confp);
        }
    }

	/* Make vty structure and register commands. */
	vtysh_init_vty();
	vtysh_init_cmd();
	vtysh_user_init();
	vtysh_config_init();

	vty_init_vtysh();

	log_ref_init();
	lib_error_init();

	/* Do not connect until we have passed authentication. */
	if (vtysh_connect_all(daemon_name) <= 0) {
		log_printf("Exiting: failed to connect to any daemons.\n");
        #if 0
		if (geteuid() != 0)
			fprintf(stderr,
				"Hint: if this seems wrong, try running me as a privileged user!\n");
		if (no_error)
			exit(0);
		else
			exit(1);
        #endif
	}

    daemon(0, 0);

    log_printf("rip %d ripng %d ospf %d ospf6 %d bgp %d\n",
        rip_nm_status, ripng_nm_status, ospf_nm_status, 
        ospf6_nm_status, bgp_nm_status);

    /* Enter into enable node. */
    vtysh_execute("enable");

    /* eval mode. */
    vtysh_nm_process();
    return 0;
}
#else
void vtysh_nm_status_config_dump(void);
void vtysh_nm_status_config_dump(void)
{
}
#endif

#if 1
static void get_process_name(pid_t pid, char *name, int size) 
{
    char path[256];
    FILE *file;
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
	memset(name, 0, size);
	
    file = fopen(path, "r");
    if (file == NULL) {
        return NULL;
    }

    if (fgets(name, size, file) == -1) {
        fclose(file);
        return;
    }

    fclose(file);

    return ;
}

static void kill_self_by_ppname(void)
{
	int i = 0;
	char name[512] = {0};
	char *loguser = NULL;
	const char *ppname[] = {
		"postgres",
		NULL
	};

	get_process_name(getpid(), name, sizeof(name));
	while(ppname[i]){
		if(0 == strncmp(name, ppname[i], strlen(ppname[i]))){
			printf("hycli: kill self by(%s)\n", name);	
			exit(0);
		}
		i++;
	}

	if (access("/etc/.pgsql", 0) == 0)
	{
		loguser = getenv("USER");
		if (NULL != loguser && 0 == strcmp(loguser, "postgres"))
			exit( 0 );
	}	
}
#endif

/* VTY shell main routine. */
int main(int argc, char **argv, char **env)
{
	char *p;
	int opt;
	int dryrun = 0;
	int boot_flag = 0;
	bool ts_flag = false;
	bool no_fork = false;
	const char *daemon_name = NULL;
	const char *inputfile = NULL;
	struct cmd_rec {
		char *line;
		struct cmd_rec *next;
	} *cmd = NULL;
	struct cmd_rec *tail = NULL;
	int echo_command = 0;
	int no_error = 0;
	int markfile = 0;
	int writeconfig = 0;
	int ret = 0;
	char *homedir = NULL;
	int ditch_suid = 0;
	char sysconfdir[MAXPATHLEN];
	const char *pathspace_arg = NULL;
	char pathspace[MAXPATHLEN] = "";
	const char *histfile = NULL;
	const char *histfile_env = getenv("VTYSH_HISTFILE");

	kill_self_by_ppname();

#if VTYSH_NM_DAEMON
    return vtysh_nm_main();
#endif

    /* SUID: drop down to calling user & go back up when needed */
	elevuid = geteuid();
	elevgid = getegid();
	realuid = getuid();
	realgid = getgid();
	suid_off();

	user_mode = 0;		/* may be set in options processing */

	/* Preserve name of myself. */
	progname = ((p = strrchr(argv[0], '/')) ? ++p : argv[0]);

	strlcpy(sysconfdir, frr_sysconfdir, sizeof(sysconfdir));

	strlcpy(vtydir, frr_runstatedir, sizeof(vtydir));

	/* Option handling. */
	while (1) {
		opt = getopt_long(argc, argv, "be:c:d:nf:H:mEhCwN:ut", longopts,
				  0);

		if (opt == EOF)
			break;

		switch (opt) {
		case 0:
			break;
		case 'b':
			boot_flag = 1;
			break;
		case 'e':
		case 'c': {
			struct cmd_rec *cr;
			cr = XMALLOC(MTYPE_TMP, sizeof(*cr));
			cr->line = optarg;
			cr->next = NULL;
			if (tail)
				tail->next = cr;
			else
				cmd = cr;
			tail = cr;
		} break;
		case OPTION_VTYSOCK:
			ditch_suid = 1; /* option disables SUID */
			strlcpy(vtydir, optarg, sizeof(vtydir));
			break;
		case OPTION_CONFDIR:
			ditch_suid = 1; /* option disables SUID */
			snprintf(sysconfdir, sizeof(sysconfdir), "%s/", optarg);
			break;
		case OPTION_NOFORK:
			no_fork = true;
			break;
		case 'N':
			if (strchr(optarg, '/') || strchr(optarg, '.')) {
				fprintf(stderr,
					"slashes or dots are not permitted in the --pathspace option.\n");
				exit(1);
			}
			pathspace_arg = optarg;
			snprintf(pathspace, sizeof(pathspace), "%s/", optarg);
			break;
		case 'd':
			daemon_name = optarg;
			break;
		case 'f':
			inputfile = optarg;
			break;
		case 'm':
			markfile = 1;
			break;
		case 'n':
			no_error = 1;
			break;
		case 'E':
			echo_command = 1;
			break;
		case 'C':
			dryrun = 1;
			break;
		case 'u':
			user_mode = 1;
			break;
		case 't':
			ts_flag = true;
			break;
		case 'w':
			writeconfig = 1;
			break;
		case 'h':
			usage(0);
			break;
		case 'H':
			histfile = optarg;
			break;
		default:
			usage(1);
			break;
		}
	}

	/* No need for forks if we're talking to 1 daemon */
	if (daemon_name)
		no_fork = true;

	if (ditch_suid) {
		elevuid = realuid;
		elevgid = realgid;
	}

	if (markfile + writeconfig + dryrun + boot_flag > 1) {
		fprintf(stderr,
			"Invalid combination of arguments.  Please specify at most one of:\n\t-b, -C, -m, -w\n");
		return 1;
	}
	if (inputfile && (writeconfig || boot_flag)) {
		fprintf(stderr,
			"WARNING: Combining the -f option with -b or -w is NOT SUPPORTED since its\nresults are inconsistent!\n");
	}

	snprintf(vtysh_config, sizeof(vtysh_config), "%s%s%s", sysconfdir,
		 pathspace, VTYSH_CONFIG_NAME);
	snprintf(frr_config, sizeof(frr_config), "%s%s%s", sysconfdir,
		 pathspace, FRR_CONFIG_NAME);

	if (pathspace_arg) {
		strlcat(vtydir, "/", sizeof(vtydir));
		strlcat(vtydir, pathspace_arg, sizeof(vtydir));
	}

	/* Initialize user input buffer. */
	setlinebuf(stdout);

	/* Signal and others. */
	vtysh_signal_init();

	/* Make vty structure and register commands. */
	vtysh_init_vty();
	vtysh_init_cmd();
	vtysh_user_init();
	vtysh_config_init();

	vty_init_vtysh();

	if (!user_mode) {
		/* Read vtysh configuration file before connecting to daemons.
		 * (file may not be readable to calling user in SUID mode) */
		suid_on();
		vtysh_apply_config(vtysh_config, dryrun, false);
		suid_off();
	}
	/* Error code library system */
	log_ref_init();
	lib_error_init();

	if (markfile) {
		if (!inputfile) {
			fprintf(stderr,
				"-f option MUST be specified with -m option\n");
			return 1;
		}
		return (vtysh_mark_file(inputfile));
	}

	/* Start execution only if not in dry-run mode */
	if (dryrun && !cmd) {
		if (inputfile) {
			ret = vtysh_apply_config(inputfile, dryrun, false);
		} else {
			ret = vtysh_apply_config(frr_config, dryrun, false);
		}

		exit(ret);
	}

	if (dryrun && cmd && cmd->line) {
		if (!user_mode)
			vtysh_execute("enable");
		while (cmd) {
			struct cmd_rec *cr;
			char *cmdnow = cmd->line, *next;
			do {
				next = strchr(cmdnow, '\n');
				if (next)
					*next++ = '\0';

				if (echo_command)
					printf("%s%s\n", vtysh_prompt(),
					       cmdnow);

				ret = vtysh_execute_no_pager(cmdnow);
				if (!no_error
				    && !(ret == CMD_SUCCESS
					 || ret == CMD_SUCCESS_DAEMON
					 || ret == CMD_WARNING))
					exit(1);
			} while ((cmdnow = next) != NULL);

			cr = cmd;
			cmd = cmd->next;
			XFREE(MTYPE_TMP, cr);
		}
		exit(ret);
	}

	/* Ignore error messages */
	if (no_error) {
		if (freopen("/dev/null", "w", stdout) == NULL) {
			fprintf(stderr,
				"Exiting: Failed to duplicate stdout with -n option");
			exit(1);
		}
	}

	/* SUID: go back up elevated privs */
	suid_on();

	/* Make sure we pass authentication before proceeding. */
	vtysh_auth();

	/* Do not connect until we have passed authentication. */
	if (vtysh_connect_all(daemon_name) <= 0) {
		fprintf(stderr, "Exiting: failed to connect to any daemons.\n");
		if (geteuid() != 0)
			fprintf(stderr,
				"Hint: if this seems wrong, try running me as a privileged user!\n");
		if (no_error)
			exit(0);
		else
			exit(1);
	}

	/* SUID: back down, don't need privs further on */
	suid_off();

	if (writeconfig) {
		if (user_mode) {
			fprintf(stderr,
				"writeconfig cannot be used when running as an unprivileged user.\n");
			if (no_error)
				exit(0);
			else
				exit(1);
		}
		vtysh_execute("enable");
		return vtysh_write_config_integrated();
	}

	if (boot_flag)
		inputfile = frr_config;

	if (inputfile || boot_flag) {
		vtysh_flock_config(inputfile);
		ret = vtysh_apply_config(inputfile, dryrun, !no_fork);
		vtysh_unflock_config();

		if (no_error)
			ret = 0;

		exit(ret);
	}

	/*
	 * Setup history file for use by both -c and regular input
	 * If we can't find the home directory, then don't store
	 * the history information.
	 * VTYSH_HISTFILE is preferred over command line
	 * argument (-H/--histfile).
	 */
	if (histfile_env) {
		strlcpy(history_file, histfile_env, sizeof(history_file));
	} else if (histfile) {
		strlcpy(history_file, histfile, sizeof(history_file));
	} else {
		homedir = vtysh_get_home();
		if (homedir)
			snprintf(history_file, sizeof(history_file),
				 "%s/.history_frr", homedir);
	}

	if (strlen(history_file) > 0) {
		if (read_history(history_file) != 0) {
			int fp;

			fp = open(history_file, O_CREAT | O_EXCL,
				  S_IRUSR | S_IWUSR);
			if (fp != -1)
				close(fp);

			read_history(history_file);
		}
	}

	if (getenv("VTYSH_LOG")) {
		const char *logpath = getenv("VTYSH_LOG");

		logfile = fopen(logpath, "a");
		if (!logfile) {
			fprintf(stderr, "Failed to open logfile (%s): %s\n",
				logpath, strerror(errno));
			exit(1);
		}
	}

	/* If eval mode. */
	if (cmd && cmd->line) {
		/* Enter into enable node. */
		if (!user_mode)
			vtysh_execute("enable");

		vtysh_add_timestamp = ts_flag;

		while (cmd != NULL) {
			char *eol;

			while ((eol = strchr(cmd->line, '\n')) != NULL) {
				*eol = '\0';

				add_history(cmd->line);
				append_history(1, history_file);

				if (echo_command)
					printf("%s%s\n", vtysh_prompt(),
					       cmd->line);

				if (logfile)
					log_it(cmd->line);

				ret = vtysh_execute_no_pager(cmd->line);
				if (!no_error
				    && !(ret == CMD_SUCCESS
					 || ret == CMD_SUCCESS_DAEMON
					 || ret == CMD_WARNING))
					exit(1);

				cmd->line = eol + 1;
			}

			add_history(cmd->line);
			append_history(1, history_file);

			if (echo_command)
				printf("%s%s\n", vtysh_prompt(), cmd->line);

			if (logfile)
				log_it(cmd->line);

			/*
			 * Parsing logic for regular commands will be different
			 * than for those commands requiring further
			 * processing, such as cli instructions terminating
			 * with question-mark character.
			 */
			if (!vtysh_execute_command_questionmark(cmd->line))
				ret = CMD_SUCCESS;
			else
				ret = vtysh_execute_no_pager(cmd->line);

			if (!no_error
			    && !(ret == CMD_SUCCESS || ret == CMD_SUCCESS_DAEMON
				 || ret == CMD_WARNING))
				exit(1);

			{
				struct cmd_rec *cr;
				cr = cmd;
				cmd = cmd->next;
				XFREE(MTYPE_TMP, cr);
			}
		}

		history_truncate_file(history_file, 1000);
		exit(0);
	}

	vtysh_readline_init();

	vty_hello(vty);

	/* Enter into enable node. */
	if (!user_mode){
		vtysh_execute("enable");
		vtysh_execute("disable");
	}
	
	vtysh_add_timestamp = ts_flag;

	/* Main command loop. */
	vtysh_rl_run();

	vtysh_uninit();

	history_truncate_file(history_file, 1000);
	printf("\n");

	/* Rest in peace. */
	exit(0);
}
