//******************Space Invader************
#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <math.h>
#include <vector>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif
using namespace std;
//globals
ShaderProgram program;
glm::mat4 modelMatrix = glm::mat4(1.0f);
GLuint spriteTexture;
GLuint fontTexture;
SDL_Event event;
enum GameMode {STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER};
GameMode mode = STATE_MAIN_MENU;

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
float accumulator = 0.0f;

const int MAX_BULLETS = 30;
//const int MAX_ENEMIES = 15;
int bulletIndex = 0;
float elapsed = 0.0f;
float lastFrameTicks = 0.0f;
float bulletTimer = 0.5f;
bool done = false;

SDL_Window* displayWindow;
GLuint LoadTexture(const char* filePath) {
    int w, h, comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    
    if (image == NULL) {
        cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_image_free(image);
    return retTexture;
}

void DrawText(ShaderProgram &program, int fontTexture, string text, float x, float y, float size, float spacing) {
    float texture_size = 1.0 / 16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    glm::mat4 textModelMatrix = glm::mat4(1.0f);
    textModelMatrix = glm::translate(textModelMatrix, glm::vec3(x, y, 1.0f));
    for (size_t i = 0; i < text.size(); i++) {
        float texture_x = (float)(((int)text[i]) % 16) / 16.0f;
        float texture_y = (float)(((int)text[i]) / 16) / 16.0f;
        vertexData.insert(vertexData.end(), {
            ((size + spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size + spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size + spacing) * i) + (0.5f * size), 0.5f * size,
            ((size + spacing) * i) + (0.5f * size), -0.5f * size,
            ((size + spacing) * i) + (0.5f * size), 0.5f * size,
            ((size + spacing) * i) + (-0.5f * size), -0.5f * size,
        });
        texCoordData.insert(texCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x + texture_size, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x, texture_y + texture_size,
        });
    }
    glUseProgram(program.programID);
    program.SetModelMatrix(textModelMatrix);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glEnableVertexAttribArray(program.texCoordAttribute);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glDrawArrays(GL_TRIANGLES, 0, int(text.size()) * 6);
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
}
//non-uniform sprite
class SheetSprite {
public:
    SheetSprite(){};
    SheetSprite(unsigned int input_textureID, float input_u, float input_v, float input_width, float input_height, float
                input_size){
        this -> textureID = input_textureID;
        this -> u = input_u;
        this -> v = input_v;
        this -> width = input_width;
        this -> height = input_height;
        this -> size = input_size;
    }
    void Draw(ShaderProgram &program);
    float size;
    unsigned int textureID;
    float u;
    float v;
    float width;
    float height;
    glm::vec2 trueSize;
};


void SheetSprite::Draw(ShaderProgram &program) {
    glBindTexture(GL_TEXTURE_2D, textureID);
    GLfloat texCoords[] = {
        u, v+height,
        u+width, v,
        u, v,
        u+width, v,
        u, v+height,
        u+width, v+height
    };
    float aspect = width*2.0f / height;
    float vertices[] = {
        -0.5f * size * aspect, -0.5f * size,
        0.5f * size * aspect, 0.5f * size,
        -0.5f * size * aspect, 0.5f * size,
        0.5f * size * aspect, 0.5f * size,
        -0.5f * size * aspect, -0.5f * size ,
        0.5f * size * aspect, -0.5f * size};
    
    trueSize.x = size * aspect;
    trueSize.y = size;
    
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
}

class Entity{
public:
    Entity(){}
    Entity(const string& Type, SheetSprite& input_sprite, float x, float y, float v_x, float v_y, float size_x, float size_y){
        this->position = glm::vec2(x,y);
        this->velocity = glm::vec2(v_x,v_y);
        this->size = glm::vec2(size_x,size_y);
        this->type = Type;
        this->sprite = input_sprite;
        if(type == "enemy"){
            this->health = 1;
        }else if(type == "player"){
            this->health = 2;
        }
        
    }
    void draw(ShaderProgram &p){
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(position.x, position.y, 1.0f));
        program.SetModelMatrix(modelMatrix);//move
        
