#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
unsigned int loadTexture(char const * path);
unsigned int loadCubemap(vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;

    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;

    glm::vec3 position = glm::vec3(0.0f);
    float scale = 1.0f;

    PointLight pointLight;
    PointLight lampPointLight1;
    PointLight lampPointLight2;
    DirLight dirLight;

    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);
    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n'
        << position.x << '\n'
        << position.y << '\n'
        << position.z << '\n'
        << scale << '\n'
        << pointLight.constant << '\n'
        << pointLight.linear << '\n'
        << pointLight.quadratic << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z
           >> position.x
           >> position.y
           >> position.z
           >> scale
           >> pointLight.constant
           >> pointLight.linear
           >> pointLight.quadratic;
    }
}

ProgramState *programState;
void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "cabin", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);

    // enable face culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    // build and compile shaders
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");

    //initializing vertices (first three coordinates, second three normals, and two for textures)
    float vertices1[] = {
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
    };
    float vertices2[] = {
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,0.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    };
    float vertices3[] = {
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    };
    float vertices4[] = {
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    };
    float vertices5[] = {
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
    };
    float vertices6[] = {
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,0.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,0.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,0.0f, 0.0f,
    };
    float skyboxVertices[] = {
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };
    float platformVertices[] = {
            1.0f, 0.0f,  1.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, 0.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            -1.0f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f,

            1.0f, 0.0f,  1.0f,  0.0f, 1.0f, 0.0f,1.0f, 0.0f,
            -1.0f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f,0.0f, 1.0f,
            1.0f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f,1.0f, 1.0f
    };
    float roofVertices[] = {
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 1.0f,

            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f,

            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f,0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.5f, 1.0f,

            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 1.0f,

            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.5f, 1.0f,

            -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f
    };

    //initializing roof VAO and VBO
    unsigned int roofVAO, roofVBO;
    glGenVertexArrays(1, &roofVAO);
    glGenBuffers(1, &roofVBO);
    glBindBuffer(GL_ARRAY_BUFFER, roofVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(roofVertices), roofVertices, GL_STATIC_DRAW);
    glBindVertexArray(roofVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // initializing VAOs and VBOs for the room
    unsigned int VBO, cubeVAO1, cubeVAO2, cubeVAO3, cubeVAO4, cubeVAO5, cubeVAO6,  cubeVAOP1, cubeVAOP2, cubeVAOP3;
    glGenVertexArrays(1, &cubeVAO1);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices1), vertices1, GL_STATIC_DRAW);
    glBindVertexArray(cubeVAO1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &cubeVAO2);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);
    glBindVertexArray(cubeVAO2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &cubeVAO3);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices3), vertices3, GL_STATIC_DRAW);
    glBindVertexArray(cubeVAO3);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &cubeVAO4);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices4), vertices4, GL_STATIC_DRAW);
    glBindVertexArray(cubeVAO4);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &cubeVAO5);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices5), vertices5, GL_STATIC_DRAW);
    glBindVertexArray(cubeVAO5);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &cubeVAO6);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices6), vertices6, GL_STATIC_DRAW);
    glBindVertexArray(cubeVAO6);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    //initializing VAO and VBOs for auxiliary walls
    glGenVertexArrays(1, &cubeVAOP1);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);
    glBindVertexArray(cubeVAOP1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &cubeVAOP2);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices3), vertices3, GL_STATIC_DRAW);
    glBindVertexArray(cubeVAOP2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    //initializing VAO and VBO for the kitchen half-wall
    glGenVertexArrays(1, &cubeVAOP3);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices4), vertices4, GL_STATIC_DRAW);
    glBindVertexArray(cubeVAOP3);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    //initializing VAO and VBO for skybox
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    //initializing VAO and VBO for skybox
    unsigned int platformVAO, platformVBO;
    glGenVertexArrays(1, &platformVAO);
    glGenBuffers(1, &platformVBO);
    glBindVertexArray(platformVAO);
    glBindBuffer(GL_ARRAY_BUFFER, platformVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(platformVertices), &platformVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    stbi_set_flip_vertically_on_load(false);

    //loading textures
    unsigned int floor = loadTexture(FileSystem::getPath("resources/textures/floor/laminate_floor_02_diff_4k.jpg").c_str());
    unsigned int wall = loadTexture(FileSystem::getPath("resources/textures/wall/wood_plank_wall_diff_4k.jpg").c_str());
    unsigned int grass = loadTexture(FileSystem::getPath("resources/textures/grass/forrest_ground_01_diff_4k.jpg").c_str());
    unsigned int roof = loadTexture(FileSystem::getPath("resources/textures/roof/thatch_roof_angled_diff_4k.jpg").c_str());
    vector<std::string> faces {
            FileSystem::getPath("resources/textures/skybox/right.jpg"),
            FileSystem::getPath("resources/textures/skybox/left.jpg"),
            FileSystem::getPath("resources/textures/skybox/top.jpg"),
            FileSystem::getPath("resources/textures/skybox/bottom.jpg"),
            FileSystem::getPath("resources/textures/skybox/front.jpg"),
            FileSystem::getPath("resources/textures/skybox/back.jpg")
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    //random generating positions for trees
    vector<glm::vec3> vegetation;
    for (int i = 0; i < 20; i++) {
        int rand_x = rand() % 40;
        int rand_z = rand() % 35;
        glm::vec3 pos = glm::vec3(rand_x, 0.0f, rand_z);
        vegetation.push_back(pos);
    }

    // loading models
    Model bed("resources/objects/bed/bed.obj");
    bed.SetShaderTextureNamePrefix("material.");

    Model kitchen("resources/objects/kitchen/kitchen.obj");
    kitchen.SetShaderTextureNamePrefix("material.");

    Model wardrobe("resources/objects/wardrobe/orman.obj");
    wardrobe.SetShaderTextureNamePrefix("material.");

    Model tableSet("resources/objects/tableSet/untitled.obj");
    tableSet.SetShaderTextureNamePrefix("material.");

    Model vase("resources/objects/flower/Scaniverse.obj");
    vase.SetShaderTextureNamePrefix("material.");

    Model rug("resources/objects/rug/rug.obj");
    rug.SetShaderTextureNamePrefix("material.");

    Model door("resources/objects/door/10057_wooden_door_v3_iterations-2.obj");
    door.SetShaderTextureNamePrefix("material.");

    Model frame("resources/objects/frame/dog2obj.obj");
    frame.SetShaderTextureNamePrefix("material.");

    Model lamp("resources/objects/lamp/Asta LG1.obj");
    lamp.SetShaderTextureNamePrefix("material.");

    Model lamp2("resources/objects/lamp/Asta LG1.obj");
    lamp2.SetShaderTextureNamePrefix("material.");

    Model tree("resources/objects/tree/tree2.obj");
    tree.SetShaderTextureNamePrefix("material.");

    //moon light
    DirLight& dirLight = programState->dirLight;
    dirLight.direction = glm::vec3(1.9f, -1.4f, -2.15f);
    dirLight.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    dirLight.diffuse = glm::vec3(0.1f, 0.1f, 0.1f);
    dirLight.specular = glm::vec3(0.5f, 0.5f, 0.5f);

    //lamp lights
    PointLight& lampPointLight1 = programState->lampPointLight1;
    lampPointLight1.position = glm::vec3(0.984, 0.882, -3.268);
    lampPointLight1.ambient = glm::vec3(0.6f, 0.6f, 0.6f);
    lampPointLight1.diffuse = glm::vec3(0.6, 0.6, 0.6);
    lampPointLight1.specular = glm::vec3(0.4, 0.4, 0.4);
    lampPointLight1.constant = 1.0f;
    lampPointLight1.linear = 1.0f;
    lampPointLight1.quadratic = 1.0f;

    PointLight& lampPointLight2 = programState->lampPointLight2;
    lampPointLight2.position = glm::vec3(-0.984, 0.882, -3.268);
    lampPointLight2.ambient = glm::vec3(0.6f, 0.6f, 0.6f);
    lampPointLight2.diffuse = glm::vec3(0.6, 0.6, 0.6);
    lampPointLight2.specular = glm::vec3(0.4, 0.4, 0.4);
    lampPointLight2.constant = 1.0f;
    lampPointLight2.linear = 1.0f;
    lampPointLight2.quadratic = 1.0f;

    //shader config
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
    ourShader.use();
    ourShader.setInt("material.texture_diffuse1", 0);
    ourShader.setInt("material.texture_specular1", 1);

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // render
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();

        //forwarding information to shaders
        ourShader.setVec3("lampPointLight1.position", lampPointLight1.position);
        ourShader.setVec3("lampPointLight1.ambient", lampPointLight1.ambient);
        ourShader.setVec3("lampPointLight1.diffuse", lampPointLight1.diffuse);
        ourShader.setVec3("lampPointLight1.specular", lampPointLight1.specular);
        ourShader.setFloat("lampPointLight1.constant", lampPointLight1.constant);
        ourShader.setFloat("lampPointLight1.linear", lampPointLight1.linear);
        ourShader.setFloat("lampPointLight1.quadratic", lampPointLight1.quadratic);

        ourShader.setVec3("lampPointLight2.position", lampPointLight2.position);
        ourShader.setVec3("lampPointLight2.ambient", lampPointLight2.ambient);
        ourShader.setVec3("lampPointLight2.diffuse", lampPointLight2.diffuse);
        ourShader.setVec3("lampPointLight2.specular", lampPointLight2.specular);
        ourShader.setFloat("lampPointLight2.constant", lampPointLight2.constant);
        ourShader.setFloat("lampPointLight2.linear", lampPointLight2.linear);
        ourShader.setFloat("lampPointLight2.quadratic", lampPointLight2.quadratic);

        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        //dirLight
        //ourShader.setVec3("dirLight.direction", dirLight.direction);
        //ourShader.setVec3("dirLight.ambient", dirLight.ambient);
        //ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        //ourShader.setVec3("dirLight.specular", dirLight.specular);
        //for now
        ourShader.setVec3("dirLight.direction", 0.3f, -0.75f, -0.6f);
        ourShader.setVec3("dirLight.ambient", 0.1f, 0.1f, 0.1f);
        ourShader.setVec3("dirLight.diffuse", 0.25f, 0.25f, 0.25f);
        ourShader.setVec3("dirLight.specular", 0.3f, 0.3f, 0.3f);

        glDisable(GL_CULL_FACE);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render bed
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(0.0f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(0.9f));
        ourShader.setMat4("model", model);
        bed.Draw(ourShader);

        //render wardrobe
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(
                               3.0f, 0.0f, -2.27f));
        model = glm::scale(model, glm::vec3(1.3f));
        ourShader.setMat4("model", model);
        wardrobe.Draw(ourShader);

        //render kitchen
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(-2.2f, 0.46f, 3.0f));
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(0.45f));
        ourShader.setMat4("model", model);
        kitchen.Draw(ourShader);

        //render rug
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(-0.8f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(1.2f));
        ourShader.setMat4("model", model);
        rug.Draw(ourShader);

        //render tableSet
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(-2.4f, 0.0f, -1.8f));
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(0.011));
        ourShader.setMat4("model", model);
        tableSet.Draw(ourShader);

        //render door
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(3.5f, 0.0f, 2.5f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 0, 1));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(0.009));
        ourShader.setMat4("model", model);
        door.Draw(ourShader);

        //render frame
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(-3.68f, 1.2f, -1.8f));
        model = glm::rotate(model, glm::radians(-17.0f), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(1.2f));
        ourShader.setMat4("model", model);
        frame.Draw(ourShader);

        //render vase
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(-2.45f, 0.8f, -1.75f));
        model = glm::scale(model, glm::vec3(1.3f));
        ourShader.setMat4("model", model);
        vase.Draw(ourShader);

        //render lamps
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(-1.0f, 0.51f, -3.27f));
        model = glm::scale(model, glm::vec3(1.0f));
        ourShader.setMat4("model", model);
        lamp.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(1.0f, 0.51f, -3.27f));
        model = glm::scale(model, glm::vec3(1.0f));
        ourShader.setMat4("model", model);
        lamp2.Draw(ourShader);

        //room scaling
        model = glm::mat4(1);
        model = glm::translate(model, glm::vec3(0, 1.5, 0));
        model = glm::scale(model, glm::vec3(7, 3, 7));
        ourShader.setMat4("model", model);

        //draw room
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setMat4("model", model);
        glBindVertexArray(cubeVAO1);
        glBindTexture(GL_TEXTURE_2D, wall);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(cubeVAO2);
        glBindTexture(GL_TEXTURE_2D, wall);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(cubeVAO3);
        glBindTexture(GL_TEXTURE_2D, wall);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(cubeVAO4);
        glBindTexture(GL_TEXTURE_2D, wall);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(cubeVAO5);
        glBindTexture(GL_TEXTURE_2D, floor);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(cubeVAO6);
        glBindTexture(GL_TEXTURE_2D, wall);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //draw half-wall
        model = glm::mat4(1);
        model = glm::translate(model, glm::vec3(-1.51f, 1.48f, 1.76f));
        model = glm::scale(model, glm::vec3(7, 3, 3.5));
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setMat4("model", model);
        glBindVertexArray(cubeVAOP3);
        glBindTexture(GL_TEXTURE_2D, wall);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //draw auxiliary walls
        model = glm::mat4(1);
        model = glm::translate(model, glm::vec3(-0.1f, 1.5f, 0.02f));
        model = glm::scale(model, glm::vec3(7, 3, 7));
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setMat4("model", model);
        glBindVertexArray(cubeVAOP1);
        glBindTexture(GL_TEXTURE_2D, wall);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(cubeVAOP2);
        glBindTexture(GL_TEXTURE_2D, wall);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glEnable(GL_CULL_FACE);

        // draw skybox
        glCullFace(GL_FRONT);
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        //draw platform
        glCullFace(GL_BACK);
        ourShader.use();
        projection = glm::perspective(glm::radians(programState->camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        view = programState->camera.GetViewMatrix();
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.001f, 0.0f));
        model = glm::scale(model, glm::vec3(50.0f, 1.0f, 50.0f));
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        glBindVertexArray(platformVAO);
        glBindTexture(GL_TEXTURE_2D, grass);
        ourShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //draw roof
        glDisable(GL_CULL_FACE);
        ourShader.use();
        model = glm::mat4(1);
        model = glm::translate(model, glm::vec3(0.0f, 6.2f, 0.1f));
        model = glm::scale(model, glm::vec3(8.05));
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setMat4("model", model);
        glBindVertexArray(roofVAO);
        glBindTexture(GL_TEXTURE_2D, roof);
        glDrawArrays(GL_TRIANGLES, 0, 12);
        glEnable(GL_CULL_FACE);

