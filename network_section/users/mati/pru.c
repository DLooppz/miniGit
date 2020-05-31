enum Commandos {
    Login,
    Logout,
    Response,
    File,
};

enum ResponseValues {
    OK = 200,
    ErrorA = 400,
    ErrorB = 401,
    NotFound = 404,
}

struct Header __attribute__((packed)) {
    uint16_t command;   // <<<< enum Commands
    uint16_t lenght;    // Longitud de todo el paquete
}

struct LoginArgs __attribute__((packed)) {
    char username[64];
    char password[64];
};

struct ResponseArgs __attribute__((packed)) {
    uint16_t value;   // <<<<<< enum ResponseValues
};

struct FileArgs __attribute__((packed)) {
    char name[128];
    uint32_t totalLength;
};

struct File2SocksArgs __attribute__((packed)) {
    char name[128];
    uint32_t totalLength;
};

struct BlockArgs __attribute__((packed)) {
    uint32_t blockLength;
    char block[1024];
};


struct Packet __attribute__((packed)) {

    struct Header header;

    union Payload {

        struct LoginArgs loginArgs;

        struct ResponseArgs responseArgs;

        struct BlaBlaArgs blablaArgs;

        struct FileArgs fileArgs;

        struct BlockArgs blockArgs;


    } payload;
};

void setLogin(struct Packet *p, const char *user, const char*pass) {
    p->header.command = Login;
    p->header.length = sizeof(struct LoginArgs) + sizeof(struct Header);
    strcpy(p->payload.loginArgs.username, user);
    strcpy(p->payload.loginArgs.password, pass);
}

void setRespone(struct Packet *p, ResponseValues val) {
    p->header.command = Response;
    p->header.length = sizeof(struct ResponseArgs) + sizeof(struct Header);
    p->payload.responseArgs.value = val;
}

int main() {
    struct Packet packet;
    
    setLogin(&packet, "user", "pass");
    sendPacket(sockfd, &packet);

}

int sendPacket(int socket, const struct Packet *packet)
{
    int sent;
    int toSend = packet->lenght;

    const uint8_t *ptr = (const uint8_t *) packet;

    while( toSend ) {
        sent = send(socket, ptr, toSend, 0);
        if( sent == 0 )
            return 0;
        if( sent == -1 ) {
            if( errno == EINTR )
                continue;
            return -1;
        }
        toSend -= sent;
        ptr += sent;
    }
}

int sendPacket(int socket, const struct Packet *packet)
{
    int sent;
    int toSend = packet->lenght;

    const uint8_t *ptr = (const uint8_t *) packet;

    while( toSend ) {
        sent = send(socket, ptr, toSend, 0);
        if( sent == 0 )
            return 0;
        if( sent == -1 ) {
            if( errno == EINTR )
                continue;
            return -1;
        }
        toSend -= sent;
        ptr += sent;
    }
}

// packet.h
//     structs
//     int sendPacket(int socket, const struct Packet *packet);
//     int recvPacket(int socket, struct Packet *packet);

// packet.c
// int sendPacket(int socket, const struct Packet *packet) {
//
// }
// int recvPacket(int socket, struct Packet *packet) {
//
// }


// gcc client.c  packet.c  -o client.exe
// gcc server.c  packet.c  -o server.exe




