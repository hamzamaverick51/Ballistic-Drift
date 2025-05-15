#include <iostream>
#include <raylib.h>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

Color Green = Color{ 38, 185, 154, 255 };
Color Dark_Green = Color{ 28, 160, 133, 255 };
Color Light_Green = Color{ 129, 204, 184, 255 };
Color Yellow = Color{ 243, 213, 91, 255 };
Color Orange = Color{ 255, 146, 0, 255 };
Color DarkOrange = Color{ 230, 97, 0, 255 };
Color Blue = Color{ 0, 0, 241, 255 };
Color Red = Color{ 230, 41, 55, 255 };
Color light_grey = Color{ 69, 69, 69, 255 };
Color grey = Color{ 32, 32, 32, 255 };

struct RainLine {
    float x;
    float y;
    float speed;
    float length;
    float thickness;
};

struct HighScore {
    int easy = 0;
    int medium = 0;
    int hard = 0;
};

struct FlameParticle {
    Vector2 pos;
    float size;
    float life;
    float maxLife;
    Color color;
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    float life;
};

struct Ripple {
    Vector2 center;
    float radius;
    float maxRadius;
    float thickness;
    float alpha;
    float speed;
    Color color;
};

int player_score = 0;
int cpu_score = 0;
int total_score = 0;
int last_scorer = 0;

enum GameScreen { MENU, MENU_DIFFICULTIES, PAUSE, GAMEPLAY_CPU, GAMEPLAY_PVP };
enum difficulties { EASY, MEDIUM, HARD };

bool flameOn = true;

void SaveHighScore(const HighScore& highScore)
{
    ofstream file("highscores.txt");
    if (file.is_open()) {
        file << highScore.easy << "\n";
        file << highScore.medium << "\n";
        file << highScore.hard << "\n";
        file.close();
    }
}

HighScore LoadHighScore()
{
    HighScore highScore;
    ifstream file("highscores.txt");
    if (file.is_open())
    {
        string line;
        getline(file, line);
        highScore.easy = stoi(line);
        getline(file, line);
        highScore.medium = stoi(line);
        getline(file, line);
        highScore.hard = stoi(line);
        file.close();
    }
    return highScore;
}

class Ball {
public:
    float x;
    float y;
    float speed_x;
    float speed_y;
    int radius;
    vector<FlameParticle> flames;
    float flameTimer = 0.0f;

    void Draw()
    {
        for (const auto& flame : flames)
            DrawCircleV(flame.pos, flame.size, ColorAlpha(flame.color, flame.life / flame.maxLife));

        DrawCircleGradient(
            static_cast<int>(x),
            static_cast<int>(y),
            static_cast<float>(radius),
            DarkOrange,
            Yellow
        );
    }

    void UpdateFlames(float dt, bool toggleFlames)
    {
        if (toggleFlames == true) {
            for (int i = flames.size() - 1; i >= 0; i--) {
                flames[i].life -= dt;
                if (flames[i].life <= 0)
                    flames.erase(flames.begin() + i);
            }

            flameTimer -= dt;
            if (flameTimer <= 0) {
                flameTimer = 0.01f;

                float speed = sqrt(speed_x * speed_x + speed_y * speed_y);
                float dir_x = -speed_x / speed;
                float dir_y = -speed_y / speed;

                for (int i = 0; i < 3; i++) {
                    FlameParticle flame;
                    flame.pos.x = x + dir_x * (radius * 0.8f);
                    flame.pos.y = y + dir_y * (radius * 0.8f);
                    flame.pos.x += GetRandomValue(-5, 5);
                    flame.pos.y += GetRandomValue(-5, 5);
                    flame.size = GetRandomValue(8, radius);
                    flame.maxLife = GetRandomValue(5, 15) / 100.0f;
                    flame.life = flame.maxLife;

                    int colorChoice = GetRandomValue(0, 10);
                    if (colorChoice < 5)
                        flame.color = Orange;
                    else if (colorChoice < 8)
                        flame.color = RED;
                    else
                        flame.color = Yellow;

                    flames.push_back(flame);
                }
            }
        }
    }

