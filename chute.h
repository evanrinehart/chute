
struct packedItem {
    long item;
    long count;
    struct packedItem * next;
};

struct chunk {
    long left;
    long right;
    struct packedItem * items;
    struct chunk * next;
};

struct chute {

    long length; // in units
    long speed;  // units per tick
    long slow_speed;  // units per tick

    struct chunk * free_chunks;
    struct chunk * final_chunk;

};
