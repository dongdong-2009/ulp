#lwip-git
inc-$(CONFIG_LWIP_GIT) += port-nosys/
inc-$(CONFIG_LWIP_GIT) += lwip/src/include/
inc-$(CONFIG_LWIP_GIT) += lwip/src/include/ipv4/
inc-$(CONFIG_LWIP_GIT) += lwip/src/include/ipv6/

obj-$(CONFIG_LWIP_GIT) += lwip/src/core/def.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/dhcp.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/dns.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/inet_chksum.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/init.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/mem.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/memp.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/netif.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/pbuf.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/raw.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/stats.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/sys.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/tcp.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/tcp_in.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/tcp_out.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/timers.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/udp.o

obj-$(CONFIG_LWIP_GIT) += lwip/src/core/ipv4/autoip.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/ipv4/icmp.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/ipv4/igmp.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/ipv4/ip4.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/ipv4/ip4_addr.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/core/ipv4/ip_frag.o

obj-$(CONFIG_LWIP_GIT) += lwip/src/netif/etharp.o
obj-$(CONFIG_LWIP_GIT) += lwip/src/netif/slipif.o

#apps
obj-$(CONFIG_LWIP_APP_HELLO) += lwip_app_hello.o
obj-$(CONFIG_LWIP_APP_TCPSERVER) += lwip_app_tcpserver.o

#app - httpd
obj-$(CONFIG_LWIP_APP_HTTPD) += lwip-contrib/apps/httpserver_raw/httpd.o
obj-$(CONFIG_LWIP_APP_HTTPD) += lwip-contrib/apps/httpserver_raw/fs.o

#lwip-lib
obj-y += lwip_lib.o
obj-$(CONFIG_LWIP_V131) += lwip-1.3.1/