//        draw trees
//        model = glm::scale(model, glm::vec3(1.0f));
//        ourShader.setMat4("model", model);
//        for (auto i : vegetation)
//        {
//            model = glm::mat4(1.0f);
//            model = glm::translate(model, i);
//            ourShader.setMat4("model", model);
//            tree.Draw(ourShader);
//        }

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.

    // de-allocation
    glDeleteVertexArrays(1, &cubeVAO1);
    glDeleteVertexArrays(1, &cubeVAO2);
    glDeleteVertexArrays(1, &cubeVAO3);
    glDeleteVertexArrays(1, &cubeVAO4);
    glDeleteVertexArrays(1, &cubeVAO5);
    glDeleteVertexArrays(1, &cubeVAO6);
    glDeleteVertexArrays(1, &cubeVAOP1);
    glDeleteVertexArrays(1, &cubeVAOP2);
    glDeleteVertexArrays(1, &cubeVAOP3);
    glDeleteVertexArrays(1, &roofVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &roofVBO);

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(DOWN, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
//        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
//        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
//        ImGui::DragFloat3("position", (float*)&programState->position);
//        ImGui::DragFloat("scale", &programState->scale, 0.05, 0.0001, 50.0);

//        ImGui::DragFloat3("dirLight.direction", (float*)&programState->dirLight.direction, 0.05);
//        ImGui::DragFloat3("dirLight.ambient", (float*)&programState->dirLight.ambient, 0.05, 0.0, 1.0);
//        ImGui::DragFloat3("dirLight.diffuse", (float*)&programState->dirLight.diffuse, 0.05, 0.0, 1.0);
//        ImGui::DragFloat3("dirLight.specular", (float*)&programState->dirLight.specular, 0.05, 0.0, 1.0);

//        ImGui::DragFloat3("lampPointLight1.position", (float*)&programState->lampPointLight1.position);
//        ImGui::DragFloat3("lampPointLight1.ambient", (float*)&programState->lampPointLight1.ambient, 0.05);
//        ImGui::DragFloat3("lampPointLight1.diffuse", (float*)&programState->lampPointLight1.diffuse, 0.05);
//        ImGui::DragFloat3("lampPointLight1.specular", (float*)&programState->lampPointLight1.specular, 0.05);
//        ImGui::DragFloat("pointLight.constant", &programState->lampPointLight1.constant, 0.05);
//        ImGui::DragFloat("pointLight.linear", &programState->lampPointLight1.linear, 0.05);
//        ImGui::DragFloat("pointLight.quadratic", &programState->lampPointLight1.quadratic, 0.05);
//
//        ImGui::DragFloat3("lampPointLight2.position", (float*)&programState->lampPointLight2.position);
//        ImGui::DragFloat3("lampPointLight2.ambient", (float*)&programState->lampPointLight2.ambient, 0.05);
//        ImGui::DragFloat3("lampPointLight2.diffuse", (float*)&programState->lampPointLight2.diffuse, 0.05);
//        ImGui::DragFloat3("lampPointLight2.specular", (float*)&programState->lampPointLight2.specular, 0.05);
//        ImGui::DragFloat("pointLight.constant", &programState->lampPointLight2.constant, 0.05);
//        ImGui::DragFloat("pointLight.linear", &programState->lampPointLight2.linear, 0.05);
//        ImGui::DragFloat("pointLight.quadratic", &programState->lampPointLight2.quadratic, 0.05);

        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

unsigned int loadTexture(char const * path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}