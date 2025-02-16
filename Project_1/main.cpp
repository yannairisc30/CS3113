/**
* Author: Yannairis Cruz
* Assignment: Simple 2D Scene
* Date due: 2025-02-15, 11:59pm
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
#include "glm/mat4x4.hpp"                // 4x4 Matrix
#include "glm/gtc/matrix_transform.hpp"  // Matrix transformation methods
#include "ShaderProgram.h"               // We'll talk about these later in the course
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

// Our window dimensions
constexpr int WINDOW_WIDTH = 640,
               WINDOW_HEIGHT = 480;

// Background color components
constexpr float BG_RED = 0.1922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

// Our viewport—or our "camera"'s—position and dimensions
constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// Our shader filepaths; these are necessary for a number of things
// Not least, to actually draw our shapes 
// We'll have a whole lecture on these later
constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

//used for delta time calculations
constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, LEVEL_OF_DETAIL = 0,TEXTURE_BORDER = 0;

//image files
constexpr char PERRY_SPRITE_FILEPATH[] = "perry.png",
               FERB_SPRITE_FILEPATH[] = "ferb.png";

constexpr glm::vec3 INIT_SCALE = glm::vec3(5.0f, 5.9f, 0.0f),
                    INIT_POS_PERRY = glm::vec3(2.0f, 0.0f, 0.0f),
                    INIT_POS_FERB = glm::vec3(-2.0f, 0.0f, 0.0f);
 
//rotation speed
constexpr float ROT_INCREMENT = 1.0f;
//delta_time
float g_previous_ticks = 0.0f;
float g_ferb_x = 0.0f;
float g_perry_x = 0.0f;
int g_frame_counter = 0;
bool g_is_growing = true;

//translate variables and scale
constexpr float TRAN_VALUE = 1.0f;
constexpr float G_GROWTH_FACTOR = 1.01f;
constexpr float G_SHRINK_FACTOR = 0.99f;
constexpr int   G_MAX_FRAME = 40;


AppStatus g_app_status = RUNNING;
SDL_Window* g_display_window;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix, g_perry_matrix, g_ferb_matrix, g_projection_matrix;

glm::vec3 g_rotation_perry = glm::vec3(0.0f, 0.0f, 0.0f),
          g_rotation_ferb = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_perry_texture_id, g_ferb_texture_id;


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

    g_display_window = SDL_CreateWindow("Hello, Project1!",
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

    // ———————————————————————————————————————————————————————————————————————————————————

    // SOFT INITIALISE ———————————————————————————————————————————————————————————————————
    // Initialise our camera
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    // Load up our shaders
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    // Initialise our matrices
    g_perry_matrix = glm::mat4(1.0f);
    g_ferb_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    // Each object has its own unique ID
    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_perry_texture_id = load_texture(PERRY_SPRITE_FILEPATH);
    g_ferb_texture_id = load_texture(FERB_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}

void update()
{
    g_frame_counter++;

    //Delta time calculations
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    glm::vec3 translation_vector;
    glm::vec3 scale_vector; 

 

    if(g_frame_counter >= G_MAX_FRAME)
    {
        g_is_growing = !g_is_growing;
        g_frame_counter = 0;
    }

    g_rotation_perry.y += ROT_INCREMENT * delta_time;
    g_rotation_ferb.y += -1 * ROT_INCREMENT * delta_time;

    g_ferb_x += 1.0f * delta_time;
    g_perry_x += 1.0f * delta_time;

    translation_vector = glm::vec3(TRAN_VALUE * delta_time, TRAN_VALUE * delta_time, 0.0f);
    scale_vector = glm::vec3(g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
        g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
        1.0f);

    g_perry_matrix = glm::mat4(1.0f);
    g_ferb_matrix = glm::mat4(1.0f);

    g_perry_matrix = glm::translate(g_perry_matrix,translation_vector);
    g_perry_matrix = glm::rotate(g_perry_matrix, g_rotation_perry.y, glm::vec3(0.0f, 1.0f, 0.0f));
    //g_perry_matrix = glm::scale(g_perry_matrix, scale_vector);

    g_ferb_matrix = glm::translate(g_perry_matrix, glm::vec3(1.0f * glm::cos(g_perry_x), 1.0f * glm::sin(g_perry_x), 0.0f));
    //g_ferb_matrix = glm::rotate(g_ferb_matrix, g_rotation_ferb.y, glm::vec3(0.0f, 1.0f, 0.0f));
    //g_ferb_matrix = glm::scale(g_ferb_matrix, INIT_SCALE);

}

void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    /*g_shader_program.set_model_matrix(g_model_matrix);*/

    float vertices[] =
    {
         -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,
         -0.5f, -0.5f, 0.5f,   0.5f, -0.5f, 0.5f
    };

    float texture_coordinates[] =
    {
        0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    draw_object(g_perry_matrix, g_perry_texture_id);
    draw_object(g_ferb_matrix, g_ferb_texture_id);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

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