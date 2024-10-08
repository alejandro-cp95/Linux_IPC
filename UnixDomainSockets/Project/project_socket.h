#include <array>

using namespace std;

#define SOCKET_NAME "/tmp/project_socket"

struct ArrayHasher {
    std::size_t operator()(const std::array<char, 17>& a) const {
        std::size_t h = 0;

        for (auto e : a) {
            h ^= std::hash<int>{}(e)  + 0x9e3779b9 + (h << 6) + (h >> 2); 
        }
        return h;
    }   
};

enum class OPCODE
{
    CREATE,
    UPDATE,
    DELETE,
    CREATE_ERROR,
    UPDATE_ERROR,
    DELETE_ERROR,
    DUMP_START,
    DUMP_END
};

typedef struct _msg_body
{
    array<char, 15> destination;
    char mask;
    array<char, 15> gateway_ip;
    array<char, 32> output_interface;
} msg_body_t;

typedef struct _sync_msg
{
    OPCODE op_code;
    msg_body_t msg_body;
} sync_msg_t;