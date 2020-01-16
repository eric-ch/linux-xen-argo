#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "libargo.h"

int do_listen(unsigned long port)
{
    int rc, srv, cli;
    xen_argo_addr_t addr = {
        .aport = port,
        .domain_id = XEN_ARGO_DOMID_ANY,
        .pad = 0,
    };
    xen_argo_addr_t peer;
    char buf[1024] = { 0 };

    srv = argo_socket(SOCK_STREAM);
    if (srv < 0) {
        perror("socket");
        return -1;
    }

    rc = argo_bind(srv, &addr, XEN_ARGO_DOMID_ANY);
    if (rc != 0) {
        perror("bind");
        close(srv);
        return -1;
    }

    rc = argo_listen(srv, 4);
    if (rc != 0) {
        perror("listen");
        close(srv);
        return -1;
    }

    cli = argo_accept(srv, &peer);
    if (cli < 0) {
        perror("accept");
        close(srv);
        return -1;
    }

    do {
        rc = argo_recv(cli, buf, sizeof (buf), 0);
        if (rc < 0 && errno != EPIPE) {
            perror("read");
            break;
        } else if ((rc == 0) ||
		   ((rc < 0) && errno == EPIPE)) { // Not POSIX compliant (Read on Argo disconnected).
            fprintf(stderr, "Otherend closed.\n");
            break;
        } else
            write(STDOUT_FILENO, buf, rc);
    } while (1);

    close(cli);
    close(srv);
    return rc;
}

int do_connect(unsigned long domid, unsigned long port)
{
    int rc, fd;
    xen_argo_addr_t addr = {
        .aport = port,
        .domain_id = domid,
        .pad = 0,
    };
    char buf[1024] = { 0 };

    fd = argo_socket(SOCK_STREAM);
    if (fd < 0) {
        perror("socket");
        return -1;
    }
    rc = argo_connect(fd, &addr);
    if (rc != 0) {
        perror("connect");
        close(fd);
        return -1;
    }
    do {
        rc = read(STDIN_FILENO, buf, sizeof (buf));
        if (rc < 0) {
            perror("read");
            break;
        } else if (rc == 0) {
            fprintf(stderr, "Closing...\n");
            break;
        } else {
            rc = argo_send(fd, buf, rc, 0);
            if (rc < 0) {
                perror("send");
                break;
            }
        }
    } while (1);

    close(fd);
    return rc;
}

int main(int argc, char *argv[])
{
    int rc = 0, fd;
    unsigned long domid, port;

    switch (argc) {
        case 2:
            port = strtoul(argv[1], NULL, 0);
            rc = do_listen(port);
            break;
        case 3:
            domid = strtoul(argv[1], NULL, 0);
            port = strtoul(argv[2], NULL, 0);
            rc = do_connect(domid, port);
            break;
        default:
            fprintf(stderr, "%s DOMID PORT for server.\n");
            fprintf(stderr, "%s PORT for client.\n");
            break;
    }

    return rc;
}
