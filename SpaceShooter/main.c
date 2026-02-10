#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_BULLETS 10
#define MAX_ENEMIES 7
#define MAX_STARS 100

typedef struct {
    Vector2 pos;
    bool active;
} Bullet;

typedef struct {
    Vector2 pos;
    bool active;
    float speed;
} Enemy;

typedef struct {
    Vector2 pos;
    bool active;
    double expireTime;
} Shield;

typedef struct {
    Vector2 pos;
    float speed;
} Star;



int main(void)
{
    const int screenWidth = 1600;
    const int screenHeight = 850;

    InitWindow(screenWidth, screenHeight, "Space Shooter with Levels & Timer");
    InitAudioDevice(); 

    //  LOAD ASSETS 
    Texture2D playerTexture = LoadTexture("assets/player.png");
    Texture2D enemyTexture = LoadTexture("assets/enemy.png");

    Sound shootSound = LoadSound("assets/shoot.wav");
    Sound explosionSound = LoadSound("assets/explosion.wav");
    Sound levelupSound = LoadSound("assets/levelup.wav");

    //  Check if assets loaded
    if (playerTexture.id == 0) printf("ERROR: Failed to load player.png\n");
    if (enemyTexture.id == 0) printf("ERROR: Failed to load enemy.png\n");
    if (shootSound.frameCount == 0) printf("ERROR: Failed to load shoot.wav\n");
    if (explosionSound.frameCount == 0) printf("ERROR: Failed to load explosion.wav\n");
    if (levelupSound.frameCount == 0) printf("ERROR: Failed to load levelup.wav\n");


    //  Set scaling and dimensions for collision 
    float playerScale = 0.2f;
    float enemyScale = 0.125f;
    float playerWidth = (float)playerTexture.width * playerScale;
    float playerHeight = (float)playerTexture.height * playerScale;
    float enemyWidth = (float)enemyTexture.width * enemyScale;
    float enemyHeight = (float)enemyTexture.height * enemyScale;


    Vector2 playerPos = { screenWidth / 2.0f, screenHeight - 60 };
    int playerSpeed = 5;


    Bullet bullets[MAX_BULLETS] = { 0 };


    Enemy enemies[MAX_ENEMIES] = { 0 };
    float enemySpeed = 2.0f;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].pos = (Vector2){ GetRandomValue(0, screenWidth - (int)enemyWidth), GetRandomValue(-200, -50) };
        enemies[i].active = true;
        enemies[i].speed = enemySpeed;
    }


    Shield shield = { 0 };
    bool shieldActive = false;
    double shieldDuration = 5.0;
    double shieldExpireTime = 0;


    Star stars[MAX_STARS] = { 0 };
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].pos = (Vector2){ GetRandomValue(0, screenWidth), GetRandomValue(0, screenHeight) };
        stars[i].speed = (float)(GetRandomValue(1, 3));
    }



    int score = 0;
    int level = 1;
    int highScore = 0;
    bool gameOver = false;

    double startTime = GetTime();
    double lastLevelUpTime = GetTime();
    double levelMessageTime = 0;
    bool showLevelMessage = false;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        double currentTime = GetTime();

        if (!gameOver)
        {
            //  Level Up 
            if (currentTime - lastLevelUpTime >= 60.0)
            {
                level++;
                lastLevelUpTime = currentTime;
                enemySpeed += 0.5f;
                for (int i = 0; i < MAX_ENEMIES; i++)
                    enemies[i].speed = enemySpeed;

                PlaySound(levelupSound); 
                showLevelMessage = true;
                levelMessageTime = currentTime + 2.0;
            }

            //  Player Movement (Fixed boundaries for texture size) 
            if (IsKeyDown(KEY_LEFT) && playerPos.x > 0) playerPos.x -= playerSpeed;
            if (IsKeyDown(KEY_RIGHT) && playerPos.x < screenWidth - playerWidth) playerPos.x += playerSpeed;
            if (IsKeyDown(KEY_UP) && playerPos.y > 0) playerPos.y -= playerSpeed;
            if (IsKeyDown(KEY_DOWN) && playerPos.y < screenHeight - playerHeight) playerPos.y += playerSpeed;


            //  Player Shooting (Fixed bullet position and added sound) 
            if (IsKeyPressed(KEY_SPACE)) {
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (!bullets[i].active) {
                        // Fire from the center of the player texture
                        bullets[i].pos = (Vector2){ playerPos.x + (playerWidth / 2) - 2, playerPos.y };
                        bullets[i].active = true;
                        PlaySound(shootSound);
                        break;
                    }
                }
            }


            //  Move Bullets 
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (bullets[i].active) {
                    bullets[i].pos.y -= 5;
                    if (bullets[i].pos.y < 0) bullets[i].active = false;
                }
            }


            //  Move Enemies 
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active) {
                    if (GetRandomValue(0, 100) < 50) {
                        if (playerPos.x > enemies[i].pos.x) enemies[i].pos.x += 1;
                        else enemies[i].pos.x -= 1;
                    }
                    enemies[i].pos.y += enemies[i].speed;
                    if (enemies[i].pos.y > screenHeight) {
                        enemies[i].pos = (Vector2){ GetRandomValue(0, screenWidth - (int)enemyWidth), GetRandomValue(-200, -50) };
                        enemies[i].speed = enemySpeed;
                    }
                }
            }


            //  Bullet-Enemy Collision (Fixed for textures) 
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (bullets[i].active) {
                    Rectangle bulletRec = { bullets[i].pos.x - 2, bullets[i].pos.y - 2, 4, 4 }; 
                    for (int j = 0; j < MAX_ENEMIES; j++) {
                        if (enemies[j].active) {
                            Rectangle enemyRec = { enemies[j].pos.x, enemies[j].pos.y, enemyWidth, enemyHeight };

                             
                            if (CheckCollisionRecs(bulletRec, enemyRec)) { 
                                bullets[i].active = false;
                                enemies[j].pos = (Vector2){ GetRandomValue(0, screenWidth - (int)enemyWidth), GetRandomValue(-200, -50) };
                                enemies[j].speed = enemySpeed;

                                PlaySound(explosionSound); 

                                if (GetRandomValue(0, 100) < 60) {                  // 60% shield chance
                                    shield.pos = enemies[j].pos;
                                    shield.active = true;
                                    shieldExpireTime = currentTime + shieldDuration;
                                }

                                score += 10;
                                if (score > highScore) highScore = score;
                            }
                        }
                    }
                }
            }



            //  Enemy-Player Collision                 
            float paddingPlayer = 8.0f;  
            float paddingEnemy = 10.25f;  

            // Create player rectangle (top-left origin + padding)
            Rectangle playerRec = {
                playerPos.x + paddingPlayer,
                playerPos.y + paddingPlayer,
                playerWidth - 2.95 * paddingPlayer,
                playerHeight - 2.5 * paddingPlayer
            };

            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!enemies[i].active) continue;

                // Create enemy rectangle (top-left origin + padding)
                Rectangle enemyRec = {
                    enemies[i].pos.x + paddingEnemy,
                    enemies[i].pos.y + paddingEnemy,
                    enemyWidth -3.25 * paddingEnemy,
                    enemyHeight - 2 * paddingEnemy
                };


                if (CheckCollisionRecs(playerRec, enemyRec)) {
                    if (shieldActive) {
                        shieldActive = false;

                        // Reset enemy position and speed
                        enemies[i].pos = (Vector2){
                            GetRandomValue(0, screenWidth - (int)enemyWidth),
                            GetRandomValue(-200, -50)
                        };
                        enemies[i].speed = enemySpeed;
                    }
                    else {
                        gameOver = true;
                        PlaySound(explosionSound);
                       
                    }
                }
            }                                      


            //  Shield Collision 
            if (shield.active) {
                Rectangle shieldRec = { shield.pos.x - 10, shield.pos.y - 10, 20, 20 };

                // if (CheckCollisionCircles(playerPos, 15, shield.pos, 10)) { 
                if (CheckCollisionRecs(playerRec, shieldRec)) { 
                    shieldActive = true;
                    shield.active = false;
                    shieldExpireTime = currentTime + shieldDuration;
                }
            }

            if (shieldActive && currentTime > shieldExpireTime) shieldActive = false;


            //  Move Stars 
            for (int i = 0; i < MAX_STARS; i++) {
                stars[i].pos.y += stars[i].speed;
                if (stars[i].pos.y > screenHeight) {
                    stars[i].pos.x = GetRandomValue(0, screenWidth);
                    stars[i].pos.y = 0;
                    stars[i].speed = (float)(GetRandomValue(1, 3));
                }
            }
        }



        BeginDrawing();
        ClearBackground(BLACK);


        for (int i = 0; i < MAX_STARS; i++) {
            DrawPixel(stars[i].pos.x, stars[i].pos.y, WHITE);
        }

        if (!gameOver)
        {
            //  Draw Player Texture 
            DrawTextureEx(playerTexture, playerPos, 0.0f, playerScale, WHITE); 

            //  Draw Shield  
            if (shieldActive) DrawCircleLines(playerPos.x + playerWidth / 2, playerPos.y + playerHeight / 2, 25, SKYBLUE);


            //  Draw Bullets 
            for (int i = 0; i < MAX_BULLETS; i++)
                if (bullets[i].active) DrawCircle(bullets[i].pos.x, bullets[i].pos.y, 5, YELLOW);

            //  Draw Enemy Textures 
            for (int i = 0; i < MAX_ENEMIES; i++)
                if (enemies[i].active) {
                    // DrawCircle(enemies[i].pos.x, enemies[i].pos.y, 10, RED); 
                    DrawTextureEx(enemyTexture, enemies[i].pos, 0.0f, enemyScale, WHITE); 
                }

            //  Draw Shield Pickup 
            if (shield.active) DrawCircle(shield.pos.x, shield.pos.y, 10, GREEN);


            //  Draw UI 
            DrawText(TextFormat("Score: %d", score), 10, 10, 20, WHITE);
            DrawText(TextFormat("High Score: %d", highScore), 10, 40, 20, GOLD);
            DrawText(TextFormat("Level: %d", level), 10, 70, 20, WHITE);


            int timeLeft = 60 - (int)(currentTime - lastLevelUpTime);
            DrawText(TextFormat("Next Level In: %d s", timeLeft), screenWidth - 220, 10, 20, YELLOW);


            if (showLevelMessage) {
                DrawText(TextFormat("Level %d Complete!", level - 1), screenWidth / 2 - 100, screenHeight / 2 - 50, 30, ORANGE);
                if (currentTime > levelMessageTime) showLevelMessage = false;
            }
        }
        else
        {
            //  Game Over Screen 
            DrawText("GAME OVER!", screenWidth / 2 - 120, screenHeight / 2 - 40, 40, RED);
            DrawText(TextFormat("Final Score: %d", score), screenWidth / 2 - 90, screenHeight / 2 + 10, 20, WHITE);
            DrawText(TextFormat("High Score: %d", highScore), screenWidth / 2 - 90, screenHeight / 2 + 40, 20, GOLD);
            DrawText("Press [ENTER] to Play Again", screenWidth / 2 - 150, screenHeight / 2 + 70, 20, GRAY);


            if (IsKeyPressed(KEY_ENTER))
            {
                playerPos = (Vector2){ screenWidth / 2.0f, screenHeight - 60 };
                score = 0;
                level = 1;
                enemySpeed = 1.0f;
                lastLevelUpTime = currentTime;
                shieldActive = false;
                shield.active = false;


                for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = false;
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    enemies[i].pos = (Vector2){ GetRandomValue(0, screenWidth - (int)enemyWidth), GetRandomValue(-200, -50) };
                    enemies[i].active = true;
                    enemies[i].speed = enemySpeed;
                }

                gameOver = false;
            }
        }

        EndDrawing();
    }


    UnloadTexture(playerTexture);
    UnloadTexture(enemyTexture);
    UnloadSound(shootSound);
    UnloadSound(explosionSound);
    UnloadSound(levelupSound);

    CloseAudioDevice(); 
    CloseWindow();
    return 0;
}
