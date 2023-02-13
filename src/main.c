#include <stdio.h>
#include <openssl/rand.h>

#include "net/ws_conn.h"
#include "gateway/gateway.h"
#include "gateway/event.h"
#include "gateway/hello_data.h"

int main()
{
    struct gateway* g = gateway_open("help");

    if (g != NULL) {
        gateway_close(g);
    }
    
    return 0;
}