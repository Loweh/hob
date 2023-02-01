#include <stdio.h>
#include "net/https_conn.h"

int main()
{
    struct https_conn* conn = https_conn_init("wss://gateway.discord.gg", "443");
    
    if (!https_conn_open(conn)) {
        char* buf = "Hello, world!";
        char buf2[4096] = {0};
        SSL_write(conn->ssl, buf, 13);

        while (SSL_read(conn->ssl, buf2, 4096) <= 0) {}
        printf("buf: %s\n", buf2);

        https_conn_close(conn);
    } else {
        https_conn_close(conn);
    }
    
    return 0;
}