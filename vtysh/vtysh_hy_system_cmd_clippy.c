#if 1
/* show_interface => "show interface [NAME$interface_name]" */
DEFUN_CMD_FUNC_DECL(show_interface)
#define funcdecl_show_interface static int show_interface_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	const char * interface_name)
funcdecl_show_interface;
DEFUN_CMD_FUNC_TEXT(show_interface)
{
#if 1 /* anything to parse? */
	int _i;
#if 0 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	const char *interface_name = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 0 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "interface_name")) {
			interface_name = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
#if 0 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 0 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif

	return show_interface_magic(self, vty, argc, argv, interface_name);
}
#endif

/* vtysh_ip_route_static => "ip route-static A.B.C.D$dest {A.B.C.D$mask|(0-32)$mask_len} A.B.C.D$next [(1-255)$pref]" */
DEFUN_CMD_FUNC_DECL(vtysh_ip_route_static)
#define funcdecl_vtysh_ip_route_static static int vtysh_ip_route_static_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	struct in_addr dest,\
	const char * dest_str __attribute__ ((unused)),\
	struct in_addr mask,\
	const char * mask_str __attribute__ ((unused)),\
	long mask_len,\
	const char * mask_len_str __attribute__ ((unused)),\
	struct in_addr next,\
	const char * next_str __attribute__ ((unused)),\
	long pref,\
	const char * pref_str __attribute__ ((unused)))
funcdecl_vtysh_ip_route_static;
DEFUN_CMD_FUNC_TEXT(vtysh_ip_route_static)
{
#if 5 /* anything to parse? */
	int _i;
#if 1 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	struct in_addr dest = { INADDR_ANY };
	const char *dest_str = NULL;
	struct in_addr mask = { INADDR_ANY };
	const char *mask_str = NULL;
	long mask_len = 0;
	const char *mask_len_str = NULL;
	struct in_addr next = { INADDR_ANY };
	const char *next_str = NULL;
	long pref = 0;
	const char *pref_str = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 1 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "dest")) {
			dest_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &dest);
		}
		if (!strcmp(argv[_i]->varname, "mask")) {
			mask_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &mask);
		}
		if (!strcmp(argv[_i]->varname, "mask_len")) {
			mask_len_str = argv[_i]->arg;
			char *_end;
			mask_len = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "next")) {
			next_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &next);
		}
		if (!strcmp(argv[_i]->varname, "pref")) {
			pref_str = argv[_i]->arg;
			char *_end;
			pref = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
#if 1 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 1 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif
	if (!dest_str) {
		vty_out(vty, "Internal CLI error [%s]\n", "dest_str");
		return CMD_WARNING;
	}
	if (!next_str) {
		vty_out(vty, "Internal CLI error [%s]\n", "next_str");
		return CMD_WARNING;
	}

	return vtysh_ip_route_static_magic(self, vty, argc, argv, dest, dest_str, mask, mask_str, mask_len, mask_len_str, next, next_str, pref, pref_str);
}

/* no_vtysh_ip_route_static => "no ip route-static A.B.C.D$dest {A.B.C.D$mask|(0-32)$mask_len} A.B.C.D$next" */
DEFUN_CMD_FUNC_DECL(no_vtysh_ip_route_static)
#define funcdecl_no_vtysh_ip_route_static static int no_vtysh_ip_route_static_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	struct in_addr dest,\
	const char * dest_str __attribute__ ((unused)),\
	struct in_addr mask,\
	const char * mask_str __attribute__ ((unused)),\
	long mask_len,\
	const char * mask_len_str __attribute__ ((unused)),\
	struct in_addr next,\
	const char * next_str __attribute__ ((unused)))
funcdecl_no_vtysh_ip_route_static;
DEFUN_CMD_FUNC_TEXT(no_vtysh_ip_route_static)
{
#if 4 /* anything to parse? */
	int _i;
#if 1 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	struct in_addr dest = { INADDR_ANY };
	const char *dest_str = NULL;
	struct in_addr mask = { INADDR_ANY };
	const char *mask_str = NULL;
	long mask_len = 0;
	const char *mask_len_str = NULL;
	struct in_addr next = { INADDR_ANY };
	const char *next_str = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 1 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "dest")) {
			dest_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &dest);
		}
		if (!strcmp(argv[_i]->varname, "mask")) {
			mask_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &mask);
		}
		if (!strcmp(argv[_i]->varname, "mask_len")) {
			mask_len_str = argv[_i]->arg;
			char *_end;
			mask_len = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "next")) {
			next_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &next);
		}
#if 1 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 1 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif
	if (!dest_str) {
		vty_out(vty, "Internal CLI error [%s]\n", "dest_str");
		return CMD_WARNING;
	}
	if (!next_str) {
		vty_out(vty, "Internal CLI error [%s]\n", "next_str");
		return CMD_WARNING;
	}

	return no_vtysh_ip_route_static_magic(self, vty, argc, argv, dest, dest_str, mask, mask_str, mask_len, mask_len_str, next, next_str);
}

