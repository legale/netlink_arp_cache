#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>


#define ENTRY(x) {x, #x}
#define STRING_M(x) #x

static int parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len) {
    while (RTA_OK(rta, len) && max--) {
        //fprintf(stderr, "len: %i, rta_type: %i, rta_len: %i\n", len, rta->rta_type, rta->rta_len);
        tb[rta->rta_type] = rta;
        rta = RTA_NEXT(rta, len);
    };
    return 0;
}

int main(int argc, char **argv) {
    int status;
    void *p; //just a ptr

    //open socket
    int sd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);

    //arp cache request
    struct {
        struct nlmsghdr n;
        struct ndmsg ndm;
        char buf[1024];
    } req = {
            .n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg)),
            .n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT, // to get all table instead of a single entry
            .n.nlmsg_type = RTM_GETNEIGH, //to get arp cache
            .ndm.ndm_family = AF_INET, // IP protocol family. AF_INET/AF_INET6
    };

    //send request
    status = send(sd, &req, req.n.nlmsg_len, 0);
//    fprintf(stderr, "send status: %i\n", status);

    //this is buffer
    char buf[16384] = {0};

    //recv answer
    status = recv(sd, buf, sizeof(buf), 0);
//    fprintf(stderr, "recv status: %i\n", status);
    int buf_size = status;
    p = (void *) buf;

    while (buf_size > 0) {
        struct nlmsghdr *answer = (struct nlmsghdr *) p;

        int len = answer->nlmsg_len;
        struct ndmsg *msg = NLMSG_DATA(answer);
        int msg_len = NLMSG_LENGTH(sizeof(*msg));
        len -= msg_len;
        p += msg_len;
        /*fprintf(stderr, "answer ptr: %u, msg ptr: %u diff: %u, nl msg len: %i\n",
                &answer, &msg, (void *) &answer - (void *) &msg, len);*/

        //rtnetlink route netlink attributes buffer
        struct rtattr *tb[NDA_MAX + 1] = {0};
        //fprintf(stderr, "size tb: %i\n", sizeof(tb));


        //this is first rtnetlink attribute
        struct rtattr *rta = (struct rtattr *) p;
        memset(tb, 0, sizeof(tb));
        parse_rtattr(tb, NDA_MAX, rta, len);
        if (tb[NDA_DST]) {
            char ip[INET6_ADDRSTRLEN] = {0};
            inet_ntop(msg->ndm_family, RTA_DATA(tb[NDA_DST]), ip, INET6_ADDRSTRLEN);
            fprintf(stderr, "%s ", ip);
        }
            if (tb[NDA_LLADDR]) {
            const unsigned char *addr = RTA_DATA(tb[NDA_LLADDR]);
                fprintf(stderr, "lladdr: %02X:%02X:%02X:%02X:%02X:%02X\n",
                        addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
            }else {
                fprintf(stderr, "lladdr: \n");
            }

        p += len;
        buf_size -= answer->nlmsg_len;
    }

    return 0;
}