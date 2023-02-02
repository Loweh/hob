#include <stdio.h>
#include "net/https_conn.h"
#include "net/https_req.h"

int main()
{
    struct https_req* rq = https_req_init(HTTPS_GET, "/", "aaaaaa");
    https_req_add_hdr(rq, "Host", "example.com");
    https_req_add_hdr(rq, "Content-Type", "application/json");

    char* buf = NULL;
    int buf_sz = https_req_serialize(rq, &buf);
    printf("buf: %s (%i)\n", buf, buf_sz);
    https_req_free(rq);
    free(buf);
    /*
    struct https_conn* conn = https_conn_init("wss://gateway.discord.gg", "443");
    int result = https_conn_open(conn);
    if (!result) {
        char* buf = "Hello, world!";
        char buf2[4096] = {0};
        SSL_write(conn->ssl, buf, 13);

        while (SSL_read(conn->ssl, buf2, 4096) <= 0) {}
        printf("buf: %s\n", buf2);

        https_conn_close(conn);
    } else {
        printf("%i\n", result);
        https_conn_close(conn);
    }
    */
    return 0;
}