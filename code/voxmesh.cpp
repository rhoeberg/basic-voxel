/*
  
  - TODO multiple chunks

  - TODO different materials for the different voxels
    perhaps chunking together voxels of the same type
  
*/
#define Kilobytes(Value) ((Value)*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define new_max(x,y) ((x) >= (y)) ? (x) : (y)
#define new_min(x,y) ((x) <= (y)) ? (x) : (y)
#define array_size(x) (sizeof(x) / sizeof(x[0]))

// opengl
#include "GL/glew.h"
#include <GLFW/glfw3.h>

// imgui
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"
#include "imgui/imgui_demo.cpp"

// math library
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/normal.hpp>

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/mman.h>

#define STB_DEFINE                                                     
#include "stb.h"

// image loading
#include <SOIL/SOIL.h>

struct voxel{
    glm::mat4 model;
    glm::vec4 col;
};
struct VoxelMesh
{
    glm::vec3 frontTopRight;
    glm::vec3 frontBottomRight;
    glm::vec3 frontBottomLeft;
    glm::vec3 frontTopLeft;

    glm::vec3 backTopRight;
    glm::vec3 backBottomRight;
    glm::vec3 backBottomLeft;
    glm::vec3 backTopLeft;
};

void add_voxel_face(glm::vec3 c1, glm::vec3 c2, glm::vec3 c3, glm::vec3 c4, glm::vec3 color, glm::vec3 normal);
// void add_voxel(VoxelMesh mesh, glm::vec3 color);
GLuint get_chunk_vbo();
void cam_movement();
void move_on_spot();
static void error_callback(int error, const char* description) {fprintf(stderr, "Error: %s\n", description);}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
glm::vec3 get_cursor_pos(GLFWwindow *window);
void toggle_fullscreen();
void cleanup();

GLuint shader;
GLuint textureShader;
GLuint rectangle;
GLuint unfilledRectangle;
GLuint sphereTex;
GLuint sphere;
GLuint vox;

GLuint lightShader;
glm::vec3 lightPos;

bool keys[1024];
double mouseX, mouseY;
GLfloat lastX, lastY;
bool mouseButtons[12];

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

GLFWwindow *win;
int screenWidth = 1024, screenHeight = 768;
bool fullscreen = false;

// voxel map
int map_size = 50;
int map_height = 20;
GLfloat voxel_size = 1.0f;
float map_xoffset = 0;
float map_zoffset = 0;
glm::vec3 map_offset = {0,20,0};
bool map_auto_move = false;
int map_noise_octaves = 2;
float map_noise_persistance = 0.4f;
float map_noise_frequency = 0.02f;

GLfloat *vox_vertices;
GLuint *vox_indices;

int vox_size = 0; // TODO this should maybe not be global
int vox_face_size = 0; // same with this
GLuint chunk;

glm::vec3 water = {0.003921568627f * 60, 0.003921568627f * 136, 0.003921568627f * 221};
glm::vec3 dirt = {0.003921568627f * 214, 0.003921568627f * 175, 0.003921568627f * 136};
glm::vec3 grass = {0.003921568627f * 106, 0.003921568627f * 185, 0.003921568627f * 89};
glm::vec3 stone = {0.003921568627f * 160, 0.003921568627f * 160, 0.003921568627f * 160};
glm::vec3 snow = {0.003921568627f * 231, 0.003921568627f * 235, 0.003921568627f * 238};


#include "Camera.h"
// /////////////
// the framework
#include "snoise.c"

