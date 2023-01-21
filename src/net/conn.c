#include "conn.h"

struct conn* conn_init(char* hostname, char* port, char* path)
{
    struct conn* c = (struct conn*) malloc(sizeof(struct conn));
    c->state = CONN_CLOSED;
    c->hostname = hostname;
    c->port = port;
    c->path = path;
    c->last_ping = 0;

    SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
    int error = 0;

    if (ctx != NULL) {
        c->ssl = SSL_new(ctx);

        if (c->ssl != NULL) {
            c->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            error = SSL_set_fd(c->ssl, c->socket) == 1 ? 0 : 1;
        } else {
            error = 1;
        }
    } else {
        error = 1;
    }

    if (error) {
        free(c);
        c = NULL;
    }

    return c;
}

int conn_open(struct conn* c)
{
    int result = 0;

    struct addrinfo* h_info = NULL;

    if (!getaddrinfo(c->hostname, c->port, 0, &h_info)) {
        if (!connect(c->socket, h_info->ai_addr, sizeof(*h_info->ai_addr))) {
            if (SSL_connect(c->ssl) != -1) {
                result = SSL_do_handshake(c->ssl) == 1 ? 0 : -4;
                c->state = !result ? CONN_OPEN : CONN_CLOSED;
            } else {
                // SSL_connect seems to shutdown itself on error so
                // no need to shutdown here
                result = -3;
            }
        } else {
            result = -2;
        }
    } else {
        result = -1;
    }

    return result;
}

void conn_close(struct conn** c)
{
    if ((*c)->state == CONN_OPEN) {
        SSL_shutdown((*c)->ssl);
        close((*c)->socket);
        free(*c);
        *c = NULL;
    }
}

int conn_handshake(struct conn* c)
{
    int result = 0;

    if (c->state == CONN_OPEN) {
        char* rq = NULL;
        unsigned char key[25] = {0};
        int rq_sz = _conn_assemble_handshake(c, &rq, key);
        SSL_write(c->ssl, rq, rq_sz);
        free(rq);

        // Adjust this buffer size?
        char buf[2048] = {0};
        SSL_read(c->ssl, buf, 2048);
    } else {
        result = -1;
    }

    return result;
}

int _conn_assemble_handshake(struct conn* c, char** r, unsigned char* key)
{
    // Generate the base64 key
    unsigned char bytes[CONN_WS_KEY_SZ] = {0};

    if (RAND_bytes(bytes, CONN_WS_KEY_SZ) != 1) {
        return -1;
    }

    int key_sz = EVP_EncodeBlock(key, bytes, CONN_WS_KEY_SZ);

    // Determine all the pieces of the HTTP request and their lengths
    // Maybe worth looking at HTTP 2?
    char* hdr1 = "GET ";
    char* hdr2 = " HTTP/1.1\r\n"
                "Host: ";
    char* hdr3 = "\r\n"
                "Connection: Upgrade\r\n"
                "Upgrade: websocket\r\n"
                "Sec-WebSocket-Version: 13\r\n"
                "Sec-WebSocket-Key: ";
    char* hdr4 = "\r\n\r\n";

    char* segments[] = {hdr1, c->path, hdr2, c->hostname, hdr3, key, hdr4};
    int sizes[] = {
        strlen(hdr1),
        strlen(c->path),
        strlen(hdr2), 
        strlen(c->hostname),
        strlen(hdr3),
        key_sz,
        strlen(hdr4)
    };
    int seg_num = 7;

    // Get the full size of the request and allocate memory for it
    int rq_sz = 0;
    
    for (int i = 0; i < seg_num; i++) {
        rq_sz += sizes[i];
    }
    
    *r = (char*) malloc(rq_sz);
    char* rq = *r;
    int offset = 0;

    // Copy the segments into the request buffer
    for (int i = 0; i < seg_num; i++) {
        memcpy(rq + offset, segments[i], sizes[i]);
        offset += sizes[i];
    }

    // Add a null terminator at the end
    rq[offset] = 0;

    return offset;
}

// Refactor this method (try to eliminate magic numbers where possible)
int _conn_check_handshake_response(
    char* rs,
    int rs_len,
    unsigned char* key,
    int key_len)
{
    int result = 0;

    // Make sure the proper HTTP response was received.
    char* expected = "HTTP/1.1 101 Switching Protocols\r\n";
    int exp_sz = strlen(expected);

    if (!strncmp(expected, rs, exp_sz)) {
        // Make sure all the necessary header fields are present
        // TODO: Make case insensitive
        char* connection = strstr(rs, "Connection: upgrade\r\n");
        char* upgrade = strstr(rs, "Upgrade: websocket\r\n");
        char* accept = strstr(rs, "Sec-WebSocket-Accept: ");
        int accept_len = strlen("Sec-WebSocket-Accept: ");

        if (connection != NULL && upgrade != NULL && accept != NULL) {
            // Pull the accept key from the Sec-WebSocket-Accept header
            unsigned char accept_key[32] = {0};
            char* start = accept + accept_len;

            for (char* cur = start; *cur != '\r' && cur - start < 32; cur++) {
                accept_key[cur - start] = *cur;
            }

            // Get the hash from the accept key by doing a base64 decode
            unsigned char hash[32] = {0};
            int hash_len = EVP_DecodeBlock(hash, accept_key, 32);
            
            // Take the original key generated in _conn_assemble_handshake and
            // perform an SHA1 hash on its concatenation with the below magic
            // string (part of the WebSocket protocol).
            unsigned char exp[64] = {0};
            memcpy(exp, key, key_len);
            memcpy(exp + key_len - 1, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);

            // TODO: Get rid of these deprecated calls and get EVP_Digest
            // working instead.
            SHA_CTX hash_ctx;
            SHA1_Init(&hash_ctx);
            SHA1_Update(&hash_ctx, exp, 60);
            unsigned char end[32] = {0};
            SHA1_Final(end, &hash_ctx);
            
            // Make sure the hashed key and the given hash by the server are
            // the same.
            if (strncmp((char*) hash, (char*) end, hash_len)) {
                result = -3;
            }
        } else {
            result = -2;
        }
    } else {
        result = -1;
    }

    return result;
}