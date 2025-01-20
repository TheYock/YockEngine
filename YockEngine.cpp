#include <SDL.h>
#include <SDL_image.h>
#include <imgui.h>
#include "backend/imgui_impl_sdl2.h"
#include "backend/imgui_impl_opengl3.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <string>

// Screen dimensions
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// Structure to represent a sprite
struct Sprite {
    SDL_Rect rect;         // Rectangle representing position and size
    int speedX, speedY;    // Movement speeds in the x and y directions
    int lifetime;          // Remaining lifetime of the sprite (in frames)
    SDL_Texture* texture;  // Texture to render
};

// Function declaration for collision checking
bool checkCollision(const SDL_Rect& a, const SDL_Rect& b);

// Checks if two rectangles are overlapping
bool checkCollision(const SDL_Rect& a, const SDL_Rect& b) {
    return a.x < b.x + b.w &&
        a.x + a.w > b.x &&
        a.y < b.y + b.h &&
        a.y + a.h > b.y;
}

// Loads a texture from a file
SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());  // Load the image as a surface
    if (!surface) {
        std::cerr << "Failed to load image " << path << ": " << IMG_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);  // Create texture from surface
    SDL_FreeSurface(surface);  // Free the surface memory
    return texture;
}

// Frees all resources and cleans up SDL
void cleanup(SDL_Window* window, SDL_Renderer* renderer, const std::vector<SDL_Texture*>& textures) {
    for (SDL_Texture* texture : textures) {
        SDL_DestroyTexture(texture);  // Destroy each texture
    }
    if (renderer) SDL_DestroyRenderer(renderer);  // Destroy renderer if it exists
    if (window) SDL_DestroyWindow(window);        // Destroy window if it exists
    IMG_Quit();                                   // Quit SDL_image
    SDL_Quit();                                   // Quit SDL
}

// Spawns a new sprite with random properties
Sprite spawnSprite(const std::vector<SDL_Texture*>& textures) {
    Sprite sprite;
    // Random position within screen bounds, minus sprite size
    sprite.rect = { rand() % (SCREEN_WIDTH - 50), rand() % (SCREEN_HEIGHT - 50), 50, 50 };
    // Random speed in x and y directions
    sprite.speedX = (rand() % 5 + 1) * (rand() % 2 ? 1 : -1);  // Avoid zero speed
    sprite.speedY = (rand() % 5 + 1) * (rand() % 2 ? 1 : -1);
    // Random lifetime between 100 and 400 frames
    sprite.lifetime = rand() % 300 + 100;
    // Assign a random texture
    sprite.texture = textures[rand() % textures.size()];
    return sprite;
}

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(0)));  // Seed the random number generator

    // Initialize SDL and SDL_image
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create SDL window and OpenGL context
    SDL_Window* window = SDL_CreateWindow("YockEngine 0.0.1PROTOTYPE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);

    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load textures
    std::vector<SDL_Texture*> textures;
    textures.push_back(loadTexture(renderer, "assets/char1.png"));
    textures.push_back(loadTexture(renderer, "assets/char2.png"));
    textures.push_back(loadTexture(renderer, "assets/char3.png"));

    // Check for texture loading errors
    for (size_t i = 0; i < textures.size(); ++i) {
        if (!textures[i]) {
            std::cerr << "Error: Texture " << i + 1 << " failed to load!" << std::endl;
            cleanup(window, renderer, textures);
            return 1;
        }
    }

    std::vector<Sprite> sprites;  // Vector of active sprites
    int spawnTimer = 0;          // Timer to control sprite spawning

    bool isRunning = true;
    SDL_Event event;

    Uint32 lastTime = SDL_GetTicks();  // Track time for delta time calculation

    while (isRunning) {
        // Handle events (e.g., quit event)
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                isRunning = false;
            }
        }

        // Start Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // ImGui UI
        ImGui::Begin("Debug Info");
        ImGui::Text("Sprite Count: %zu", sprites.size());
        ImGui::SliderInt("Spawn Timer", &spawnTimer, 0, 60);
        ImGui::End();

        // Spawn new sprites at regular intervals
        spawnTimer++;
        if (spawnTimer > 30) {
            sprites.push_back(spawnSprite(textures));
            spawnTimer = 0;
        }

        // Update sprite positions
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        for (size_t i = 0; i < sprites.size(); ++i) {
            sprites[i].rect.x += static_cast<int>(sprites[i].speedX * deltaTime * 60);
            sprites[i].rect.y += static_cast<int>(sprites[i].speedY * deltaTime * 60);

            // Bounce off screen edges
            if (sprites[i].rect.x <= 0 || sprites[i].rect.x + sprites[i].rect.w >= SCREEN_WIDTH) {
                sprites[i].speedX = -sprites[i].speedX;
            }
            if (sprites[i].rect.y <= 0 || sprites[i].rect.y + sprites[i].rect.h >= SCREEN_HEIGHT) {
                sprites[i].speedY = -sprites[i].speedY;
            }

            // Decrease lifetime
            sprites[i].lifetime--;
        }

        // Handle sprite collisions
        for (size_t i = 0; i < sprites.size(); ++i) {
            for (size_t j = i + 1; j < sprites.size(); ++j) {
                if (checkCollision(sprites[i].rect, sprites[j].rect)) {
                    // Reverse direction of both sprites
                    sprites[i].speedX = -sprites[i].speedX;
                    sprites[i].speedY = -sprites[i].speedY;

                    sprites[j].speedX = -sprites[j].speedX;
                    sprites[j].speedY = -sprites[j].speedY;

                    // Slightly separate the sprites to prevent overlapping
                    if (sprites[i].rect.x < sprites[j].rect.x) {
                        sprites[i].rect.x -= 1;
                        sprites[j].rect.x += 1;
                    }
                    else {
                        sprites[i].rect.x += 1;
                        sprites[j].rect.x -= 1;
                    }

                    if (sprites[i].rect.y < sprites[j].rect.y) {
                        sprites[i].rect.y -= 1;
                        sprites[j].rect.y += 1;
                    }
                    else {
                        sprites[i].rect.y += 1;
                        sprites[j].rect.y -= 1;
                    }
                }
            }
        }

        // Remove expired sprites
        for (auto it = sprites.begin(); it != sprites.end();) {
            if (it->lifetime <= 0) {
                it = sprites.erase(it);  // Remove sprite and update iterator
            }
            else {
                ++it;  // Move to the next sprite
            }
        }

        // Remove expired sprites
        for (auto it = sprites.begin(); it != sprites.end();) {
            if (it->lifetime <= 0) {
                it = sprites.erase(it);  // Remove sprite and update iterator
            }
            else {
                ++it;  // Move to the next sprite
            }
        }

        // Render the scene
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Clear screen with black
        SDL_RenderClear(renderer);

        for (const auto& sprite : sprites) {
            SDL_RenderCopy(renderer, sprite.texture, nullptr, &sprite.rect);  // Draw sprite
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_RenderPresent(renderer);  // Present the frame

        SDL_Delay(16);  // Cap frame rate at ~60 FPS
    }

    // Cleanup resources and quit
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    cleanup(window, renderer, textures);
    return 0;
}

// Fly High Cale \\