/* configuration_reauthentication => "configuration reauthentication {enable|disable}$token" */
DEFUN_CMD_FUNC_DECL(configuration_reauthentication)
#define funcdecl_configuration_reauthentication static int configuration_reauthentication_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	const char * token)
funcdecl_configuration_reauthentication;
DEFUN_CMD_FUNC_TEXT(configuration_reauthentication)
{
#if 1 /* anything to parse? */
	int _i;
#if 0 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	const char *token = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 0 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "token")) {
			token = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
#if 0 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 0 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif
	if (!token) {
		vty_out(vty, "Internal CLI error [%s]\n", "token");
		return CMD_WARNING;
	}

	return configuration_reauthentication_magic(self, vty, argc, argv, token);
}

/* update_force_rule => "update force {applib|urllib|avlib|ipslib|waflib|geoip-lib|ssl-vpn-client|security-client}$token" */
DEFUN_CMD_FUNC_DECL(update_force_rule)
#define funcdecl_update_force_rule static int update_force_rule_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	const char * token)
funcdecl_update_force_rule;
DEFUN_CMD_FUNC_TEXT(update_force_rule)
{
#if 1 /* anything to parse? */
	int _i;
#if 0 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	const char *token = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 0 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "token")) {
			token = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
#if 0 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 0 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif
	if (!token) {
		vty_out(vty, "Internal CLI error [%s]\n", "token");
		return CMD_WARNING;
	}

	return update_force_rule_magic(self, vty, argc, argv, token);
}

/* update_online_rule => "update online {applib|urllib|avlib|ipslib|waflib|geoip-lib|security-client|safe-patch}$token" */
DEFUN_CMD_FUNC_DECL(update_online_rule)
#define funcdecl_update_online_rule static int update_online_rule_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	const char * token)
funcdecl_update_online_rule;
DEFUN_CMD_FUNC_TEXT(update_online_rule)
{
#if 1 /* anything to parse? */
	int _i;
#if 0 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	const char *token = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 0 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "token")) {
			token = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
#if 0 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 0 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif
	if (!token) {
		vty_out(vty, "Internal CLI error [%s]\n", "token");
		return CMD_WARNING;
	}

	return update_online_rule_magic(self, vty, argc, argv, token);
}

/* debug_pcappacket_start => "pcappacket INTERFACE [ether-type ETYPE$etype|arp] [vlan (1-4095)$vlanid]  [ip1 A.B.C.D$ip1|ip1 A.B.C.D/M$ip1_subnet| ip1 X:X::X:X$ip1v6 | ip1 X:X::X:X/M$ip1v6_subnet ] [ip2 A.B.C.D$ip2|ip2 A.B.C.D/M$ip2_subnet| ip2 X:X::X:X$ip2v6 | ip2 X:X::X:X/M$ip2v6_subnet ] [ipv4-proto (1-255)$ipv4proto | ipv6-proto (1-255)$ipv6proto | icmp | icmpv6 | tcp [port1 (1-65535)$port1] [port2 (1-65535)$port2] | udp [port1 (1-65535)$u_port1] [port2 (1-65535)$u_port2]] [count (1-10000)$count]" */
DEFUN_CMD_FUNC_DECL(debug_pcappacket_start)
#define funcdecl_debug_pcappacket_start static int debug_pcappacket_start_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	const char * interface,\
	const char * etype,\
	long vlanid,\
	const char * vlanid_str __attribute__ ((unused)),\
	struct in_addr ip1,\
	const char * ip1_str __attribute__ ((unused)),\
	const struct prefix_ipv4 * ip1_subnet,\
	const char * ip1_subnet_str __attribute__ ((unused)),\
	struct in6_addr ip1v6,\
	const char * ip1v6_str __attribute__ ((unused)),\
	const struct prefix_ipv6 * ip1v6_subnet,\
	const char * ip1v6_subnet_str __attribute__ ((unused)),\
	struct in_addr ip2,\
	const char * ip2_str __attribute__ ((unused)),\
	const struct prefix_ipv4 * ip2_subnet,\
	const char * ip2_subnet_str __attribute__ ((unused)),\
	struct in6_addr ip2v6,\
	const char * ip2v6_str __attribute__ ((unused)),\
	const struct prefix_ipv6 * ip2v6_subnet,\
	const char * ip2v6_subnet_str __attribute__ ((unused)),\
	long ipv4proto,\
	const char * ipv4proto_str __attribute__ ((unused)),\
	long ipv6proto,\
	const char * ipv6proto_str __attribute__ ((unused)),\
	long port1,\
	const char * port1_str __attribute__ ((unused)),\
	long port2,\
	const char * port2_str __attribute__ ((unused)),\
	long u_port1,\
	const char * u_port1_str __attribute__ ((unused)),\
	long u_port2,\
	const char * u_port2_str __attribute__ ((unused)),\
	long count,\
	const char * count_str __attribute__ ((unused)))
