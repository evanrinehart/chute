#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <raylib.h>
#include <rlgl.h>

#include <chute.h>

extern void chuteAdvanceTime(struct chute * ch, long delta);
extern void chuteAction(struct chute * ch, int * entry_clear, long * item_exit);
extern long timeUntilChuteEvent(struct chute * ch);
extern long timeUntilChuteEventDebug(struct chute * ch);
extern void debugChute(struct chute * chute);

extern struct packedItem * packItems(long item, long count, struct packedItem * next);
extern struct chunk * addChunk(long left, long right, struct packedItem * items, struct chunk * next);

long msg1;
long msg2;
struct chute chute;

void setupChute(long speed, long slow_speed) {
    long yard = 256 * 60;

    msg1 = speed;
    msg2 = slow_speed;

    chute.length = 10 * yard;
    chute.speed  = speed;
    chute.slow_speed = slow_speed;
    chute.final_chunk = NULL;
    chute.free_chunks =
        addChunk(0*yard, 1*yard, packItems(7,1,NULL),
        addChunk(2*yard, 4*yard, packItems(3,2,NULL), NULL
        ));
}

int screenW = 800;
int screenH = 600;

void drawAnchorL(float x, float y) {
    DrawRectangleLines(x, y-16, 32, 32, GRAY);
}

void drawAnchorR(float x, float y) {
    DrawRectangleLines(x-32, y-16, 32, 32, GRAY);
}

void drawRail(float x1, float y1, float x2, float y2) {
    DrawLine(x1, y1, x2, y2, GREEN);
}

void drawItem(float x, float y, long item) {
    DrawCircle(x, y, 16, item == 3 ? RED : BLUE);
}

void drawGrid(float xbase, int n) {
    for(int i = 0; i < n; i++) {
        float x = xbase + i * 32;
        DrawLine(x, 0, x, 200, (Color){40,40,40,255});
    }
}

int main(){

    InitWindow(screenW, screenH, "chute");
    SetTargetFPS(60);

    setupChute(8, 8);
    debugChute(&chute);

    //long eta = timeUntilChuteEvent(&chute);
    //printf("eta = %ld\n", eta);

    long clock;
    //int bail = 0;

    while(!WindowShouldClose()) {

        if(IsKeyPressed(KEY_ONE)) setupChute(8, 8);
        if(IsKeyPressed(KEY_TWO)) setupChute(16, 0);
        if(IsKeyPressed(KEY_THREE)) setupChute(16, 8);
        if(IsKeyPressed(KEY_FOUR)) setupChute(24, 8);
        if(IsKeyPressed(KEY_FIVE)) setupChute(48, 16);

/*
        if(bail){
            if(bail == 2) exit(1);
            bail++;
        }
*/

        clock = 480; // 1 tick = 480

        //printf("\nstart new tick\n");

        while(clock > 0) {
            long eta = timeUntilChuteEvent(&chute);

            if(eta == 0) {
                printf("eta = 0, avoided possible freeze\n");
                timeUntilChuteEventDebug(&chute);
                exit(1);
            }

            //printf("clock = %ld, eta = %ld\n", clock, eta);
            if(eta <= clock) {
                int entry_clear = 0;
                long item_exit = 0;
                //printf("advancing by %ld\n", eta);
                chuteAdvanceTime(&chute, eta);
                //debugChute(&chute);
                //printf("doing chute action\n");
                chuteAction(&chute, &entry_clear, &item_exit);
                //printf("entry_clear = %d, item_exit = %ld\n", entry_clear, item_exit);
                //debugChute(&chute);
                clock -= eta;
                //bail = 1;
            }
            else {
                //printf("*cricket*, advancing by %ld\n", clock);
                chuteAdvanceTime(&chute, clock);
                //debugChute(&chute);
                clock = 0;
            }
        }

        // draw the state of play
        BeginDrawing();

        ClearBackground(BLACK);

        float yard = 256*60;
        float x1 = 100;
        float y1 = 100;
        float x2 = 100 + chute.length/yard * 32;
        float y2 = 100;

        drawGrid(x1, 11);
        drawAnchorL(x1, y1);
        drawAnchorR(x2, y2);
        drawRail(x1,y1,x2,y2);

        for(struct chunk * ptr = chute.free_chunks; ptr; ptr = ptr->next) {
            float lbase = ptr->left / yard * 32;
            float rbase = ptr->right / yard * 32;
            DrawLine(x1 + lbase, y1 + 20, x1 + rbase, y1 + 20, MAGENTA);
            for(struct packedItem * pi = ptr->items; pi; pi = pi->next) {
                for(int i = 0; i < pi->count; i++) {
                    drawItem(x1 + lbase + 16, y1, pi->item);
                    lbase += 32;
                }
            }
        }

        struct chunk * ptr = chute.final_chunk;
        if(ptr) {
            float lbase = ptr->left / yard * 32;
            float rbase = ptr->right / yard * 32;
            DrawLine(x1 + lbase, y1 + 20, x1 + rbase, y1 + 20, GOLD);
            for(struct packedItem * pi = ptr->items; pi; pi = pi->next) {
                for(int i = 0; i < pi->count; i++) {
                    drawItem(x1 + lbase + 16, y1, pi->item);
                    lbase += 32;
                }
            }
        }
        //DrawLine(x1, y1 + 20, x1 + 50, y1 + 20, GOLD);

        DrawText(TextFormat("speed = %d", msg1), 100, 500, 32, WHITE);
        DrawText(TextFormat("slow_speed = %d", msg2), 100, 550, 32, WHITE);

        EndDrawing();

    }

    CloseWindow(); 

    return 0;
}