GLuint create_shader(const char *vPath, const char *fPath)
{
    GLuint shaderProgram;
    std::ifstream vShaderFile(vPath);
    std::ifstream fShaderFile(fPath);
    
    std::string vStream((std::istreambuf_iterator<char>(vShaderFile)), std::istreambuf_iterator<char>());
    std::string fStream((std::istreambuf_iterator<char>(fShaderFile)), std::istreambuf_iterator<char>());
    const char* vertexCode = vStream.c_str();
    const char* fragmentCode = fStream.c_str();

    GLint success;
    GLchar infoLog[512];
    GLuint vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexCode, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success) {
	glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
	std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    GLuint fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentCode, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success) {
	glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
	std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success) {
	glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void use_shader(glm::mat4 view, glm::mat4 projection, GLuint shader)
{
    glUseProgram(shader);
    GLuint viewLoc = glGetUniformLocation(shader, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    GLuint projectionLoc = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

Camera camera(glm::vec3(10.0f, 40.0f, 10.0f));

double OctavePerlin2(double x, double y, int octaves, double persistence, double frequency) {
    double total = 0;
    // double frequency = 1;
    double amplitude = 1;
    double maxValue = 0;  // Used for normalizing result to 0.0 - 1.0
    for(int i=0;i<octaves;i++) {
        total += sdnoise2(x * frequency, y * frequency, NULL, NULL) * amplitude;
        
        maxValue += amplitude;
        
        amplitude *= persistence;
        frequency *= 2;
    }
    
    return total/maxValue;
}

double OctavePerlin3(float x, float y, float z, int octaves, float persistence, float frequency) {
    double total = 0;
    // double frequency = 1;
    double amplitude = 1;
    double maxValue = 0;  // Used for normalizing result to 0.0 - 1.0
    for(int i=0;i<octaves;i++) {
        total += sdnoise3(x * frequency, y * frequency, z * frequency, NULL, NULL, NULL) * amplitude;
        
        maxValue += amplitude;
        
        amplitude *= persistence;
        frequency *= 2;
    }
    
    return total/maxValue;
}

// void print_mesh(VoxelMesh mesh)
// {
//     printf("fronTopRight:%f, %f, %f\n", mesh.frontTopRight.x, mesh.frontTopRight.y, mesh.frontTopRight.z);
// }

struct Voxel {
    bool active = false;
    glm::vec3 color;
};
// bool voxel_map[20][20][20];
// Voxel voxel_map[50][50][50];
Voxel *voxel_map;

// 1 2 3 4
// 5 6 7 8
// 5 6 7 8
// 5 6 7 8
int get_voxel_index(int x, int y, int z)
{
    return x + map_size * (y + map_size * z);
}


void generate_voxel_landscape()
{
    printf("generating voxel landscape\n");
    
    
    stb_arr_free(vox_vertices);
    stb_arr_free(vox_indices);
    stb_arr_free(voxel_map);
    // Voxel (*voxel_map)[map_size] = malloc(sizeof(Voxel[map_size][map_size]));
    // free(voxel_map);
    // stb_arr_free(voxel_map);

    stb_arr_setlen(voxel_map, map_size * map_size * map_size);
    for(int i = 0; i < stb_arr_len(voxel_map); i++) {
        voxel_map[i].active = false;
    }

    vox_size = 0;
    vox_face_size = 0;

    // generate noise map
    for(int x = 0; x < map_size; x++) {
        for(int z = 0; z < map_size; z++) {
            float n = OctavePerlin2(x + map_offset.x ,z + map_offset.z , map_noise_octaves, map_noise_persistance, map_noise_frequency) + 0.5f;
            // float posY = (int)(n * map_size);
            float posY = (int)(n * map_height);
            int y = posY;
            while(y > 0) {
                int index = x + map_size * (y + map_size * z);
                voxel_map[index].active = true;

                glm::vec3 newPos = { x - (map_size / 2), (int)y, z - (map_size / 2) };

                glm::vec3 color;
                if(newPos.y < 5) {color = water;} 
                else if(newPos.y < 7) {color = dirt;} 
                else if(newPos.y < 15) {color = grass;} 
                else if(newPos.y < 17) {color = stone;} 
                else {color = snow;}
                voxel_map[index].color = color;

                y--;
            }
        }
    }
    
    for(int x = 0; x < map_size; x++) {
        for(int y = 0; y < map_size; y++) {
            for(int z = 0; z < map_size; z++) {
                // if(voxel_map[x][y][z].active) {
                if(voxel_map[get_voxel_index(x,y,z)].active) {
                    vox_size++;
                    bool xPositive = true;
                    bool xNegative = true;
                    bool yPositive = true;
                    bool yNegative = true;
                    bool zPositive = true;
                    bool zNegative = true;

                    // VoxelMesh voxMesh;
                    GLfloat xPos = x * voxel_size;
                    GLfloat yPos = y * voxel_size;
                    GLfloat zPos = z * voxel_size;
                    GLfloat voxelHalfSize = voxel_size / 2;
                    glm::vec3 frontTopRight = { xPos + voxelHalfSize, yPos + voxelHalfSize,zPos + voxelHalfSize };
                    glm::vec3 frontBottomRight = { xPos + voxelHalfSize, yPos - voxelHalfSize,zPos + voxelHalfSize };
                    glm::vec3 frontBottomLeft = { xPos - voxelHalfSize, yPos - voxelHalfSize,zPos + voxelHalfSize };
                    glm::vec3 frontTopLeft = { xPos - voxelHalfSize, yPos + voxelHalfSize,zPos + voxelHalfSize };
                    glm::vec3 backTopRight = { xPos + voxelHalfSize, yPos + voxelHalfSize,zPos - voxelHalfSize };
                    glm::vec3 backBottomRight = { xPos + voxelHalfSize, yPos - voxelHalfSize,zPos - voxelHalfSize };
                    glm::vec3 backBottomLeft = { xPos - voxelHalfSize, yPos - voxelHalfSize,zPos - voxelHalfSize };
                    glm::vec3 backTopLeft = { xPos - voxelHalfSize, yPos + voxelHalfSize,zPos - voxelHalfSize };

                    // 0, 1, 3, 1, 2, 3, front
                    // 4, 5, 0, 5, 1, 0, right (btoprright bbottomright fronttopright
                    // 7, 6, 4, 6, 5, 4, back 
                    // 3, 2, 7, 2, 6, 7, left
                    // 7, 4, 0, 0, 3, 7, top
                    // 6, 5, 1, 1, 2, 6  bottom

                    //0, 1, 3,
                    //1, 2, 3
                    glm::vec3 color = voxel_map[get_voxel_index(x,y,z)].color;
                    if(x < map_size - 1) { // right
                        if(!voxel_map[get_voxel_index(x+1,y,z)].active) {
                            glm::vec3 normal = { -1,0,0 };
                            add_voxel_face(backTopRight, backBottomRight, frontBottomRight, frontTopRight, color, normal);
                        }
                    }
                    else {
                        glm::vec3 normal = { -1,0,0 };
                        add_voxel_face(backTopRight, backBottomRight, frontBottomRight, frontTopRight, color, normal);
                    }
                    if(x > 0) { // left
                        if(!voxel_map[get_voxel_index(x-1,y,z)].active) {
                            glm::vec3 normal = { 1,0,0 };
                            add_voxel_face(frontTopLeft, frontBottomLeft, backBottomLeft, backTopLeft, color, normal);
                        }
                    }
                    else {
                        glm::vec3 normal = { 1,0,0 };
                        add_voxel_face(frontTopLeft, frontBottomLeft, backBottomLeft, backTopLeft, color, normal);
                    }
                    if(y < map_size - 1) { // top
                        if(!voxel_map[get_voxel_index(x, y+1, z)].active)  {
                            glm::vec3 normal = { 0,1,0 };
                            add_voxel_face(backTopLeft, backTopRight, frontTopRight, frontTopLeft, color, normal);
                        }
                    }
                    else {
                        glm::vec3 normal = { 0,1,0 };
                        add_voxel_face(backTopLeft, backTopRight, frontTopRight, frontTopLeft, color, normal);
                    }
                    if(y > 0) { // bottom
                        if(!voxel_map[get_voxel_index(x, y-1, z)].active)  {
                            glm::vec3 normal = { 0,-1,0 };
                            add_voxel_face(backBottomLeft, backBottomRight, frontBottomRight, frontBottomLeft, color, normal);
                        }
                    }
                    else {
                        glm::vec3 normal = { 0,-1,0 };
                        add_voxel_face(backBottomLeft, backBottomRight, frontBottomRight, frontBottomLeft, color, normal);
                    }
                    if(z < map_size - 1) { // front
                        if(!voxel_map[get_voxel_index(x, y, z+1)].active)  {
                            glm::vec3 normal = { 0,0,1 };
                            add_voxel_face(frontTopRight, frontBottomRight, frontBottomLeft, frontTopLeft, color, normal);
                        }
                    }
                    else {
                        glm::vec3 normal = { 0,0,1 };
                        add_voxel_face(frontTopRight, frontBottomRight, frontBottomLeft, frontTopLeft, color, normal);
                    }
                    if(z > 0) { // back
                        if(!voxel_map[get_voxel_index(x, y, z-1)].active)  {
                            glm::vec3 normal = { 0,0,-1 };
                            add_voxel_face(backTopLeft, backBottomLeft, backBottomRight, backTopRight, color, normal);
                        }
                    }
                    else {
                        glm::vec3 normal = { 0,0,-1 };
                        add_voxel_face(backTopLeft, backBottomLeft, backBottomRight, backTopRight, color, normal);
                    }
                }
            }
        }
    }
    
    chunk = get_chunk_vbo();
}

void add_voxel_face(glm::vec3 c1, glm::vec3 c2, glm::vec3 c3, glm::vec3 c4, glm::vec3 color, glm::vec3 normal) {
    vox_face_size++;
    const GLfloat vertices[] = {
        c1.x, c1.y, c1.z, color.x, color.y, color.z, normal.x, normal.y, normal.z,
        c2.x, c2.y, c2.z, color.x, color.y, color.z, normal.x, normal.y, normal.z,   
        c3.x, c3.y, c3.z, color.x, color.y, color.z, normal.x, normal.y, normal.z, 
        c4.x, c4.y, c4.z, color.x, color.y, color.z, normal.x, normal.y, normal.z
    };
    for(int i = 0; i < sizeof(vertices) / sizeof(GLfloat); i++) {
        stb_arr_push(vox_vertices, vertices[i]);
    }
    const GLuint indices[] = {
        0, 1, 3,
        1, 2, 3
    };
    for(int i = 0; i < sizeof(indices) / sizeof(GLuint); i++) {
        stb_arr_push(vox_indices, indices[i] + vox_face_size * 4);
    }
}

// void add_voxel(VoxelMesh mesh, glm::vec3 color)
// {
//     const GLfloat vertices[] = {
//         mesh.frontTopRight.x, mesh.frontTopRight.y, mesh.frontTopRight.z, color.x, color.y, color.z,
//         mesh.frontBottomRight.x, mesh.frontBottomRight.y, mesh.frontBottomRight.z, color.x, color.y, color.z,
//         mesh.frontBottomLeft.x, mesh.frontBottomLeft.y, mesh.frontBottomLeft.z, color.x, color.y, color.z,
//         mesh.frontTopLeft.x, mesh.frontTopLeft.y, mesh.frontTopLeft.z, color.x, color.y, color.z,
//         mesh.backTopRight.x, mesh.backTopRight.y, mesh.backTopRight.z, color.x, color.y, color.z,
//         mesh.backBottomRight.x, mesh.backBottomRight.y, mesh.backBottomRight.z, color.x, color.y, color.z,
//         mesh.backBottomLeft.x, mesh.backBottomLeft.y, mesh.backBottomLeft.z, color.x, color.y, color.z,
//         mesh.backTopLeft.x, mesh.backTopLeft.y, mesh.backTopLeft.z, color.x, color.y, color.z
//     };
    
//     int vertLength = sizeof(vertices) / sizeof(GLfloat);
//     for(int i = 0; i < vertLength; i++) {
//         stb_arr_push(vox_vertices, vertices[i]);
//     }
    
//     const GLuint indices[] = {
//         0, 1, 3, 1, 2, 3, // front
//         4, 5, 0, 5, 1, 0, // right 
//         7, 6, 4, 6, 5, 4, // back
//         3, 2, 7, 2, 6, 7, // left
//         7, 4, 0, 0, 3, 7, // top
//         6, 5, 1, 1, 2, 6  // bottom
//     };

//     int indiceLength = sizeof(indices) / sizeof(GLuint);
//     for(int i = 0; i < indiceLength; i++) {
//         stb_arr_push(vox_indices, indices[i] + vox_size * 8);
//     }
    
//     vox_size++;
// }

GLuint get_chunk_vbo()
{
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, stb_arr_len(vox_vertices) * sizeof(GLfloat), vox_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, stb_arr_len(vox_indices) * sizeof(GLuint), vox_indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}


int main(int argc, char *argv[])
{
    /////////////////
    // GL SETUP
    glfwSetErrorCallback(error_callback);
    if(!glfwInit()) {
        exit(EXIT_FAILURE);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    win = glfwCreateWindow(screenWidth, screenHeight, "test project", NULL, NULL);
    if(!win) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(win);
    glfwSetCursorPosCallback(win, mouse_callback);
    // glfwSetKeyCallback(win, key_callback);
    glewExperimental = GL_TRUE;
    glewInit();
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    glPointSize(20);
    int vWidth, vHeight;
    glfwGetFramebufferSize(win, &vWidth, &vHeight);
    glViewport(0, 0, vWidth, vHeight);

    // setup imgui
    ImGui_ImplGlfwGL3_Init(win, true);

    //////////////////
    // VOXEL SETUP
    shader = create_shader("assets/vertexShader.vert", "assets/fragmentShader.frag");
    lightShader = create_shader("assets/light.vert", "assets/light.frag");

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), ((float)screenWidth / (float)screenHeight), 0.1f, 1000.0f);
    glm::mat4 view;
    view = glm::rotate(view, 0.1f, glm::vec3(1.0f, 0.0f, 0.0f)); 
    view = glm::translate(view, glm::vec3(0.0f, -4.0f, -20.0f));
    
    // create voxels
    generate_voxel_landscape();

    // use_shader(view, projection, shader);

    use_shader(view, projection, lightShader);
    GLuint colorLoc = glGetUniformLocation(lightShader, "Color");
    GLuint lightColLoc = glGetUniformLocation(lightShader, "lightColor");
    GLuint lightPosLoc = glGetUniformLocation(lightShader, "lightPos");
    GLuint viewPosLoc = glGetUniformLocation(lightShader, "viewPos");
    GLuint modelLoc = glGetUniformLocation(lightShader, "model");
    // GLuint objColorLoc = glGetUniformLocation(shader, "objectColor");

    lightPos = {0,50,50};
    glm::vec3 lightColor = {1,1,1};
    GLfloat shader_light_R = 1.0f;
    GLfloat shader_light_G = 1.0f;
    GLfloat shader_light_B = 1.0f;
    glUniform3fv(lightColLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));

    while(!glfwWindowShouldClose(win))
    {
        glfwPollEvents();
        double currentTime= glfwGetTime();
        deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        static bool keydown = false;
        static bool camOn = true;
        if(keys[GLFW_KEY_C] && !keydown) {
            keydown = true;
            camOn = !camOn;

            if(camOn) {
                glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                lastX = mouseX;
                lastY = mouseY;
            }
            else {
                glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
        else if(!keys[GLFW_KEY_C] && keydown) {keydown = false;}
        if(camOn) {
            cam_movement();
        }


	if(keys[GLFW_KEY_F]) {
	    toggle_fullscreen();
	}

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplGlfwGL3_NewFrame();

        view = camera.GetViewMatrix();

        use_shader(view, projection, lightShader);
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(camera.Position));
        // use_shader(view, projection, shader);


        glBindVertexArray(chunk);
        glDrawElements(GL_TRIANGLES, stb_arr_len(vox_indices), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);




        
        double endTime = glfwGetTime();
        double fps = 1.0 / (endTime - currentTime);
        double frameDuration = 1000 * (endTime - currentTime);
        

        /////////
        // editor
        bool show = true;
        ImGui::Begin("performance", &show);
        ImGui::Text("fps: %f\n", fps);
        ImGui::Text("frame duration: %f\n", frameDuration);
        ImGui::Text("voxel size: %i\n", vox_size);
        ImGui::Text("indices size: %i\n", stb_arr_len(vox_indices));
        ImGui::Text("vertices size: %i\n", stb_arr_len(vox_vertices));
        ImGui::Text("cam pos\nx:%f\ny:%f\nz:%f", camera.Position.x, camera.Position.y, camera.Position.z);
        
        if(ImGui::Button("lightPos")) {
            camera.Position = lightPos;
        }
        ImGui::End();

        ImGui::Begin("landscape", &show);
        ImGui::InputInt("size", &map_size);
        ImGui::InputInt("map_height", &map_height);
        ImGui::InputFloat("voxel size", &voxel_size);
        if(ImGui::Button("generate")) {
            generate_voxel_landscape();
        }
        
        // ImGui::InputFloat("map_xoffset", &map_xoffset);
        // ImGui::InputFloat("map_zoffset", &map_zoffset);
        // ImGui::Checkbox("auto gen", &map_auto_move);
        // if(map_auto_move) {
        //     map_xoffset += 0.2f;
        //     map_zoffset += 0.2f;
        //     generate_voxel_landscape();
        // }
        
        ImGui::Text("NOISE");
        ImGui::InputInt("octaves", &map_noise_octaves);
        ImGui::InputFloat("persistance", &map_noise_persistance);
        ImGui::InputFloat("frequency", &map_noise_frequency);
        ImGui::End();
        
        ImGui::Begin("shader", &show);
        ImGui::ColorPicker3("shaderCol", glm::value_ptr(lightColor));
        // ImGui::InputFloat("r", &shader_light_R, 0.0f, 0.0f, 2);
        // ImGui::InputFloat("g", &shader_light_G, 0.0f, 0.0f, 2);
        // ImGui::InputFloat("b", &shader_light_B, 0.0f, 0.0f, 2);
        if(ImGui::Button("refresh shader")) {
            // glUniform3fv(lightColLoc, 1, glm::value_ptr(glm::vec3{shader_light_R,shader_light_G,shader_light_B}));
            glUniform3fv(lightColLoc, 1, glm::value_ptr(lightColor));
        }
        ImGui::End();
        
        



        ImGui::Render();

        glfwSwapBuffers(win);
    }
    printf("am i here?\n");
    cleanup();
    glfwTerminate();
    return 0;
}

void toggle_fullscreen()
{
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    if(fullscreen) {
	glfwSetWindowSize(win, mode->width, mode->height);
	glfwSetWindowPos(win, 0, 0);
	int vWidth, vHeight;
	glfwGetFramebufferSize(win, &vWidth, &vHeight);
	glViewport(0, 0, vWidth, vHeight);
    }
    else {
	int vWidth, vHeight;
	glfwGetFramebufferSize(win, &vWidth, &vHeight);
	glViewport(0, 0, vWidth, vHeight);
	glfwSetWindowSize(win, screenWidth, screenHeight);
    }

    fullscreen = !fullscreen;
}

void key_callback(GLFWwindow* window, int key, int action, int mode)
{
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    	glfwSetWindowShouldClose(window, GL_TRUE);

    if(action == GLFW_PRESS) {
        keys[key] = true;
    }
    else if(action == GLFW_RELEASE) {
        keys[key] = false;
    }
}  

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    mouseX = xpos;
    mouseY = ypos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action)
{
    if(action == GLFW_PRESS) {
        mouseButtons[button] = true;
    }
    else if(action == GLFW_RELEASE) {
        mouseButtons[button] = false;
    }
}

void mouse_movement()
{
    double map_xoffset = mouseX - lastX;
    double yoffset = lastY - mouseY; 
    
    lastX = mouseX;
    lastY = mouseY;

    camera.ProcessMouseMovement(map_xoffset, yoffset);
}

void cam_movement()
{
    mouse_movement();
    float speed = deltaTime * 5.0f;
    // Camera controls
    if(keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, speed);
    if(keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, speed);
    if(keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, speed);
    if(keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, speed);
}

void cleanup()
{
    stb_arr_free(vox_vertices);
    stb_arr_free(vox_indices);

    stb_arr_free(voxel_map);

    printf("done cleaning up\n");
}