funcdecl_debug_pcappacket_start;
DEFUN_CMD_FUNC_TEXT(debug_pcappacket_start)
{
#if 18 /* anything to parse? */
	int _i;
#if 1 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	const char *interface = NULL;
	const char *etype = NULL;
	long vlanid = 0;
	const char *vlanid_str = NULL;
	struct in_addr ip1 = { INADDR_ANY };
	const char *ip1_str = NULL;
	struct prefix_ipv4 ip1_subnet = { };
	const char *ip1_subnet_str = NULL;
	struct in6_addr ip1v6 = {};
	const char *ip1v6_str = NULL;
	struct prefix_ipv6 ip1v6_subnet = { };
	const char *ip1v6_subnet_str = NULL;
	struct in_addr ip2 = { INADDR_ANY };
	const char *ip2_str = NULL;
	struct prefix_ipv4 ip2_subnet = { };
	const char *ip2_subnet_str = NULL;
	struct in6_addr ip2v6 = {};
	const char *ip2v6_str = NULL;
	struct prefix_ipv6 ip2v6_subnet = { };
	const char *ip2v6_subnet_str = NULL;
	long ipv4proto = 0;
	const char *ipv4proto_str = NULL;
	long ipv6proto = 0;
	const char *ipv6proto_str = NULL;
	long port1 = 0;
	const char *port1_str = NULL;
	long port2 = 0;
	const char *port2_str = NULL;
	long u_port1 = 0;
	const char *u_port1_str = NULL;
	long u_port2 = 0;
	const char *u_port2_str = NULL;
	long count = 0;
	const char *count_str = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 1 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "interface")) {
			interface = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
		if (!strcmp(argv[_i]->varname, "etype")) {
			etype = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
		if (!strcmp(argv[_i]->varname, "vlanid")) {
			vlanid_str = argv[_i]->arg;
			char *_end;
			vlanid = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "ip1")) {
			ip1_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &ip1);
		}
		if (!strcmp(argv[_i]->varname, "ip1_subnet")) {
			ip1_subnet_str = argv[_i]->arg;
			_fail = !str2prefix_ipv4(argv[_i]->arg, &ip1_subnet);
		}
		if (!strcmp(argv[_i]->varname, "ip1v6")) {
			ip1v6_str = argv[_i]->arg;
			_fail = !inet_pton(AF_INET6, argv[_i]->arg, &ip1v6);
		}
		if (!strcmp(argv[_i]->varname, "ip1v6_subnet")) {
			ip1v6_subnet_str = argv[_i]->arg;
			_fail = !str2prefix_ipv6(argv[_i]->arg, &ip1v6_subnet);
		}
		if (!strcmp(argv[_i]->varname, "ip2")) {
			ip2_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &ip2);
		}
		if (!strcmp(argv[_i]->varname, "ip2_subnet")) {
			ip2_subnet_str = argv[_i]->arg;
			_fail = !str2prefix_ipv4(argv[_i]->arg, &ip2_subnet);
		}
		if (!strcmp(argv[_i]->varname, "ip2v6")) {
			ip2v6_str = argv[_i]->arg;
			_fail = !inet_pton(AF_INET6, argv[_i]->arg, &ip2v6);
		}
		if (!strcmp(argv[_i]->varname, "ip2v6_subnet")) {
			ip2v6_subnet_str = argv[_i]->arg;
			_fail = !str2prefix_ipv6(argv[_i]->arg, &ip2v6_subnet);
		}
		if (!strcmp(argv[_i]->varname, "ipv4proto")) {
			ipv4proto_str = argv[_i]->arg;
			char *_end;
			ipv4proto = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "ipv6proto")) {
			ipv6proto_str = argv[_i]->arg;
			char *_end;
			ipv6proto = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "port1")) {
			port1_str = argv[_i]->arg;
			char *_end;
			port1 = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "port2")) {
			port2_str = argv[_i]->arg;
			char *_end;
			port2 = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "u_port1")) {
			u_port1_str = argv[_i]->arg;
			char *_end;
			u_port1 = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "u_port2")) {
			u_port2_str = argv[_i]->arg;
			char *_end;
			u_port2 = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "count")) {
			count_str = argv[_i]->arg;
			char *_end;
			count = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
#if 1 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 1 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif
	if (!interface) {
		vty_out(vty, "Internal CLI error [%s]\n", "interface");
		return CMD_WARNING;
	}

	return debug_pcappacket_start_magic(self, vty, argc, argv, interface, etype, vlanid, vlanid_str, ip1, ip1_str, &ip1_subnet, ip1_subnet_str, ip1v6, ip1v6_str, &ip1v6_subnet, ip1v6_subnet_str, ip2, ip2_str, &ip2_subnet, ip2_subnet_str, ip2v6, ip2v6_str, &ip2v6_subnet, ip2v6_subnet_str, ipv4proto, ipv4proto_str, ipv6proto, ipv6proto_str, port1, port1_str, port2, port2_str, u_port1, u_port1_str, u_port2, u_port2_str, count, count_str);
}

