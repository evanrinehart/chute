#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>


/* 1 yard = 256 inches
   1 inch = 60 microns

   1 second = 60 ticks
   1 tick   = 480 subticks

   speed measured in inches per tick times subticks / 8 = distance in microns

    distance in microns times 480 / speed = time in subticks

*/




struct packedItem {
    long item;
    long count;
    struct packedItem * next;
};

// pop an item from the end of a non empty packed item list, return the updated list (might be NULL)
struct packedItem * popPackedItemList(struct packedItem * list, long * popped) {
    struct packedItem * prev = NULL;
    struct packedItem * ptr  = list;

    while(ptr->next){
        prev = ptr;
        ptr  = ptr->next;
    }

    if(ptr->count > 1) {
        *popped = ptr->item;
        ptr->count--;
        return list;
    }
    else if(prev) {
        *popped = ptr->item;
        prev->next = NULL;
        free(ptr);
        return list;
    }
    else {
        *popped = ptr->item;
        free(ptr);
        return NULL;
    }
    
}

// merge two non-empty packed item lists into 1
struct packedItem * mergePackedItemLists(struct packedItem * list1, struct packedItem * list2) {
    struct packedItem * ptr = list1;
    while(ptr->next) ptr = ptr->next;

    if(ptr->item == list2->item) { // same item between
        long count = ptr->count + list2->count;
        ptr->next = list2->next;
        free(list2);
        ptr->count = count;
        return list1;
    }
    else {
        ptr->next = list2;
        return list1;
    }
}


struct chunk {
    long left;
    long right;
    struct packedItem * items;
    struct chunk * next;
};

// return the merge of two (isolated) chunks, consumes the input chunks
struct chunk * mergeChunks(struct chunk * c1, struct chunk * c2) {
    long left  = c1->left;
    long right = c2->right;
    c1->left  = left;
    c1->right = right;
    c1->items = mergePackedItemLists(c1->items, c2->items);
    c1->next  = NULL;
    free(c2);
    return c1;
}

// pop a chunk from a list of chunks, return the updated list (may be NULL)
struct chunk * popChunkList(struct chunk * list, struct chunk ** popped) {
    struct chunk * prev = NULL;
    struct chunk * ptr  = list;

    while(ptr->next){
        prev = ptr;
        ptr  = ptr->next;
    }

    *popped = ptr;

    if(prev) {
        prev->next = NULL;
        return list;
    }
    else {
        return NULL;
    }
    
}

// get last chunk from a chunk list, NULL if empty
struct chunk * getLastChunk(struct chunk * list){
    if(list == NULL) return NULL;
    struct chunk * ptr = list;
    while(ptr->next) ptr = ptr->next;
    return ptr;
}


/* chute */

struct chute {

    long length; // in units
    long speed;  // units per tick
    long slow_speed;  // units per tick

    struct chunk * free_chunks;
    struct chunk * final_chunk;

};


// final chunk moves 1 item off the end of the chute
long timeUntilChuteEvent1(struct chute * ch){
    struct chunk * final = ch->final_chunk;
    if(final && ch->slow_speed > 0) return (ch->length + 256*60 - final->right) * 8 / ch->slow_speed;
    else return LONG_MAX;
}

// free chunk reaches end of chute, will become the final chunk
long timeUntilChuteEvent2(struct chute * ch){
    struct chunk * last = getLastChunk(ch->free_chunks);
    if(last) return (ch->length - last->right) * 8 / ch->speed;
    else return LONG_MAX;
}

// last free chunk collides with final chunk
long timeUntilChuteEvent3(struct chute * ch){
    struct chunk * last = getLastChunk(ch->free_chunks);
    struct chunk * final = ch->final_chunk;
    long relative_speed = ch->speed - ch->slow_speed;
    if(last && final && relative_speed > 0){
        return (final->left - last->right) * 8 / relative_speed;
    }
    else{
        return LONG_MAX;
    }
}

// chute entry becomes clear
long timeUntilChuteEvent4(struct chute * ch){
    struct chunk * first = ch->free_chunks;
    struct chunk * final = ch->final_chunk;
    if(first && first->left < 256*60){
        return (256*60 - first->left) * 8 / ch->speed;
    }
    else if(final && final->left < 256*60 && ch->slow_speed > 0){
        return (256*60 - final->left) * 8 / ch->slow_speed;
    }
    else{
        return LONG_MAX;
    }
}

long timeUntilChuteEvent(struct chute * ch) {
    long e1 = timeUntilChuteEvent1(ch);
    long e2 = timeUntilChuteEvent2(ch);
    long e3 = timeUntilChuteEvent3(ch);
    long e4 = timeUntilChuteEvent4(ch);
    long min = LONG_MAX;
    if(e1 < min) min = e1;
    if(e2 < min) min = e2;
    if(e3 < min) min = e3;
    if(e4 < min) min = e4;
    return min;
}

