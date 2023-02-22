#include <stdio.h>
#include <openssl/rand.h>

#include "net/ws_conn.h"
#include "gateway/gateway.h"
#include "gateway/event.h"
#include "gateway/hello_data.h"

int main()
{
    FILE* token_fd = fopen("./token", "r+");

    if (token_fd == NULL) {
        printf("Could not locate token file.\n");
        return -1;
    }

    fseek(token_fd, 0, SEEK_END);
    int sz = ftell(token_fd);
    fseek(token_fd, 0, SEEK_SET);

    char* token = (char*) malloc(sz + 1);
    fread(token, sz, 1, token_fd);
    token[sz] = 0;

    printf("%s\n", token);

    fclose(token_fd);

    struct gateway* g = gateway_open(token);

    if (g != NULL) {
        gateway_close(g);
    }

    free(token);
    
    return 0;
}