/* debug_pcapdrop_start => "pcapdrop INTERFACE [ether-type ETYPE$etype|arp] [vlan (1-4095)$vlanid]  [ip1 A.B.C.D$ip1|ip1 A.B.C.D/M$ip1_subnet| ip1 X:X::X:X$ip1v6 | ip1 X:X::X:X/M$ip1v6_subnet ] [ip2 A.B.C.D$ip2|ip2 A.B.C.D/M$ip2_subnet| ip2 X:X::X:X$ip2v6 | ip2 X:X::X:X/M$ip2v6_subnet ] [ipv4-proto (1-255)$ipv4proto | ipv6-proto (1-255)$ipv6proto | icmp | icmpv6 | tcp [port1 (1-65535)$port1] [port2 (1-65535)$port2] | udp [port1 (1-65535)$u_port1] [port2 (1-65535)$u_port2]] [count (1-10000)$count]" */
DEFUN_CMD_FUNC_DECL(debug_pcapdrop_start)
#define funcdecl_debug_pcapdrop_start static int debug_pcapdrop_start_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	const char * interface,\
	const char * etype,\
	long vlanid,\
	const char * vlanid_str __attribute__ ((unused)),\
	struct in_addr ip1,\
	const char * ip1_str __attribute__ ((unused)),\
	const struct prefix_ipv4 * ip1_subnet,\
	const char * ip1_subnet_str __attribute__ ((unused)),\
	struct in6_addr ip1v6,\
	const char * ip1v6_str __attribute__ ((unused)),\
	const struct prefix_ipv6 * ip1v6_subnet,\
	const char * ip1v6_subnet_str __attribute__ ((unused)),\
	struct in_addr ip2,\
	const char * ip2_str __attribute__ ((unused)),\
	const struct prefix_ipv4 * ip2_subnet,\
	const char * ip2_subnet_str __attribute__ ((unused)),\
	struct in6_addr ip2v6,\
	const char * ip2v6_str __attribute__ ((unused)),\
	const struct prefix_ipv6 * ip2v6_subnet,\
	const char * ip2v6_subnet_str __attribute__ ((unused)),\
	long ipv4proto,\
	const char * ipv4proto_str __attribute__ ((unused)),\
	long ipv6proto,\
	const char * ipv6proto_str __attribute__ ((unused)),\
	long port1,\
	const char * port1_str __attribute__ ((unused)),\
	long port2,\
	const char * port2_str __attribute__ ((unused)),\
	long u_port1,\
	const char * u_port1_str __attribute__ ((unused)),\
	long u_port2,\
	const char * u_port2_str __attribute__ ((unused)),\
	long count,\
	const char * count_str __attribute__ ((unused)))
