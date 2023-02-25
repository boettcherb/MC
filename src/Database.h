#ifndef DATABASE_H_INCLUDED
#define DATABASE_H_INCLUDED

namespace database {

    inline constexpr int QUERY_NONE = 0;
    inline constexpr int QUERY_LOAD = 1;
    inline constexpr int QUERY_STORE = 2;

    struct Query {
        int type;
        int x, z;
        int size;
        const void* data;
    };

    void initialize();
    void close();
    void request_load(int x, int z);
    void request_store(int x, int z, int size, const void* data);
    Query get_load_result();

}

#endif