long timeUntilChuteEventDebug(struct chute * ch) {
    long e1 = timeUntilChuteEvent1(ch);
    long e2 = timeUntilChuteEvent2(ch);
    long e3 = timeUntilChuteEvent3(ch);
    long e4 = timeUntilChuteEvent4(ch);
    printf("e1 = %ld\n", e1);
    printf("e2 = %ld\n", e2);
    printf("e3 = %ld\n", e3);
    printf("e4 = %ld\n", e4);
    long min = LONG_MAX;
    if(e1 < min) min = e1;
    if(e2 < min) min = e2;
    if(e3 < min) min = e3;
    if(e4 < min) min = e4;
    return min;
}

void chuteAction(struct chute * ch, int * entry_clear, long * item_exit) {

    struct chunk * final  = ch->final_chunk;
    struct chunk * last   = getLastChunk(ch->free_chunks);
    struct chunk * first  = ch->free_chunks;

    int e1 = final && final->right == ch->length + 256*60;
    int e2 = last  && final && last->right == final->left;
    int e3 = last  && last->right  == ch->length;
    int e4 = first && first->left  == 256*60;
    int e5 = final && final->left  == 256*60 && ch->slow_speed > 0;

    *item_exit = 0;

    // entry clear
    *entry_clear = e4 || e5;

    // edge case - confluence of 3 difference events at once
    if(e1 && e2 && e3) {

        ch->free_chunks = popChunkList(ch->free_chunks, &last);
        ch->final_chunk = mergeChunks(last, ch->final_chunk);
        ch->final_chunk->items = popPackedItemList(ch->final_chunk->items, item_exit);
        ch->final_chunk->right -= 256*60;

    }
    else{

        // item exits chute, possible nulls out final chunk
        if(e1) {
            final->items = popPackedItemList(final->items, item_exit);
            if(final->items == NULL) {
                free(ch->final_chunk);
                ch->final_chunk = NULL;
            }
            else {
                ch->final_chunk->right -= 256*60;
            }
        }

        // e1 and e2 might both happen here, invalidating local variables

        // last free_chunk collides and merges with final chunk
        if(e2) {
            ch->free_chunks = popChunkList(ch->free_chunks, &last);
            ch->final_chunk = mergeChunks(last, final);
        }

        // last free_chunk reaches end of chute and becomes final (assert final == NULL)
        if(e3) {
            ch->free_chunks = popChunkList(ch->free_chunks, &last);
            ch->final_chunk = last;
        }

    }

}


void chuteAdvanceTime(struct chute * ch, long delta) {

    long travel = delta * ch->speed / 8; // sub units (60 sub units per unit)

    for(struct chunk * ptr = ch->free_chunks; ptr; ptr = ptr->next) {
        ptr->left  += travel;
        ptr->right += travel;
    }

    struct chunk * final = ch->final_chunk;
    if(final){
        long slow_travel = delta * ch->slow_speed / 8;
        final->left  += slow_travel;
        final->right += slow_travel;
    }

}



struct packedItem * packItems(long item, long count, struct packedItem * next) {
    struct packedItem * pil = malloc(sizeof *pil);
    pil->item  = item;
    pil->count = count;
    pil->next  = next;
    return pil;
}

struct chunk * addChunk(long left, long right, struct packedItem * items, struct chunk * next) {
    struct chunk * chunk = malloc(sizeof *chunk);
    chunk->left  = left;
    chunk->right = right;
    chunk->items = items;
    chunk->next  = next;
    return chunk;
}


void makeTestChute(struct chute * chute) {

    long yard = 256 * 60;

    chute->length = 10 * yard;
    chute->speed  = 8;
    chute->slow_speed = 0;
    chute->free_chunks =
        addChunk(yard/2, yard/2 + 6*yard, packItems(7,3,packItems(6,3,NULL)), NULL);
    chute->final_chunk = NULL;

}

void debugChute(struct chute * chute) {
    long yard = 256*60;
    for(struct chunk * ptr = chute->free_chunks; ptr; ptr = ptr->next) {
        printf("struct chunk:\n");
        printf("  left  = %ld:%ld\n", ptr->left / yard, ptr->left % yard);
        printf("  right = %ld:%ld\n", ptr->right / yard, ptr->right % yard);
        printf("  items:\n");
        for(struct packedItem * pi = ptr->items; pi; pi = pi->next) {
            printf("    {item=%ld, count=%ld}\n", pi->item, pi->count);
        }
    }
}

void test() {
    struct chute ch;
    makeTestChute(&ch);

    debugChute(&ch);

    long eta1 = timeUntilChuteEvent1(&ch);
    long eta2 = timeUntilChuteEvent2(&ch);
    long eta3 = timeUntilChuteEvent3(&ch);
    long eta4 = timeUntilChuteEvent4(&ch);

    printf("eta1 = %ld\n", eta1);
    printf("eta2 = %ld\n", eta2);
    printf("eta3 = %ld\n", eta3);
    printf("eta4 = %ld\n", eta4);

    chuteAdvanceTime(&ch, 1);
    debugChute(&ch);

    eta1 = timeUntilChuteEvent1(&ch);
    eta2 = timeUntilChuteEvent2(&ch);
    eta3 = timeUntilChuteEvent3(&ch);
    eta4 = timeUntilChuteEvent4(&ch);

    printf("eta1 = %ld\n", eta1);
    printf("eta2 = %ld\n", eta2);
    printf("eta3 = %ld\n", eta3);
    printf("eta4 = %ld\n", eta4);
    
}