funcdecl_debug_pcapdrop_start;
DEFUN_CMD_FUNC_TEXT(debug_pcapdrop_start)
{
#if 18 /* anything to parse? */
	int _i;
#if 1 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	const char *interface = NULL;
	const char *etype = NULL;
	long vlanid = 0;
	const char *vlanid_str = NULL;
	struct in_addr ip1 = { INADDR_ANY };
	const char *ip1_str = NULL;
	struct prefix_ipv4 ip1_subnet = { };
	const char *ip1_subnet_str = NULL;
	struct in6_addr ip1v6 = {};
	const char *ip1v6_str = NULL;
	struct prefix_ipv6 ip1v6_subnet = { };
	const char *ip1v6_subnet_str = NULL;
	struct in_addr ip2 = { INADDR_ANY };
	const char *ip2_str = NULL;
	struct prefix_ipv4 ip2_subnet = { };
	const char *ip2_subnet_str = NULL;
	struct in6_addr ip2v6 = {};
	const char *ip2v6_str = NULL;
	struct prefix_ipv6 ip2v6_subnet = { };
	const char *ip2v6_subnet_str = NULL;
	long ipv4proto = 0;
	const char *ipv4proto_str = NULL;
	long ipv6proto = 0;
	const char *ipv6proto_str = NULL;
	long port1 = 0;
	const char *port1_str = NULL;
	long port2 = 0;
	const char *port2_str = NULL;
	long u_port1 = 0;
	const char *u_port1_str = NULL;
	long u_port2 = 0;
	const char *u_port2_str = NULL;
	long count = 0;
	const char *count_str = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 1 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "interface")) {
			interface = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
		if (!strcmp(argv[_i]->varname, "etype")) {
			etype = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
		if (!strcmp(argv[_i]->varname, "vlanid")) {
			vlanid_str = argv[_i]->arg;
			char *_end;
			vlanid = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "ip1")) {
			ip1_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &ip1);
		}
		if (!strcmp(argv[_i]->varname, "ip1_subnet")) {
			ip1_subnet_str = argv[_i]->arg;
			_fail = !str2prefix_ipv4(argv[_i]->arg, &ip1_subnet);
		}
		if (!strcmp(argv[_i]->varname, "ip1v6")) {
			ip1v6_str = argv[_i]->arg;
			_fail = !inet_pton(AF_INET6, argv[_i]->arg, &ip1v6);
		}
		if (!strcmp(argv[_i]->varname, "ip1v6_subnet")) {
			ip1v6_subnet_str = argv[_i]->arg;
			_fail = !str2prefix_ipv6(argv[_i]->arg, &ip1v6_subnet);
		}
		if (!strcmp(argv[_i]->varname, "ip2")) {
			ip2_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &ip2);
		}
		if (!strcmp(argv[_i]->varname, "ip2_subnet")) {
			ip2_subnet_str = argv[_i]->arg;
			_fail = !str2prefix_ipv4(argv[_i]->arg, &ip2_subnet);
		}
		if (!strcmp(argv[_i]->varname, "ip2v6")) {
			ip2v6_str = argv[_i]->arg;
			_fail = !inet_pton(AF_INET6, argv[_i]->arg, &ip2v6);
		}
		if (!strcmp(argv[_i]->varname, "ip2v6_subnet")) {
			ip2v6_subnet_str = argv[_i]->arg;
			_fail = !str2prefix_ipv6(argv[_i]->arg, &ip2v6_subnet);
		}
		if (!strcmp(argv[_i]->varname, "ipv4proto")) {
			ipv4proto_str = argv[_i]->arg;
			char *_end;
			ipv4proto = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "ipv6proto")) {
			ipv6proto_str = argv[_i]->arg;
			char *_end;
			ipv6proto = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "port1")) {
			port1_str = argv[_i]->arg;
			char *_end;
			port1 = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "port2")) {
			port2_str = argv[_i]->arg;
			char *_end;
			port2 = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "u_port1")) {
			u_port1_str = argv[_i]->arg;
			char *_end;
			u_port1 = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "u_port2")) {
			u_port2_str = argv[_i]->arg;
			char *_end;
			u_port2 = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "count")) {
			count_str = argv[_i]->arg;
			char *_end;
			count = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
#if 1 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 1 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif
	if (!interface) {
		vty_out(vty, "Internal CLI error [%s]\n", "interface");
		return CMD_WARNING;
	}

	return debug_pcapdrop_start_magic(self, vty, argc, argv, interface, etype, vlanid, vlanid_str, ip1, ip1_str, &ip1_subnet, ip1_subnet_str, ip1v6, ip1v6_str, &ip1v6_subnet, ip1v6_subnet_str, ip2, ip2_str, &ip2_subnet, ip2_subnet_str, ip2v6, ip2v6_str, &ip2v6_subnet, ip2v6_subnet_str, ipv4proto, ipv4proto_str, ipv6proto, ipv6proto_str, port1, port1_str, port2, port2_str, u_port1, u_port1_str, u_port2, u_port2_str, count, count_str);
}

/* debug_show_corefile => "show corefile [FILENAME$filename]" */
DEFUN_CMD_FUNC_DECL(debug_show_corefile)
#define funcdecl_debug_show_corefile static int debug_show_corefile_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	const char * filename)
funcdecl_debug_show_corefile;
DEFUN_CMD_FUNC_TEXT(debug_show_corefile)
{
#if 1 /* anything to parse? */
	int _i;
#if 0 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	const char *filename = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 0 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "filename")) {
			filename = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
#if 0 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 0 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif

	return debug_show_corefile_magic(self, vty, argc, argv, filename);
}

/* debug_show_variable => "show variable <dpdk|userspace|dpi|sslvpn|collector> NAME$name" */
DEFUN_CMD_FUNC_DECL(debug_show_variable)
#define funcdecl_debug_show_variable static int debug_show_variable_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	const char * name)
funcdecl_debug_show_variable;
DEFUN_CMD_FUNC_TEXT(debug_show_variable)
{
#if 1 /* anything to parse? */
	int _i;
#if 0 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	const char *name = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 0 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "name")) {
			name = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
#if 0 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 0 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif
	if (!name) {
		vty_out(vty, "Internal CLI error [%s]\n", "name");
		return CMD_WARNING;
	}

	return debug_show_variable_magic(self, vty, argc, argv, name);
}

/* debug_variable => "variable <dpdk|userspace|dpi|sslvpn|collector> NAME$name VALUE$vvalue" */
DEFUN_CMD_FUNC_DECL(debug_variable)
#define funcdecl_debug_variable static int debug_variable_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	const char * name,\
	const char * vvalue)
