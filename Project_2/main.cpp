/**
* Author: Yannairis Cruz
* Assignment: Pong Clone
* Date due: 2025-3-01, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"                
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"              
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

// Our window dimensions
constexpr int WINDOW_WIDTH = 640*1.5,
WINDOW_HEIGHT = 480*1.5;


// Background color components
constexpr float BG_RED = 0.0f,
BG_BLUE = 0.0f,
BG_GREEN = 0.0f,
BG_OPACITY = 0.0f;

// Our viewport—or our "camera"'s—position and dimensions
constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;



//shader filepaths;
constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr GLint LEVEL_OF_DETAIL = 0, TEXTURE_BORDER = 0, NUMBER_OF_TEXTURES = 1;

//image files
constexpr char paddle_filepath[] = "paddle.png",
ball_filepath[] = "updated_ball.png";

glm::mat4 g_view_matrix, g_projection_matrix,
g_paddle_left_matrix, g_paddle_right_matrix, g_ball_matrix;

//paddle variables
glm::vec3 right_paddle_pos = glm::vec3(4.75, 0.0f, 0.0f),
left_paddle_pos = glm::vec3(-4.75f, 0.0f, 0.0f);

glm::vec3 right_paddle_mov = glm::vec3(0.0f, 0.0f, 0.0f),
left_paddle_mov = glm::vec3(0.0f, 0.0f, 0.0f);
float p_width = .75f, p_height = 1.8f, p_speed = 3.5f;


//ball info
glm::vec3 ball_position = glm::vec3(0.0f,0.0f,0.0f);
glm::vec3 ball_mov = glm::vec3(1.0f, 0.2f, 0.0f);
float b_width = 0.5f, b_height = 0.5f, b_speed = 3.5;

//player mode
bool two_player = true;

//texture ids
GLuint g_paddle_texture_id, g_ball_texture_id;

constexpr float millisec_in_sec = 1000.0;
float g_previous_ticks = 0.0f;

float screen_top = 3.75;
float screen_bottom = -3.75;

AppStatus g_app_status = RUNNING;
SDL_Window* g_display_window;
ShaderProgram g_shader_program = ShaderProgram();

GLuint load_texture(const char* filepath) {
    int width, height, number_of_componets;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_componets, STBI_rgb_alpha);
    if (image == NULL) {
        LOG("unable to load image. Check filepath");
        assert(false);
    }


    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);

    return textureID;

}

void initialise()
{
    // HARD INITIALISE ———————————————————————————————————————————————————————————————————
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Pone Clone",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    if (g_display_window == nullptr)
    {
        std::cerr << "ERROR: SDL Window could not be created.\n";
        g_app_status = TERMINATED;

        SDL_Quit();
        exit(1);
    }

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

  
    // SOFT INITIALISE ———————————————————————————————————————————————————————————————————
    // Initialise our camera
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    // Load up our shaders
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    //Initialise matrices
    g_view_matrix = glm::mat4(1.0f); 
    g_paddle_left_matrix = glm::mat4(1.0f);
    g_paddle_right_matrix = glm::mat4(1.0f);
    g_ball_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    

    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_paddle_texture_id = load_texture(paddle_filepath);
    g_ball_texture_id = load_texture(ball_filepath);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    const Uint8* key_state = SDL_GetKeyboardState(NULL);
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_t:
                two_player = false;
                break;
           
            }

        }
    }

    //left_paddle
    if (key_state[SDL_SCANCODE_W] && (left_paddle_pos.y + (p_height/2.0) < screen_top)) {
        left_paddle_mov.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_S] && (left_paddle_pos.y - (p_height / 2.0) > screen_bottom)) {
        left_paddle_mov.y = -1.0f;
    }
    else {
        left_paddle_mov.y = 0.0f;
    }
    if (glm::length(left_paddle_mov) > 1.0f) {
        left_paddle_mov = glm::normalize(left_paddle_mov);
    }

    //right_paddle
    if (two_player) {
        if (key_state[SDL_SCANCODE_UP] && (right_paddle_pos.y + (p_height / 2.0) < screen_top)) {
            right_paddle_mov.y = 1.0f;
        }
        else if (key_state[SDL_SCANCODE_DOWN] && (right_paddle_pos.y - (p_height / 2.0) > screen_bottom)) {
            right_paddle_mov.y = -1.0f;
        }
        else {
            right_paddle_mov.y = 0.0f;
        }
        if (glm::length(right_paddle_mov) > 1.0f) {
            right_paddle_mov = glm::normalize(right_paddle_mov);
        }
    }
    else {
        if (right_paddle_mov.y == 0) {
            right_paddle_mov.y = 1.0f;
        }
        if(right_paddle_pos.y > 0 && (right_paddle_pos.y + (p_height / 2.0) >= screen_top)){
            right_paddle_mov.y = -1.0f;
        }
        else if (right_paddle_mov.y < 0 && (right_paddle_pos.y - (p_height / 2.0) <= screen_bottom)) {
            right_paddle_mov.y = 1.0f;
        }

    }
}

void collision() {
    //if ball collides with paddle needs to bounce back in the opposite direction to negate it
    //if ball colllides with wall also needs to bounce but also the game should end?

    //got this number by trial and error 
    float collision_fix = 0.5f;
    float left_wall = -5.0f;
    float right_wall = 5.0f;

    float x_left_distance = fabs(ball_position.x - left_paddle_pos.x) - ((b_width  + (p_width - collision_fix)) / 2.0f);
    float y_left_distance = fabs(ball_position.y - left_paddle_pos.y) - ((b_height + (p_height - collision_fix)) / 2.0f);

    if (x_left_distance < 0 && y_left_distance < 0)
    {
        ball_mov.x = -ball_mov.x;
    }

    float x_right_distance = fabs(ball_position.x - right_paddle_pos.x) - ((b_width + (p_width - collision_fix)) / 2.0f);
    float y_right_distance = fabs(ball_position.y - right_paddle_pos.y) - ((b_height + (p_height - collision_fix)) / 2.0f);

    if (x_right_distance < 0 && y_right_distance < 0)
    {
        ball_mov.x = -ball_mov.x;
    }

    if (ball_position.y + b_height /2.0f >= screen_top) {
        ball_mov.y = -ball_mov.y;
    }
    if (ball_position.y - b_height / 2.0f <= screen_bottom) {
        ball_mov.y = -ball_mov.y;
    }

    //game stops 
    if (ball_position.x + b_width / 2.0f <= left_wall || ball_position.x + b_width / 2.0f >= right_wall) {
        g_app_status = TERMINATED;
    }
}


void update() {
    float ticks = (float)SDL_GetTicks() / millisec_in_sec;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    left_paddle_pos += left_paddle_mov * p_speed * delta_time;
    g_paddle_left_matrix = glm::translate(glm::mat4(1.0f),left_paddle_pos);
    g_paddle_left_matrix = glm::scale(g_paddle_left_matrix, glm::vec3(p_width, p_height, 1.0f));

    right_paddle_pos += right_paddle_mov * p_speed * delta_time;
    g_paddle_right_matrix = glm::translate(glm::mat4(1.0f), right_paddle_pos);
    g_paddle_right_matrix = glm::scale(g_paddle_right_matrix, glm::vec3(p_width, p_height, 1.0f));

    ball_position += ball_mov * b_speed * delta_time;
    g_ball_matrix = glm::translate(glm::mat4(1.0f), ball_position);
    g_ball_matrix = glm::scale(g_ball_matrix, glm::vec3(b_width, b_height, 0.0f));

    collision();
}

void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float texture_coordinates[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    draw_object(g_paddle_left_matrix, g_paddle_texture_id);
    draw_object(g_paddle_right_matrix, g_paddle_texture_id);
    draw_object(g_ball_matrix, g_ball_texture_id);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() 
{ 
    SDL_Quit(); 
}

/**
 Start here—we can see the general structure of a game loop without worrying too much about the details yet.
 */
int main(int argc, char* argv[])
{
    // Initialise our program—whatever that means
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();  // If the player did anything—press a button, move the joystick—process it
        update();         // Using the game's previous state, and whatever new input we have, update the game's state
        render();         // Once updated, render those changes onto the screen
    }

    shutdown();  // The game is over, so let's perform any shutdown protocols
    return 0;
}