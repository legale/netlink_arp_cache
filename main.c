#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>


#define ENTRY(x) {x, #x}
#define STRING_M(x) #x

/* rtnetlink route netlink attributes buffer */
static struct rtattr *tb[NDA_MAX + 1];

static void parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len) {
    /* loop over all rtattributes */
    while (RTA_OK(rta, len) && max--) {
        tb[rta->rta_type] = rta; /* store attribute ptr to the tb array */
        rta = RTA_NEXT(rta, len); /* special rtnetlink.h macro to get next netlink route attribute ptr */
    };
}

int main(int argc, char **argv) {
    int status;
    void *p, *lp; //just a ptrs

    /* open socket */
    int sd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);

    /* contruct arp cache request */
    struct {
        struct nlmsghdr n;
        struct ndmsg ndm;
        char buf[0];
    } req = {
            .n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg)),
            .n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT, /* to get all table instead of a single entry */
            .n.nlmsg_type = RTM_GETNEIGH, /* to get arp cache */
            //.ndm.ndm_family = AF_INET, /* IP protocol family. AF_INET/AF_INET6 */
    };

    /* send request */
    status = send(sd, &req, req.n.nlmsg_len, 0);

    /* this is buffer to store an answer */
    char buf[16384] = {0};

    /* get an answer */
    status = recv(sd, buf, sizeof(buf), 0);
    int buf_size = status; /* recv will return answer size */
    p = buf; /* set p to start of an answer */
    lp = buf + buf_size; /* answer last byte ptr */
#ifdef DEBUG
    struct rtattr *tb_[NDA_MAX + 1]; /* for debug */
#endif
    while (p < lp) { /* loop till the end */
        struct nlmsghdr *answer = p; /* netlink header structure */
        uint32_t len = answer->nlmsg_len; /* netlink message length including header */

        struct ndmsg *msg = NLMSG_DATA(answer); /* macro to get a ptr right after header */
        /*
         * Given the payload length, len, this macro returns the aligned
         * length to store in the nlmsg_len field of the nlmsghdr.
         */
        uint32_t msg_len = NLMSG_LENGTH(sizeof(*msg));
        len -= msg_len; /* count message length left */
        p += msg_len; /* move ptr forward */

        /* skip broadcast entries */
        if(msg->ndm_state & NUD_NOARP){
            p += len;
            continue;
        }

        /* this is very first rtnetlink attribute */
        struct rtattr *rta = p;
        memset(tb, 0, sizeof(tb)); /* clear attribute buffer */
        parse_rtattr(tb, NDA_MAX, rta, len); /* fill tb attribute buffer */
#ifdef DEBUG
        memcpy(tb_, tb, sizeof(tb_)); /* for debug */
#endif
        if (tb[NDA_DST]) { /* this is destination address */
            const unsigned char *ip_raw = RTA_DATA(tb[NDA_DST]);
            char ip[INET6_ADDRSTRLEN] = {0};
            inet_ntop(msg->ndm_family, ip_raw, ip, INET6_ADDRSTRLEN);
            fprintf(stdout, "%s", ip);
        }
        if (tb[NDA_LLADDR]) { /* this is hardware mac address */
            const unsigned char *addr = RTA_DATA(tb[NDA_LLADDR]);
            fprintf(stdout, " lladdr: %02X:%02X:%02X:%02X:%02X:%02X\n",
                    addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
        } else {
            fprintf(stdout, " lladdr: \n");
        }

        p += len; /* move ptr to the next netlink message */
    }

    return 0;
}