funcdecl_debug_variable;
DEFUN_CMD_FUNC_TEXT(debug_variable)
{
#if 2 /* anything to parse? */
	int _i;
#if 0 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	const char *name = NULL;
	const char *vvalue = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 0 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "name")) {
			name = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
		if (!strcmp(argv[_i]->varname, "vvalue")) {
			vvalue = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
#if 0 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 0 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif
	if (!name) {
		vty_out(vty, "Internal CLI error [%s]\n", "name");
		return CMD_WARNING;
	}
	if (!vvalue) {
		vty_out(vty, "Internal CLI error [%s]\n", "vvalue");
		return CMD_WARNING;
	}

	return debug_variable_magic(self, vty, argc, argv, name, vvalue);
}

/* debug_show_session => "show session [protocol <tcp|udp|icmp|gre>$pro] [sip <A.B.C.D$sipv4|X:X::X:X$sipv6>] [sport (0-65535)$sport] [dip <A.B.C.D$dipv4|X:X::X:X$dipv6>] [dport (0-65535)$dport] [lines (1-10000)$line]" */
DEFUN_CMD_FUNC_DECL(debug_show_session)
#define funcdecl_debug_show_session static int debug_show_session_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	const char * pro,\
	struct in_addr sipv4,\
	const char * sipv4_str __attribute__ ((unused)),\
	struct in6_addr sipv6,\
	const char * sipv6_str __attribute__ ((unused)),\
	long sport,\
	const char * sport_str __attribute__ ((unused)),\
	struct in_addr dipv4,\
	const char * dipv4_str __attribute__ ((unused)),\
	struct in6_addr dipv6,\
	const char * dipv6_str __attribute__ ((unused)),\
	long dport,\
	const char * dport_str __attribute__ ((unused)),\
	long line,\
	const char * line_str __attribute__ ((unused)))
funcdecl_debug_show_session;
DEFUN_CMD_FUNC_TEXT(debug_show_session)
{
#if 8 /* anything to parse? */
	int _i;
#if 1 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	const char *pro = NULL;
	struct in_addr sipv4 = { INADDR_ANY };
	const char *sipv4_str = NULL;
	struct in6_addr sipv6 = {};
	const char *sipv6_str = NULL;
	long sport = 0;
	const char *sport_str = NULL;
	struct in_addr dipv4 = { INADDR_ANY };
	const char *dipv4_str = NULL;
	struct in6_addr dipv6 = {};
	const char *dipv6_str = NULL;
	long dport = 0;
	const char *dport_str = NULL;
	long line = 0;
	const char *line_str = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 1 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "pro")) {
			pro = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
		if (!strcmp(argv[_i]->varname, "sipv4")) {
			sipv4_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &sipv4);
		}
		if (!strcmp(argv[_i]->varname, "sipv6")) {
			sipv6_str = argv[_i]->arg;
			_fail = !inet_pton(AF_INET6, argv[_i]->arg, &sipv6);
		}
		if (!strcmp(argv[_i]->varname, "sport")) {
			sport_str = argv[_i]->arg;
			char *_end;
			sport = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "dipv4")) {
			dipv4_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &dipv4);
		}
		if (!strcmp(argv[_i]->varname, "dipv6")) {
			dipv6_str = argv[_i]->arg;
			_fail = !inet_pton(AF_INET6, argv[_i]->arg, &dipv6);
		}
		if (!strcmp(argv[_i]->varname, "dport")) {
			dport_str = argv[_i]->arg;
			char *_end;
			dport = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "line")) {
			line_str = argv[_i]->arg;
			char *_end;
			line = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
#if 1 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 1 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif

	return debug_show_session_magic(self, vty, argc, argv, pro, sipv4, sipv4_str, sipv6, sipv6_str, sport, sport_str, dipv4, dipv4_str, dipv6, dipv6_str, dport, dport_str, line, line_str);
}

/* debug_show_host => "show host <A.B.C.D$sipv4|X:X::X:X$sipv6>" */
DEFUN_CMD_FUNC_DECL(debug_show_host)
#define funcdecl_debug_show_host static int debug_show_host_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	struct in_addr sipv4,\
	const char * sipv4_str __attribute__ ((unused)),\
	struct in6_addr sipv6,\
	const char * sipv6_str __attribute__ ((unused)))
funcdecl_debug_show_host;
DEFUN_CMD_FUNC_TEXT(debug_show_host)
{
#if 2 /* anything to parse? */
	int _i;
#if 1 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	struct in_addr sipv4 = { INADDR_ANY };
	const char *sipv4_str = NULL;
	struct in6_addr sipv6 = {};
	const char *sipv6_str = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 1 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "sipv4")) {
			sipv4_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &sipv4);
		}
		if (!strcmp(argv[_i]->varname, "sipv6")) {
			sipv6_str = argv[_i]->arg;
			_fail = !inet_pton(AF_INET6, argv[_i]->arg, &sipv6);
		}
#if 1 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 1 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif

	return debug_show_host_magic(self, vty, argc, argv, sipv4, sipv4_str, sipv6, sipv6_str);
}