        sprite.Draw(p);//draw entity using sprite.draw
    };
    void update(float elapsed) {
        position.x += velocity.x * elapsed;
        position.y += velocity.y * elapsed;
        if(type == "player"){
            if (position.x-(sprite.trueSize.x*0.5f) <= -1.62){
                position.x = -1.62f + sprite.trueSize.x*0.5f;
            }
            if (position.x+(sprite.trueSize.x*0.5f) >= 1.62f){
                position.x = 1.62f - sprite.trueSize.x*0.5f;
            }
        }
    }
    bool collision_check(Entity &e){
        if((abs(e.position.x-position.x) - (1.0f/2.0f)*(e.sprite.trueSize.x+sprite.trueSize.x))<0 &&(abs(e.position.y-position.y) - (1.0f/2.0f)*(e.sprite.trueSize.y+sprite.trueSize.y))<0){
            return true;
        }else
            return false;
        
    }
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec2 size;
    string type;
    SheetSprite sprite;
    float health;
};

struct GameState {
    vector<Entity> enemies;
    vector<Entity> bullets;
    vector<Entity> entities;
    int score;
};

GameState state;



void setUp(){
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Welcome to Space Invader :)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 590, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    glViewport(0, 0, 960, 590);
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projectionMatrix = glm::ortho(-1.62f, 1.62f, -1.0f, 1.0f, -1.0f, 1.0f);
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    program.SetModelMatrix(modelMatrix);
    
    fontTexture = LoadTexture(RESOURCE_FOLDER"pixel_font.png");
    spriteTexture = LoadTexture(RESOURCE_FOLDER"sprites.png");
    
    glUseProgram(program.programID);
    program.SetColor(1.0f, 0.5f, 0.0f,1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(1.0f, 0.5f, 0.0f, 1.0f);
}

void processEvents() {
    switch (mode) {
        case STATE_MAIN_MENU:
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                    done = true;
                }else if(event.type == SDL_KEYDOWN){
                    if(event.key.keysym.scancode == SDL_SCANCODE_RETURN){
                        mode = STATE_GAME_LEVEL;
                    }
                }
            }
            break;
        case STATE_GAME_LEVEL:
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                    done = true;
                }
                else if(event.key.keysym.scancode == SDL_SCANCODE_SPACE && bulletTimer >=0.5) {
                    state.bullets[bulletIndex].position.x = state.entities[0].position.x;
                    state.bullets[bulletIndex].position.y = state.entities[0].position.y;
                    state.bullets[bulletIndex].velocity.y = 3.0f;
                    bulletIndex++;
                    if(bulletIndex > MAX_BULLETS){
                        bulletIndex = 0;
                    }
                    bulletTimer = 0;
                }
            break;
        case STATE_GAME_OVER:
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                    done = true;
                }
            }
            break;
        }
    }
}

