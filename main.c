#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>


#define ENTRY(x) {x, #x}
#define STRING_M(x) #x

/* cli arguments parse macro and functions */
#define NEXT_ARG() do { argv++; if (--argc <= 0) incomplete_command(); } while(0)
#define NEXT_ARG_OK() (argc - 1 > 0)
#define PREV_ARG() do { argv--; argc++; } while(0)

static char *argv0; /* ptr to the program name string */
static void usage(void)
{
    fprintf(stdout,
            "Usage:   %s [mode] [value]         \n"
            "            mode: ip | mac | help   \n"
            "\n"
            "Example: %s ip 192.168.1.1         \n"
            "         %s mac 00:ff:12:a3:e3     \n"
            "\n", argv0, argv0, argv0);
    exit(-1);
}

/* Returns true if 'prefix' is a not empty prefix of 'string'.
 */
static bool matches(const char *prefix, const char *string)
{
    if (!*prefix)
        return false;
    while (*string && *prefix == *string) {
        prefix++;
        string++;
    }

    return !*prefix;
}

static void incomplete_command(void)
{
    fprintf(stderr, "Command line is not complete. Try option \"help\"\n");
    exit(-1);
}

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
    const char *val_search = NULL;
    int mode = 0;

    /* cli arguments parse */
    argv0 = *argv; /* set program name */
    while (argc > 1) {
        NEXT_ARG();
        if (matches(*argv, "ip")){
            mode = 1;
            NEXT_ARG();
            val_search = *argv;
        } else if (matches(*argv, "mac")){
            mode = 2;
            NEXT_ARG();
            val_search = *argv;
        } else if (matches(*argv, "help")){
            usage();
        } else {
            usage();
            exit(1);
        }
        argc--; argv++;
    }

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
        /* skip broadcast entries */
        if(msg->ndm_state & NUD_NOARP){
            p += len;
            continue;
        }

        /*
         * Given the payload length, len, this macro returns the aligned
         * length to store in the nlmsg_len field of the nlmsghdr.
         */
        uint32_t msg_len = NLMSG_LENGTH(sizeof(*msg));
        len -= msg_len; /* count message length left */
        p += msg_len; /* move ptr forward */

        /* this is very first rtnetlink attribute */
        struct rtattr *rta = p;
        memset(tb, 0, sizeof(tb)); /* clear attribute buffer */
        parse_rtattr(tb, NDA_MAX, rta, len); /* fill tb attribute buffer */
#ifdef DEBUG
        memcpy(tb_, tb, sizeof(tb_)); /* for debug */
#endif
        char ip[INET6_ADDRSTRLEN] = {0};
        if (tb[NDA_DST]) { /* this is destination address */
            const char *ip_raw = RTA_DATA(tb[NDA_DST]);
            inet_ntop(msg->ndm_family, ip_raw, ip, INET6_ADDRSTRLEN);
        }

        char addr[18] = {0}; /*low level (hw) address string buffer */
        if (tb[NDA_LLADDR]) { /* this is hardware mac address */
            const uint8_t *addr_raw = RTA_DATA(tb[NDA_LLADDR]);
            sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x",
                    addr_raw[0], addr_raw[1], addr_raw[2], addr_raw[3], addr_raw[4], addr_raw[5]);
        }

        switch(mode) {
            case 1:
                if (strcmp(val_search, ip) == 0) {
                    fprintf(stdout, "%s laddr %s\n", ip, addr);
                    return 0;
                }
                break;
            case 2:
                if (strcmp(val_search, addr) == 0) {
                    fprintf(stdout, "%s lladdr %s\n", ip, addr);
                    return 0;
                }
                break;
            default:
                fprintf(stdout, "%s lladdr %s\n", ip, addr);
                break;
        }


        p += len; /* move ptr to the next netlink message */
    }

    return 0;
}