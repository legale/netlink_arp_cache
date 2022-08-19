#include "libnlarpcache.h"

void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, unsigned len) {
    /* loop over all rtattributes */
    while (RTA_OK(rta, len) && max--) {
        tb[rta->rta_type] = rta; /* store attribute ptr to the tb array */
        rta = RTA_NEXT(rta, len); /* special rtnetlink.h macro to get next netlink route attribute ptr */
    }
}

/* warning malloc is used! free(recv_buf) REQUIRED!!!
 * return recv data size received */
ssize_t send_recv(const void *send_buf, size_t send_buf_len, void ***recv_buf){
    void *buf; /* local ptr to a buffer */
    *recv_buf = &buf; /* now external buffer ptr points to local buffer */

    int64_t status; /* to store send() recv() return value  */
    int sd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE); /* open socket */

    /* send message */
    status = send(sd, send_buf, send_buf_len, 0);
    if (status < 0) fprintf(stderr, FILELINE " error: send %ld %d\n", status, errno);

    /* get an answer */
    /*first we need to find out buffer size needed */
    ssize_t expected_buf_size = 512; /* initial buffer size */
    ssize_t buf_size = 0; /* real buffer size */
    buf = malloc( expected_buf_size ); /* alloc memory for a buffer */
    /*
     * MSG_TRUNC will return data size even if buffer is smaller than data
     * MSG_PEEK will receive queue without removing that data from the queue.
     * Thus, a subsequent receive call will return the same data.
     */
    status = recv(sd, buf, expected_buf_size, MSG_TRUNC | MSG_PEEK);
    if (status < 0) fprintf(stderr, FILELINE " error: recv %ld %d\n", status, errno);

    if (status > expected_buf_size){
        expected_buf_size = status; /* this is real size */
        buf = realloc(buf, expected_buf_size); /* increase buffer size */

        status = recv(sd, buf, expected_buf_size, 0); /* now we get the full message */
        buf_size = status; /* save real buffer bsize */
        if (status < 0) fprintf(stderr, FILELINE " error: recv %ld %d\n", status, errno);
    }

    status = close(sd); /* close socket */
    if (status < 0) fprintf(stderr, FILELINE " error: recv %ld %d\n", status, errno);

    return buf_size;
}

/* malloc is used! don't forget free(buf) */
ssize_t get_arp_cache(void ***buf_ptr) {
    /* construct arp cache request */
    struct {
        struct nlmsghdr n;
        struct ndmsg ndm;
        char buf[0];
    } req = {
            .n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg)),
            .n.nlmsg_type = RTM_GETNEIGH, /* to get arp cache */
            .n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT, /* to get all table instead of a single entry */
            //.ndm.ndm_family = AF_INET, /* IP protocol family. AF_INET/AF_INET6 or both if not set */
    };
    ssize_t status = send_recv(&req, req.n.nlmsg_len, buf_ptr);

    return status;
}

/* parse arp cache netlink messages */
ssize_t parse_arp_cache(void *buf, ssize_t buf_size, arp_cache cache[]) {
    void *p, *lp; /* just a ptrs to work with netlink answer data */
    lp = buf + buf_size; /* answer last byte ptr */
    p = buf; /* set p to start of an answer */
    int cnt = 0; /* nl message counter */

    for (int i = 0; p < lp; i++) { /* loop till the end */
        struct nlmsghdr *hdr = p; /* netlink header structure */
        uint32_t len = hdr->nlmsg_len; /* netlink message length including header */
        struct ndmsg *msg = NLMSG_DATA(hdr); /* macro to get a ptr right after header */

        cache[i].nl_hdr = hdr;
        cache[i].ndm_family = msg->ndm_family;
        cache[i].ndm_state = msg->ndm_state;
        /*
         * Given the payload length, len, this macro returns the aligned
         * length to store in the nlmsg_len field of the nlmsghdr.
         */
        uint32_t msg_len = NLMSG_LENGTH(sizeof(*msg));
        len -= msg_len; /* count message length left */
        p += msg_len; /* move ptr forward */

        /* this is very first rtnetlink attribute */
        struct rtattr *rta = p;
        parse_rtattr(cache[i].tb, NDA_MAX, rta, len); /* fill tb attribute buffer */

        p += len; /* move ptr to the next netlink message */
        cnt++;
    }
    return p == lp ? cnt : -1;
}