    void Update(float dt) {
        UpdateFlames(dt, flameOn);
        x += speed_x;
        y += speed_y;

        if (y + radius >= GetScreenHeight() || y - radius <= 0)
            speed_y *= -1;

        if (x + radius >= GetScreenWidth()) {
            cpu_score += 1;
            last_scorer = 2;
            if (speed_x < 0)
                speed_x -= 0.25;
            else
                speed_x += 0.25;
            if (speed_y < 0)
                speed_y -= 0.25;
            else
                speed_y += 0.25;
            x = static_cast<float>(GetScreenWidth() / 2);
            y = static_cast<float>(GetScreenHeight() / 2);
            flames.clear();
        }

        if (x - radius <= 0) {
            player_score += 1;
            last_scorer = 1;
            if (speed_x < 0)
                speed_x -= 0.25;
            else
                speed_x += 0.25;
            if (speed_y < 0)
                speed_y -= 0.25;
            else
                speed_y += 0.25;
            x = static_cast<float>(GetScreenWidth() / 2);
            y = static_cast<float>(GetScreenHeight() / 2);
            flames.clear();
        }
    }
};

class Paddle {
protected:
    void LimitMove() {
        if (y <= 0)
            y = 0;
        if (y + height >= GetScreenHeight())
            y = GetScreenHeight() - height;
    }

public:
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    int speed = 0;

    void Draw(int Player_Num) {
        if (Player_Num == 1)
            DrawRectangleRounded(Rectangle{ x, y, width, height }, 0.8f, 0, Blue);
        else if (Player_Num == 2)
            DrawRectangleRounded(Rectangle{ x, y, width, height }, 0.8f, 0, Red);
    }

    virtual void Update(GameScreen current)
    {
        if (current == GAMEPLAY_CPU)
        {
            if (IsKeyDown(KEY_UP))
                y -= static_cast<float>(speed);
            if (IsKeyDown(KEY_DOWN))
                y += static_cast<float>(speed);
            LimitMove();
        }
        else if (current == GAMEPLAY_PVP)
        {
            if (IsKeyDown(KEY_W))
                y -= static_cast<float>(speed);
            if (IsKeyDown(KEY_S))
                y += static_cast<float>(speed);
            LimitMove();
        }
    }
};

class CPU_PADDLE : public Paddle {
public:
    void Draw() {
        DrawRectangleRounded(Rectangle{ x, y, width, height }, 0.8f, 0, Red);
    }

    void myUpdate(float ball_x, float ball_y, difficulties Difficulty) {
        if (Difficulty == EASY) {
            if (ball_x > 1440) {
                if (this->y > ball_y)
                    y -= speed;
                else if (this->y < ball_y)
                    y += speed;
            }
        }
        else if (Difficulty == MEDIUM) {
            if (ball_x > 1200) {
                if (this->y > ball_y)
                    y -= (speed + 1.5);
                else if (this->y < ball_y)
                    y += (speed + 1.5);
            }
        }
        else if (Difficulty == HARD) {
            if (ball_x > 960) {
                if (this->y > ball_y)
                    y -= (speed + 3);
                else if (this->y < ball_y)
                    y += (speed + 3);
            }
        }
        LimitMove();
    }
};

bool Button(int buttonX, int position, int buttonWidth, int buttonHeight, const char* text) {
    Rectangle btn = { static_cast<float>(buttonX), static_cast<float>(position), static_cast<float>(buttonWidth), static_cast<float>(buttonHeight) };
    btn.y -= 30.0f;
    bool hover = CheckCollisionPointRec(GetMousePosition(), btn);
    btn.y += 30.0f;
    DrawRectangleRounded(btn, 0.5f, 0, hover ? grey : light_grey);
    const char* txt = text;
    int w = MeasureText(txt, 40);
    DrawText(txt, 1920 / 2 - w / 2, position + buttonHeight / 2 - 20, 40, WHITE);
    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        return true;
    return false;
}

