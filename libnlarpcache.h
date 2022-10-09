#ifndef _LIB_NL_ARP_CACHE
#define _LIB_NL_ARP_CACHE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <unistd.h> /* close() */


/* structure to store arp entries */
typedef struct _arp_cache {
    struct nlmsghdr *nl_hdr;
    struct rtattr *tb[NDA_MAX + 1];
    uint8_t ndm_family;
    uint16_t ndm_state; /* ndmsg structure variable */
} arp_cache;


void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, unsigned len);
ssize_t send_recv(const void *send_buf, size_t send_buf_len, void **recv_buf);
ssize_t get_arp_cache(void **buf_ptr);
ssize_t parse_arp_cache(void *buf, ssize_t buf_size, arp_cache cache[]);

#endif /* _LIB_NL_ARP_CACHE */