/* debug_show_trace => "show trace <dpdk|sum>" */
DEFUN_CMD_FUNC_DECL(debug_show_trace)
#define funcdecl_debug_show_trace static int debug_show_trace_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)))
funcdecl_debug_show_trace;
DEFUN_CMD_FUNC_TEXT(debug_show_trace)
{
#if 0 /* anything to parse? */
	int _i;
#if 0 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 0 /* anything that can fail? */
		_fail = 0;
#endif

#if 0 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 0 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif

	return debug_show_trace_magic(self, vty, argc, argv);
}

/* debug_trace_clear => "trace-clear <all|dpdk|sum>" */
DEFUN_CMD_FUNC_DECL(debug_trace_clear)
#define funcdecl_debug_trace_clear static int debug_trace_clear_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)))
funcdecl_debug_trace_clear;
DEFUN_CMD_FUNC_TEXT(debug_trace_clear)
{
#if 0 /* anything to parse? */
	int _i;
#if 0 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 0 /* anything that can fail? */
		_fail = 0;
#endif

#if 0 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 0 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif

	return debug_trace_clear_magic(self, vty, argc, argv);
}

/* debug_trace_save => "trace-save <all|(1-99)$num>" */
DEFUN_CMD_FUNC_DECL(debug_trace_save)
#define funcdecl_debug_trace_save static int debug_trace_save_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	long num,\
	const char * num_str __attribute__ ((unused)))
funcdecl_debug_trace_save;
DEFUN_CMD_FUNC_TEXT(debug_trace_save)
{
#if 1 /* anything to parse? */
	int _i;
#if 1 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	long num = 0;
	const char *num_str = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 1 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "num")) {
			num_str = argv[_i]->arg;
			char *_end;
			num = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
#if 1 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 1 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif

	return debug_trace_save_magic(self, vty, argc, argv, num, num_str);
}

/* debug_trace_add => "trace-add <dpdk-input|af-packet-input|virtio-input> (1-100)$num [protocol <(0-127)$pro|tcp|udp|icmp|icmp6|esp|arp|ppp|all>] [sip <A.B.C.D$sipv4|X:X::X:X$sipv6>] [sport (0-65535)$sport] [dip <A.B.C.D$dipv4|X:X::X:X$dipv6>] [dport (0-65535)$dport] type <path|drop|packet|path-drop|path-packet|drop-packet|path-drop-packet> [oneway]" */
DEFUN_CMD_FUNC_DECL(debug_trace_add)
#define funcdecl_debug_trace_add static int debug_trace_add_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	long num,\
	const char * num_str __attribute__ ((unused)),\
	long pro,\
	const char * pro_str __attribute__ ((unused)),\
	struct in_addr sipv4,\
	const char * sipv4_str __attribute__ ((unused)),\
	struct in6_addr sipv6,\
	const char * sipv6_str __attribute__ ((unused)),\
	long sport,\
	const char * sport_str __attribute__ ((unused)),\
	struct in_addr dipv4,\
	const char * dipv4_str __attribute__ ((unused)),\
	struct in6_addr dipv6,\
	const char * dipv6_str __attribute__ ((unused)),\
	long dport,\
	const char * dport_str __attribute__ ((unused)))
funcdecl_debug_trace_add;
DEFUN_CMD_FUNC_TEXT(debug_trace_add)
{
#if 8 /* anything to parse? */
	int _i;
#if 1 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	long num = 0;
	const char *num_str = NULL;
	long pro = 0;
	const char *pro_str = NULL;
	struct in_addr sipv4 = { INADDR_ANY };
	const char *sipv4_str = NULL;
	struct in6_addr sipv6 = {};
	const char *sipv6_str = NULL;
	long sport = 0;
	const char *sport_str = NULL;
	struct in_addr dipv4 = { INADDR_ANY };
	const char *dipv4_str = NULL;
	struct in6_addr dipv6 = {};
	const char *dipv6_str = NULL;
	long dport = 0;
	const char *dport_str = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 1 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "num")) {
			num_str = argv[_i]->arg;
			char *_end;
			num = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "pro")) {
			pro_str = argv[_i]->arg;
			char *_end;
			pro = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "sipv4")) {
			sipv4_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &sipv4);
		}
		if (!strcmp(argv[_i]->varname, "sipv6")) {
			sipv6_str = argv[_i]->arg;
			_fail = !inet_pton(AF_INET6, argv[_i]->arg, &sipv6);
		}
		if (!strcmp(argv[_i]->varname, "sport")) {
			sport_str = argv[_i]->arg;
			char *_end;
			sport = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
		if (!strcmp(argv[_i]->varname, "dipv4")) {
			dipv4_str = argv[_i]->arg;
			_fail = !inet_aton(argv[_i]->arg, &dipv4);
		}
		if (!strcmp(argv[_i]->varname, "dipv6")) {
			dipv6_str = argv[_i]->arg;
			_fail = !inet_pton(AF_INET6, argv[_i]->arg, &dipv6);
		}
		if (!strcmp(argv[_i]->varname, "dport")) {
			dport_str = argv[_i]->arg;
			char *_end;
			dport = strtol(argv[_i]->arg, &_end, 10);
			_fail = (_end == argv[_i]->arg) || (*_end != '\0');
		}
#if 1 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 1 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif
	if (!num_str) {
		vty_out(vty, "Internal CLI error [%s]\n", "num_str");
		return CMD_WARNING;
	}

	return debug_trace_add_magic(self, vty, argc, argv, num, num_str, pro, pro_str, sipv4, sipv4_str, sipv6, sipv6_str, sport, sport_str, dipv4, dipv4_str, dipv6, dipv6_str, dport, dport_str);
}