void update(float elapsed) {
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    modelMatrix = glm::mat4(1.0f);
    
    switch (mode) {
        case STATE_MAIN_MENU:
            break;
        case STATE_GAME_LEVEL:
            if(keys[SDL_SCANCODE_LEFT] ){
                state.entities[0].velocity.x = -1.5f;
            }else if(keys[SDL_SCANCODE_RIGHT]){
                state.entities[0].velocity.x = 1.5f;
            }else{
                state.entities[0].velocity.x = 0.0f;
            }
            
            state.entities[0].update(elapsed);
            
            for(Entity &e: state.enemies){
                if(e.position.y < 1000){
                     if((e.position.x - e.sprite.trueSize.x*0.5f)>= -1.62f){
                        e.velocity.x = 0.1f;
                    }
                    else if((e.position.x+e.sprite.trueSize.x*0.5f) <= 1.62f){
                        e.velocity.x = -0.1f;
                    }else{
                        e.velocity.x = 0.0f;
                    }
                }
                if ((e.position.x-(e.sprite.trueSize.x*0.5f) <= -1.62)){
                    for(Entity &enemy: state.enemies){
                        enemy.position.y -= 0.07f;
                        enemy.position.x += 0.3f;
                    }
                }else if(e.position.x+(e.sprite.trueSize.x*0.5f) >= 1.62f){
                    for(Entity &enemy: state.enemies){
                        enemy.position.y -= 0.07f;
                        enemy.position.x -= 0.3f;
                    }
                }
            }
            for(Entity &e: state.enemies){
                for(Entity &b: state.bullets){
                    if(b.collision_check(e)){
                        //                        cout << "true\n";
                        e.position.y = 1000.0f;
                        b.position.x = -1000.0f;
                        state.score++;
                    }
                    
                }
                if(e.collision_check(state.entities[0])){
                    mode = STATE_GAME_OVER;
                }
                if((e.position.y - 0.5f*e.sprite.trueSize.y) <= -1.0f){
                    mode = STATE_GAME_OVER;
                }
            }
            
            for(Entity &b: state.bullets){
                b.update(elapsed);
            }
            for(Entity &e: state.enemies){
                e.update(elapsed);
                
            }
            bulletTimer += elapsed;
            if(state.score == 7){
                mode = STATE_GAME_OVER;
            }
            
        case STATE_GAME_OVER:
            break;
    }
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    switch (mode) {
        case STATE_MAIN_MENU:
            DrawText(program, fontTexture, "Welcome to Space Invadors <3 ", -1.1f,0.0f,0.07f, 0.01f);
            DrawText(program, fontTexture, "HINT: Press left/Right to move and Space to shoot ", -1.4f,-0.2f, 0.05f, 0.01f);
            DrawText(program, fontTexture, "Press Enter To Start", -1.1f,-0.4f, 0.05f, 0.01f);
            break;
        case STATE_GAME_LEVEL:
            DrawText(program, fontTexture, "Score: "+to_string(state.score), -1.33,0.95,0.05,0.01);
            for(Entity &e: state.entities){
                e.draw(program);
            }
            for(Entity &e: state.enemies){
                e.draw(program);
            }
            for(Entity &e: state.bullets){
                e.draw(program);
            }
            break;
        case STATE_GAME_OVER:
            if(state.score == 7){
                DrawText(program, fontTexture, "Awwwww! You Win >w<", -1.12f,0.0f,0.05f, 0.01f);
                DrawText(program, fontTexture, "Awesome ®Bakugo <3",-1.12f,-0.4f, 0.05f, 0.01f);

            }
            else{
                DrawText(program, fontTexture, "YOU DIED huh",-1.12f,-0.0f, 0.05f, 0.01f);
                DrawText(program, fontTexture, "GAME OVER @~@",-1.12f,-0.4f, 0.05f, 0.01f);
            }
            break;
    }
}

int main(int argc, char *argv[])
{
    setUp();
    //   SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size);
    SheetSprite myBakugo = SheetSprite(spriteTexture, 518.0f/1024.0f, 0.0f/512.0f, 257.0f/1024.0f, 301.0f/512.0f, 0.4f);
    SheetSprite myEnemy = SheetSprite(spriteTexture, 0.0f/1024.0f, 0.0f/512.0f, 257.0f/1024.0f, 301.0f/512.0f, 0.2f);
    SheetSprite myBullet = SheetSprite(spriteTexture, 259.0f/1024.0f, 0.0f/512.0f, 257.0f/1024.0f, 301.0f/512.0f, 0.2f);
    
//Entity(const string& Type, SheetSprite& input_sprite, float x, float y, float v_x, float v_y, float size_x, float size_y)
    Entity player = Entity("player",myBakugo,0.0f,-0.7f,3.0f,0.0f,1.0f,1.7f);
    state.entities.push_back(player);
    
    float x = -1.5;
    float y = 0.3;
    for(int i = 0; i < 3; i ++){
        for(int j = 0; j < 10; j++){
            state.enemies.push_back(Entity("enemy",myEnemy,x,y,0.0f,0.0f,1.7f,2.0f));
            x += 0.3;
        }
        x = -1.5;
        y += 0.2;
    }

    for(int i = 0; i < MAX_BULLETS; i++){
        state.bullets.push_back(Entity("bullet",myBullet,0.0f,-1.5f,0.0f,0.0f,1.0f,1.7f));
    }
    while (!done) {
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        elapsed += accumulator;
        if(elapsed < FIXED_TIMESTEP) {
            accumulator = elapsed;
            continue;
            
        }
        while(elapsed >= FIXED_TIMESTEP) {
            update(FIXED_TIMESTEP);
            elapsed -= FIXED_TIMESTEP;
        }
        accumulator = elapsed;
        
        processEvents();
//        update();
        render();
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