int main() {
    const int screen_width = 1920;
    const int screen_height = 1080;
    InitWindow(screen_width, screen_height, "Ballistic Drift");
    SetTargetFPS(60);

    InitAudioDevice();
    Sound scoreSound = LoadSound("ripple.mp3");
    Sound pauseSound = LoadSound("PAUSE.mp3");

    // Load background music tracks
    Music menuMusic = LoadMusicStream("Menu_Sans.mp3");
    Music easyMediumMusic = LoadMusicStream("Easy_Medium.mp3");
    Music hardMusic = LoadMusicStream("F1_Hard.mp3");

    // Set music volume
    SetMusicVolume(menuMusic, 0.5f);
    SetMusicVolume(easyMediumMusic, 0.5f);
    SetMusicVolume(hardMusic, 1);

    SetSoundVolume(scoreSound, 0.75f);

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;

    float shakeTimer = 0.0f;
    const float shakeDuration = 0.3f;
    const int shakeIntensity = 15;

    float spawnTimer = 0.0f;
    const float spawnDelay = 0.5f;

    HighScore highScore = LoadHighScore();
    bool newHighScore = false;

    vector<Particle> particles;
    vector<Ripple> ripples;
    vector<RainLine> rain;

    for (int i = 0; i < 75; i++) {
        RainLine r;
        r.x = GetRandomValue(0, screen_width);
        r.y = GetRandomValue(0, screen_height);
        r.speed = GetRandomValue(100, 400) / 100.0f;
        r.length = GetRandomValue(50, 200);
        r.thickness = GetRandomValue(1, 3);
        rain.push_back(r);
    }

    Ball ball;
    ball.radius = 20;
    ball.x = screen_width / 2.0f;
    ball.y = screen_height / 2.0f;
    ball.speed_x = 15.0f;
    ball.speed_y = 15.0f;

    Paddle player, player2;
    player.width = player2.width = 25;
    player.height = player2.height = 120;
    player.x = 10;
    player2.x = screen_width - player2.width - 10;
    player.y = player2.y = screen_height / 2.0f - player.height / 2.0f;
    player.speed = player2.speed = 13;

    CPU_PADDLE cpu;
    cpu.width = 25;
    cpu.height = 120;
    cpu.x = screen_width - cpu.width - 10;
    cpu.y = screen_height / 2.0f - cpu.height / 2.0f;
    cpu.speed = 13;

    GameScreen currentScreen = MENU;
    difficulties currentDifficulty = EASY;

    // Music state management
    Music* currentMusic = nullptr;
    bool musicChanged = false;

    const float buttonWidth = 300.0f;
    const float buttonHeight = 60.0f;
    const float buttonX = screen_width / 2.0f - buttonWidth / 2.0f;
    const float menuY1 = screen_height / 2.0f - buttonHeight - 10.0f;
    const float menuY2 = screen_height / 2.0f + 10.0f;
    const float menuY3 = screen_height / 2.0f + buttonHeight + 30.0f;

    char flameText[2][15] = { "FLAME ON", "FLAME OFF" };
    int f = 0;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Update music stream
        if (currentMusic != nullptr) {
            UpdateMusicStream(*currentMusic);
        }

        // Handle music transitions
        if (musicChanged) {
            if (currentMusic != nullptr) {
                StopMusicStream(*currentMusic);
            }

            switch (currentScreen) {
            case MENU:
            case MENU_DIFFICULTIES:
                currentMusic = &menuMusic;
                break;
            case GAMEPLAY_CPU:
                if (currentDifficulty == HARD) {
                    currentMusic = &hardMusic;
                }
                else {
                    currentMusic = &easyMediumMusic;
                }
                break;
            case GAMEPLAY_PVP:
                currentMusic = &easyMediumMusic;
                break;
            case PAUSE:
                // Keep current music but pause it
                if (currentMusic != nullptr) {
                    PauseMusicStream(*currentMusic);
                }
                break;
            }

            if (currentMusic != nullptr && currentScreen != PAUSE) {
                PlayMusicStream(*currentMusic);
            }
            musicChanged = false;
        }

        if ((currentScreen == GAMEPLAY_CPU || currentScreen == GAMEPLAY_PVP) && IsKeyPressed(KEY_Q)) {
            PlaySound(pauseSound);
            currentScreen = PAUSE;
            musicChanged = true;
        }

        BeginDrawing();

        if (currentScreen == GAMEPLAY_CPU || currentScreen == GAMEPLAY_PVP) {
            for (auto& r : rain) {
                r.y += r.speed;
                if (r.y > screen_height + r.length) {
                    r.y = -r.length;
                    r.x = GetRandomValue(0, screen_width);
                    r.speed = GetRandomValue(100, 400) / 100.0f;
                    r.length = GetRandomValue(50, 200);
                    r.thickness = GetRandomValue(1, 3);
                }
            }
            ClearBackground(BLACK);

            // Check for new high score
            if (currentScreen == GAMEPLAY_CPU)
            {
                int currentScore = (last_scorer == 1) ? player_score : cpu_score;
                switch (currentDifficulty)
                {
                case EASY:
                    if (currentScore > highScore.easy && last_scorer == 2) //GIGGITY
                    {
                        highScore.easy = currentScore;
                        newHighScore = true;
                        SaveHighScore(highScore);
                    }
                    break;
                case MEDIUM:
                    if (currentScore > highScore.medium && last_scorer == 2)
                    {
                        highScore.medium = currentScore;
                        newHighScore = true;
                        SaveHighScore(highScore);
                    }
                    break;
                case HARD:
                    if (currentScore > highScore.hard && last_scorer == 2)
                    {
                        highScore.hard = currentScore;
                        newHighScore = true;
                        SaveHighScore(highScore);
                    }
                    break;
                }
            }
        }
        else {
            ClearBackground(BLACK);
        }

        if (currentScreen == MENU) {
            const char* title = "Ballistic Drift";
            int titleSize = 80;
            int titleWidth = MeasureText(title, titleSize);
            DrawText(title, screen_width / 2 - titleWidth / 2, screen_height / 4, titleSize, WHITE);

            if (Button(static_cast<int>(buttonX), static_cast<int>(menuY1), static_cast<int>(buttonWidth), static_cast<int>(buttonHeight), "P1 VS CPU") == true)
            {
                currentScreen = MENU_DIFFICULTIES;
                newHighScore = false;
                musicChanged = true;
            }

            if (Button(static_cast<int>(buttonX), static_cast<int>(menuY2), static_cast<int>(buttonWidth), static_cast<int>(buttonHeight), "P1 VS P2") == true) {
                currentScreen = GAMEPLAY_PVP;
                musicChanged = true;
            }

            if (Button(static_cast<int>(buttonX), static_cast<int>(menuY3), static_cast<int>(buttonWidth), static_cast<int>(buttonHeight), flameText[f]) == true) {
                flameOn = !flameOn;
                f = 1 - f;
                Button(static_cast<int>(buttonX), static_cast<int>(menuY3), static_cast<int>(buttonWidth), static_cast<int>(buttonHeight), flameText[f]);
            }
            else
                Button(static_cast<int>(buttonX), static_cast<int>(menuY3), static_cast<int>(buttonWidth), static_cast<int>(buttonHeight), flameText[f]);
        }
        else if (currentScreen == PAUSE) {
            DrawRectangle(0, 0, screen_width, screen_height, Color{ 0, 0, 0, 200 });
            DrawText("Paused", screen_width / 2 - MeasureText("Paused", 60) / 2, screen_height / 3, 60, WHITE);

            const float pauseButtonWidth = 300.0f;
            const float pauseButtonHeight = 60.0f;
            const float pauseButtonX = screen_width / 2.0f - pauseButtonWidth / 2.0f;
            const float pauseMenuY1 = screen_height / 2.0f - pauseButtonHeight - 10.0f;
            const float pauseMenuY2 = screen_height / 2.0f + 10.0f;
            const float pauseMenuY3 = screen_height / 2.0f + pauseButtonHeight + 30.0f;

            if (Button(static_cast<int>(pauseButtonX), static_cast<int>(pauseMenuY1), static_cast<int>(pauseButtonWidth), static_cast<int>(pauseButtonHeight), "RESUME") == true) {
                PlaySound(pauseSound);
                currentScreen = (player2.x == 10.0f ? GAMEPLAY_PVP : GAMEPLAY_CPU);
                musicChanged = true;
            }

            if (Button(static_cast<int>(pauseButtonX), static_cast<int>(pauseMenuY2), static_cast<int>(pauseButtonWidth), static_cast<int>(pauseButtonHeight), "RESTART") == true) {
                PlaySound(pauseSound);
                player_score = 0;
                cpu_score = 0;
                ball.x = screen_width / 2.0f;
                ball.y = screen_height / 2.0f;
                ball.speed_x = 15.0f;
                ball.speed_y = 15.0f;
                player.y = screen_height / 2.0f - player.height / 2.0f;
                player2.y = screen_height / 2.0f - player2.height / 2.0f;
                cpu.y = screen_height / 2.0f - cpu.height / 2.0f;
                currentScreen = (player2.x == 10.0f ? GAMEPLAY_PVP : GAMEPLAY_CPU);
                musicChanged = true;
            }

            if (Button(static_cast<int>(pauseButtonX), static_cast<int>(pauseMenuY3), static_cast<int>(pauseButtonWidth), static_cast<int>(pauseButtonHeight), "MAIN MENU") == true) {
                PlaySound(pauseSound);
                player_score = 0;
                cpu_score = 0;
                ball.x = screen_width / 2.0f;
                ball.y = screen_height / 2.0f;
                ball.speed_x = 15.0f;
                ball.speed_y = 15.0f;
                player.y = screen_height / 2.0f - player.height / 2.0f;
                player2.y = screen_height / 2.0f - player2.height / 2.0f;
                cpu.y = screen_height / 2.0f - cpu.height / 2.0f;
                currentScreen = MENU;
                musicChanged = true;
            }
        }
        else if (currentScreen == MENU_DIFFICULTIES) {
            if (Button(static_cast<int>(buttonX), static_cast<int>(menuY1) - 15, static_cast<int>(buttonWidth), static_cast<int>(buttonHeight), "EASY") == true) {
                currentScreen = GAMEPLAY_CPU;
                currentDifficulty = EASY;
                musicChanged = true;
            }

            if (Button(static_cast<int>(buttonX), static_cast<int>(menuY2) - 15, static_cast<int>(buttonWidth), static_cast<int>(buttonHeight), "MEDIUM") == true) {
                currentScreen = GAMEPLAY_CPU;
                currentDifficulty = MEDIUM;
                musicChanged = true;
            }

            if (Button(static_cast<int>(buttonX), static_cast<int>(menuY3) - 15, static_cast<int>(buttonWidth), static_cast<int>(buttonHeight), "HARD") == true) {
                currentScreen = GAMEPLAY_CPU;
                currentDifficulty = HARD;
                musicChanged = true;
            }
        }
        else {
            float dt = GetFrameTime();
            if (shakeTimer > 0) {
                shakeTimer -= dt;
                camera.offset.x = static_cast<float>(GetRandomValue(-shakeIntensity, shakeIntensity));
                camera.offset.y = static_cast<float>(GetRandomValue(-shakeIntensity, shakeIntensity));
            }
            else {
                camera.offset = { 0.0f, 0.0f };
            }

            for (int i = static_cast<int>(particles.size()) - 1; i >= 0; i--) {
                particles[i].pos.x += particles[i].vel.x * dt;
                particles[i].pos.y += particles[i].vel.y * dt;
                particles[i].life -= dt;
                if (particles[i].life <= 0) particles.erase(particles.begin() + i);
            }

            for (int i = static_cast<int>(ripples.size()) - 1; i >= 0; i--) {
                ripples[i].radius += ripples[i].speed * dt;
                ripples[i].alpha = 1.0f - (ripples[i].radius / ripples[i].maxRadius);

                if (ripples[i].radius >= ripples[i].maxRadius) {
                    ripples.erase(ripples.begin() + i);
                }
            }

            BeginMode2D(camera);

            int oldScore = player_score + cpu_score;

            if (spawnTimer <= 0) {
                ball.Update(dt);
                if (currentScreen == GAMEPLAY_CPU) {
                    player.Update(currentScreen);
                    cpu.myUpdate(ball.x, ball.y, currentDifficulty);
                }
                else {
                    player.Update(currentScreen);
                    if (IsKeyDown(KEY_UP)) player2.y -= static_cast<float>(player2.speed); //GIGGITY
                    if (IsKeyDown(KEY_DOWN)) player2.y += static_cast<float>(player2.speed);
                    if (player2.y <= 0) player2.y = 0;
                    if (player2.y + player2.height >= screen_height) player2.y = static_cast<float>(screen_height) - player2.height;
                }

                if (CheckCollisionCircleRec(Vector2{ ball.x, ball.y }, static_cast<float>(ball.radius),
                    Rectangle{ player.x, player.y, player.width, player.height })) {
                    ball.speed_x *= -1;

                    for (int i = 0; i < 25 && flameOn == true; i++) {
                        FlameParticle flame;
                        flame.pos.x = ball.x;
                        flame.pos.y = ball.y;
                        flame.size = GetRandomValue(5, 20);
                        flame.maxLife = GetRandomValue(10, 30) / 100.0f;
                        flame.life = flame.maxLife;

                        int colorChoice = GetRandomValue(0, 10);

                        if (colorChoice < 5)
                            flame.color = Orange;
                        else if (colorChoice < 8)
                            flame.color = RED;
                        else
                            flame.color = Yellow;

                        ball.flames.push_back(flame);
                    }

                    for (int i = 0; i < 15; i++) {
                        Particle p;
                        p.pos = { ball.x, ball.y };
                        float angle = static_cast<float>(GetRandomValue(0, 360)) * DEG2RAD;
                        p.vel = { cosf(angle) * 200.0f, sinf(angle) * 200.0f };
                        p.life = 0.3f;
                        particles.push_back(p);
                    }
                }

                if (currentScreen == GAMEPLAY_CPU) {
                    if (CheckCollisionCircleRec(Vector2{ ball.x, ball.y }, static_cast<float>(ball.radius),
                        Rectangle{ cpu.x, cpu.y, cpu.width, cpu.height })) {
                        ball.speed_x *= -1;

                        for (int i = 0; i < 25 && flameOn == true; i++) {
                            FlameParticle flame;
                            flame.pos.x = ball.x;
                            flame.pos.y = ball.y;
                            flame.size = GetRandomValue(5, 20);
                            flame.maxLife = GetRandomValue(10, 30) / 100.0f;
                            flame.life = flame.maxLife;

                            int colorChoice = GetRandomValue(0, 10);
                            if (colorChoice < 5)
                                flame.color = Orange;
                            else if (colorChoice < 8)
                                flame.color = RED;
                            else
                                flame.color = Yellow;

                            ball.flames.push_back(flame);
                        }

                        for (int i = 0; i < 15; i++) {
                            Particle p;
                            p.pos = { ball.x, ball.y };
                            float angle = static_cast<float>(GetRandomValue(0, 360)) * DEG2RAD;
                            p.vel = { cosf(angle) * 200.0f, sinf(angle) * 200.0f };
                            p.life = 0.3f;
                            particles.push_back(p);
                        }
                    }
                }
                else {
                    if (CheckCollisionCircleRec(Vector2{ ball.x, ball.y }, static_cast<float>(ball.radius),
                        Rectangle{ player2.x, player2.y, player2.width, player2.height })) {
                        ball.speed_x *= -1;

                        for (int i = 0; i < 25 && flameOn == true; i++) {
                            FlameParticle flame;
                            flame.pos.x = ball.x;
                            flame.pos.y = ball.y;
                            flame.size = GetRandomValue(5, 20);
                            flame.maxLife = GetRandomValue(10, 30) / 100.0f;
                            flame.life = flame.maxLife;

                            int colorChoice = GetRandomValue(0, 10);
                            if (colorChoice < 5)
                                flame.color = Orange;
                            else if (colorChoice < 8)
                                flame.color = RED;
                            else
                                flame.color = Yellow;

                            ball.flames.push_back(flame);
                        }

                        for (int i = 0; i < 15; i++) {
                            Particle p;
                            p.pos = { ball.x, ball.y };
                            float angle = static_cast<float>(GetRandomValue(0, 360)) * DEG2RAD;
                            p.vel = { cosf(angle) * 200.0f, sinf(angle) * 200.0f };
                            p.life = 0.3f;
                            particles.push_back(p);
                        }
                    }
                }

                if (player_score + cpu_score > oldScore) {
                    PlaySound(scoreSound);
                    shakeTimer = shakeDuration;
                    spawnTimer = spawnDelay;

                    Color rippleColor = (last_scorer == 1) ? Blue : Red;

                    Ripple r;
                    r.center = { static_cast<float>(screen_width) / 2.0f, static_cast<float>(screen_height) / 2.0f };
                    r.radius = 10.0f;
                    r.maxRadius = static_cast<float>(screen_width > screen_height ? screen_width : screen_height);
                    r.thickness = 10.0f;
                    r.alpha = 1.0f;
                    r.speed = 800.0f;
                    r.color = rippleColor;
                    ripples.push_back(r);

                    for (int i = 0; i < 3; i++) {
                        Ripple smallRipple;
                        smallRipple.center = { static_cast<float>(screen_width) / 2.0f, static_cast<float>(screen_height) / 2.0f };
                        smallRipple.radius = 5.0f;
                        smallRipple.maxRadius = static_cast<float>((screen_width > screen_height ? screen_width : screen_height)) * 0.7f;
                        smallRipple.thickness = 5.0f;
                        smallRipple.alpha = 1.0f;
                        smallRipple.speed = 600.0f + i * 100.0f;
                        smallRipple.color = rippleColor;
                        ripples.push_back(smallRipple);
                    }
                }
            }
            else {
                spawnTimer -= dt;
            }

            DrawRing(Vector2{ screen_width / 2.0f, screen_height / 2.0f }, 145, 146, 0, 360, 64, WHITE);
            DrawLine(screen_width / 2, 0, screen_width / 2, screen_height, WHITE);

            Color rainColor;
            if (player_score > cpu_score) rainColor = Red;
            else if (cpu_score > player_score) rainColor = Blue;
            else rainColor = WHITE;

            for (auto& r : rain) {
                DrawLineEx({ r.x, r.y }, { r.x, r.y + r.length }, r.thickness, rainColor);
                DrawCircle(r.x, r.y + r.length, 2, rainColor);
            }

            for (const auto& r : ripples) {
                DrawRing(r.center, r.radius, r.radius + r.thickness, 0.0f, 360.0f, 64, Fade(r.color, r.alpha));
            }

            for (auto& p : particles) {
                float alpha = p.life / 0.3f;
                DrawCircleV(p.pos, 3.0f, Fade(Yellow, alpha));
            }

            ball.Draw();
            player.Draw(1);
            if (currentScreen == GAMEPLAY_CPU) cpu.Draw();
            else player2.Draw(2);

            DrawText(TextFormat("%i", player_score), 3 * screen_width / 4, 60, 60, WHITE);
            DrawText(TextFormat("%i", cpu_score), screen_width / 4, 60, 60, WHITE);

            // Draw high score
            const char* difficultyText = "";
            switch (currentDifficulty) {
            case EASY: difficultyText = "EASY"; break;
            case MEDIUM: difficultyText = "MEDIUM"; break;
            case HARD: difficultyText = "HARD"; break;
            }

            if (currentScreen == GAMEPLAY_CPU) {
                int currentHighScore = 0;
                switch (currentDifficulty) {
                case EASY: currentHighScore = highScore.easy; break;
                case MEDIUM: currentHighScore = highScore.medium; break;
                case HARD: currentHighScore = highScore.hard; break;
                }

                DrawText(TextFormat("HIGH SCORE (%s): %i", difficultyText, currentHighScore),
                    screen_width / 2 - MeasureText(TextFormat("HIGH SCORE (%s): %i", difficultyText, currentHighScore), 30) / 2,
                    30, 30, WHITE);

                if (newHighScore) {
                    DrawText("NEW HIGH SCORE!", screen_width / 2 - MeasureText("NEW HIGH SCORE!", 40) / 2, 100, 40, YELLOW);
                }
            }

            EndMode2D();
        }

        EndDrawing();
    }

    // Unload music
    UnloadMusicStream(menuMusic);
    UnloadMusicStream(easyMediumMusic);
    UnloadMusicStream(hardMusic);

    UnloadSound(scoreSound);
    UnloadSound(pauseSound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}