/* debug_ike_1 => "show ike <stats|list-sas|list-conns>" */
DEFUN_CMD_FUNC_DECL(debug_ike_1)
#define funcdecl_debug_ike_1 static int debug_ike_1_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)))
funcdecl_debug_ike_1;
DEFUN_CMD_FUNC_TEXT(debug_ike_1)
{
#if 0 /* anything to parse? */
	int _i;
#if 0 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 0 /* anything that can fail? */
		_fail = 0;
#endif

#if 0 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 0 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif

	return debug_ike_1_magic(self, vty, argc, argv);
}

/* debug_ike_2 => "ike <load-all|rekey|terminate|initiate> [ike IKENAME$ikename|child CHILDNAME$childname]" */
DEFUN_CMD_FUNC_DECL(debug_ike_2)
#define funcdecl_debug_ike_2 static int debug_ike_2_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	const char * ikename,\
	const char * childname)
funcdecl_debug_ike_2;
DEFUN_CMD_FUNC_TEXT(debug_ike_2)
{
#if 2 /* anything to parse? */
	int _i;
#if 0 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	const char *ikename = NULL;
	const char *childname = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 0 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "ikename")) {
			ikename = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
		if (!strcmp(argv[_i]->varname, "childname")) {
			childname = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
#if 0 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 0 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif

	return debug_ike_2_magic(self, vty, argc, argv, ikename, childname);
}

/* debug_ipsec => "ipsec <start | restart | stop | up CONNECTIONNAME$connectionname3 | down CONNECTIONNAME$connectionname4>" */
DEFUN_CMD_FUNC_DECL(debug_ipsec)
#define funcdecl_debug_ipsec static int debug_ipsec_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	const char * connectionname3,\
	const char * connectionname4)
funcdecl_debug_ipsec;
DEFUN_CMD_FUNC_TEXT(debug_ipsec)
{
#if 2 /* anything to parse? */
	int _i;
#if 0 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	const char *connectionname3 = NULL;
	const char *connectionname4 = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 0 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "connectionname3")) {
			connectionname3 = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
		if (!strcmp(argv[_i]->varname, "connectionname4")) {
			connectionname4 = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
#if 0 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 0 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif

	return debug_ipsec_magic(self, vty, argc, argv, connectionname3, connectionname4);
}

/* debug_ipsec1 => "show ipsec <version | status [CONNECTIONNAME$connectionname1] |statusall [CONNECTIONNAME$connectionname2]>" */
DEFUN_CMD_FUNC_DECL(debug_ipsec1)
#define funcdecl_debug_ipsec1 static int debug_ipsec1_magic(\
	const struct cmd_element *self __attribute__ ((unused)),\
	struct vty *vty __attribute__ ((unused)),\
	int argc __attribute__ ((unused)),\
	struct cmd_token *argv[] __attribute__ ((unused)),\
	const char * connectionname1,\
	const char * connectionname2)
funcdecl_debug_ipsec1;
DEFUN_CMD_FUNC_TEXT(debug_ipsec1)
{
#if 2 /* anything to parse? */
	int _i;
#if 0 /* anything that can fail? */
	unsigned _fail = 0, _failcnt = 0;
#endif
	const char *connectionname1 = NULL;
	const char *connectionname2 = NULL;

	for (_i = 0; _i < argc; _i++) {
		if (!argv[_i]->varname)
			continue;
#if 0 /* anything that can fail? */
		_fail = 0;
#endif

		if (!strcmp(argv[_i]->varname, "connectionname1")) {
			connectionname1 = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
		if (!strcmp(argv[_i]->varname, "connectionname2")) {
			connectionname2 = (argv[_i]->type == WORD_TKN) ? argv[_i]->text : argv[_i]->arg;
		}
#if 0 /* anything that can fail? */
		if (_fail)
			vty_out (vty, "%% invalid input for %s: %s\n",
				   argv[_i]->varname, argv[_i]->arg);
		_failcnt += _fail;
#endif
	}
#if 0 /* anything that can fail? */
	if (_failcnt)
		return CMD_WARNING;
#endif
#endif

	return debug_ipsec1_magic(self, vty, argc, argv, connectionname1, connectionname2);
}

