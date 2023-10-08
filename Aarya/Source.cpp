#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "Model.h"
#include <iostream>
#include "shader_m.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "camera.h"
using namespace std;
#include "Sphere.h"
#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
//#include <imgui/imgui.cpp>
////#include <imgui/imgui_draw.cpp>
////#include <imgui/imgui_widgets.cpp>
//#include <imgui/backends/imgui_impl_glfw.cpp>
//#include <imgui/backends/imgui_impl_opengl3.cpp>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
float f = 0.0f;
const char* glsl_version = "#version 330";
// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
bool useBlinnPhong = false;
// camera
Camera camera(glm::vec3(0.0f, 2.0f, 0.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool gravity = false;
bool picked = false;
int pickedObjectIndex = -1;
// timing
bool showInstructionCube = true;


float deltaTime = 0.0f;
float lastFrame = 0.0f;
struct PickableObject {
    Model model;
    glm::mat4 transformationMatrix;
    BoundingBox bb;

    PickableObject(const std::string& modelPath)
        : model(modelPath), transformationMatrix(glm::mat4(1.0f)) {
        bb = model.calculateBoundingBox();
    }
};
std::vector<PickableObject> pickableObjects;


unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    stbi_set_flip_vertically_on_load(false);
    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
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
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    return textureID;

}
bool isColliding(const BoundingBox& a, const BoundingBox& b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
        (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
        (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

int previousGState = GLFW_RELEASE;
int previousBState = GLFW_RELEASE;;
BoundingBox transformBoundingBox(const BoundingBox& bb, const glm::mat4& transformation) {
    glm::vec3 min = bb.min;
    glm::vec3 max = bb.max;

    glm::vec3 transformedMin = transformation * glm::vec4(min, 1.0f);
    glm::vec3 transformedMax = transformation * glm::vec4(max, 1.0f);
    BoundingBox r;
    r.min = transformedMin;
    r.max = transformedMax;
   

    return r;
}
class ScoreManager {
public:
    int score;
    int pickedObjectIndex; // To keep track of the currently picked object
    bool keyPicked;
    string text = "THE PLANET NEEDS YOU";
    
    std::vector<int> objectsToPick; // Vector of indices of objects that can be picked

    ScoreManager() : score(0), pickedObjectIndex(-1), keyPicked(false) {}
     
    void pickObject(int objectIndex) {
        if (std::find(objectsToPick.begin(), objectsToPick.end(), objectIndex) != objectsToPick.end()) {
            pickedObjectIndex = objectIndex;
            keyPicked = true;
           /* score += 10;*/
        }
    }

    void deliverToBench() {
        if (pickedObjectIndex != -1) {
            score += 50;
            keyPicked = false;
            remove(objectsToPick.begin(), objectsToPick.end(), pickedObjectIndex);
            pickedObjectIndex = -1; // Object has been delivered
        }
    }
        void notdeli() {
            if (pickedObjectIndex != -1) {
                score -= 10;
                pickedObjectIndex = -1; // Object has been delivered
            }
     
    }
};
ScoreManager scoreManager;
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS|| glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        useBlinnPhong = !useBlinnPhong;
    int currentBState = glfwGetKey(window, GLFW_KEY_B);
    if (previousBState == GLFW_RELEASE && currentBState == GLFW_PRESS){
        useBlinnPhong = !useBlinnPhong;
    }
    previousBState = currentBState;
    
    // Add a key for jumping. For example, let's use the space bar
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.Jump(gravity);

    // Process gravity every frame
    int currentGState = glfwGetKey(window, GLFW_KEY_G);
    if (previousGState == GLFW_RELEASE && currentGState == GLFW_PRESS) {
        gravity = !gravity;
    }
    previousGState = currentGState;
   

    camera.ApplyGravity(deltaTime, gravity);
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !picked) {
        for (int i = 0; i < pickableObjects.size(); i++) {
            BoundingBox cameraBB = camera.bb; // Assume this method exists
            BoundingBox objectBB = pickableObjects[i].bb;
            if (isColliding(cameraBB, objectBB)) { // Implement this collision check function
                pickedObjectIndex = i;
                picked = true;
                break;
            }
            else {
              std::  cout << "NO item nearby" << endl;
            }
        }
    }

   
    }








// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

int currentFace = 0; // Keep track of the current face being displayed

float instructionCubeRotationX = 0.0f;
float instructionCubeRotationY = 0.0f;


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        currentFace = (currentFace + 1) % 6;

        switch (currentFace) {
        case 0: // Face 1
            instructionCubeRotationY = 0.0f;
            instructionCubeRotationX = 0.0f;
            break;
        case 1: // Face 2
            instructionCubeRotationY = 90.0f;
            instructionCubeRotationX = 0.0f;
            break;
        case 2: // Face 3
            instructionCubeRotationY = 180.0f;
            instructionCubeRotationX = 0.0f;
            break;
        case 3: // Face 4
            instructionCubeRotationY = 270.0f;
            instructionCubeRotationX = 0.0f;
            break;
        case 4: // Face 5 (top)
            instructionCubeRotationY = 0.0f;
            instructionCubeRotationX = -90.0f;
            break;
        case 5: // Face 6 (bottom)
            instructionCubeRotationY = 0.0f;
            instructionCubeRotationX = 90.0f;
            break;
        }

        if (currentFace == 0) {
            showInstructionCube = false; // Hide the cube after all faces are shown
        }
    }
}




// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

int main(void)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
 

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    
    Shader skyboxShader("skyboxvertexShader.glsl", "skyboxfragmentShader.glsl");
    Shader instruction("cubemap.vs", "cubemap.fs");
    float skyboxVertices[] = {
        // positions          
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
   
    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    //cubevao
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
    
    
    //
    std::vector<std::string> faces= 
    {
         "skybox/right.png",
        "skybox/left.png",
        "skybox/top.png",
        "skybox/bottom.png",
        "skybox/front.png",
        "skybox/back.png",
        
        
       
       
    };
    unsigned int cubemapTexture = loadCubemap(faces);
    std::vector<std::string> cubefaces =
    {
         "cubemap/right.jpg",
        "cubemap/left.jpg",
        "cubemap/top.jpg",
        "cubemap/bottom.jpg",
        "cubemap/front.jpg",
        "cubemap/back.jpg",




    };
    unsigned int instructionCubeMap = loadCubemap(cubefaces);
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
    
    Shader Modelshder("model_loading.vs", "model_loading.fs");
   
    Model satellite_floor("world/satellite_floor.obj");
    models.push_back(satellite_floor);
    Model satellite_bar("world/satellite_bar.obj");
    models.push_back(satellite_bar);
    Model maze("world/maze.obj");
    Model fake_floor("world/fake_floor.obj");
    models.push_back(fake_floor);
    Model moon_cons3("world/moon_cons3.obj");
    Model moon_cons2("world/moon_cons2.obj");
    Model moon_cons1("world/moon_cons1.obj");
    Model moon("world/moon.obj");
    Model moon_ring("world/moon_ring.obj");
    Model moon_plat("world/moon_plat.obj");
    Model planet("world/planet.obj");
    Model planet_rings("world/planet_rings.obj");
    Model planet_meteor1("world/planet_meteor1.obj");
    Model planet_meteor2("world/planet_meteor2.obj");
    models.push_back(moon_cons3);
    models.push_back(moon_cons2);
    models.push_back(moon_cons2);
    models.push_back(moon);
    models.push_back(moon_ring);
    models.push_back(moon_plat);
    models.push_back(planet);
   
    
    PickableObject cherry("world/cherry.obj");
    pickableObjects.push_back(cherry);
    scoreManager.objectsToPick.push_back(0);
    PickableObject flower("world/flower.obj");
    pickableObjects.push_back(flower);
    scoreManager.objectsToPick.push_back(1);
    Model tree("world/tree.obj");
    models.push_back(tree);
    Model dustbin("world/dustbin.obj");
    models.push_back(dustbin);
    Model screen("world/screen.obj");
    models.push_back(screen);
    Model satellite("world/Satellite.obj");
    models.push_back(satellite);
    Model garage("world/garage.obj");
    Model garage_floor("world/garage_floor.obj");
    models.push_back(garage_floor);
    Model platform("world/platform.obj");
    models.push_back(platform);

    PickableObject mango4("world/mango4.obj");
    pickableObjects.push_back(mango4);
    PickableObject mango3("world/mango3.obj");
    pickableObjects.push_back(mango3);
    PickableObject mango2("world/mango2.obj");
    pickableObjects.push_back(mango2);
    PickableObject mango1("world/mango1.obj");
    pickableObjects.push_back(mango1);

    PickableObject garbage3("world/garbage3.obj");
    pickableObjects.push_back(garbage3);

    PickableObject garbage2("world/garbage2.obj");
    pickableObjects.push_back(garbage2);

    PickableObject garbage1("world/garbage1.obj");
    pickableObjects.push_back(garbage1);

    PickableObject keyObject("world/key.obj");
    pickableObjects.push_back(keyObject);
    scoreManager.objectsToPick.push_back(9);
    Model bench("world/bench.obj");
   
    models.push_back(bench);

    for (PickableObject a : pickableObjects) {
        models.push_back(a.model);
    }

    /*models.push_back(planet_rings);
    models.push_back(planet_meteor);*/

   /* Model meteors("world/meteors.obj");
   
    models.push_back(meteors);*/
        // Robot
    Shader HandShader("model_loading.vs", "model_loading.fs");
    Model handModel("hand/hand.obj");

    Shader RobotShader("model_loading.vs", "model_loading.fs");
        Model base("Eva/body.obj");
        Model head("Eva/head.obj");
        Model lefthand("Eva/lefthand.obj");
        Model righthand("Eva/righthand.obj");
        /*models.push_back(head);
        models.push_back(base);
        models.push_back(lefthand);
        models.push_back(righthand);*/
       /* Model rightboot("robot/rightboot.obj");
        Model leftboot("robot/leftboot.obj");
        Model leftthigh("robot/leftthigh.obj");
        Model rightthigh("robot/rightthigh.obj");
        Model leftknee("robot/leftknee.obj");
        Model rightknee("robot/rightknee.obj");
        Model waist("robot/waist.obj");*/
        glm::mat4 baseMatrix = glm::mat4(1.0f);
        glm::mat4 headMatrix = glm::mat4(1.0f);
        glm::mat4 lefthandMatrix = glm::mat4(1.0f);
        glm::mat4 righthandMatrix = glm::mat4(1.0f);
       /* glm::mat4 rightbootMatrix = glm::mat4(1.0f);
        glm::mat4 leftbootMatrix = glm::mat4(1.0f);
        glm::mat4 leftthighMatrix = glm::mat4(1.0f);
        glm::mat4 rightthighMatrix = glm::mat4(1.0f);
        glm::mat4 leftkneeMatrix = glm::mat4(1.0f);
        glm::mat4 rightkneeMatrix = glm::mat4(1.0f);
        glm::mat4 waistMatrix = glm::mat4(1.0f);*/
//light

   //bounding spehere
          // A
       
        bool followCamera = false;
#include <glm/gtc/constants.hpp> // for glm::pi

        float speed = 1.0f; // Define this according to your needs
        glm::vec3 bodyPosition = glm::vec3(0.0f, 0.0f, 0.0f); // Initialize this to the starting position of the robot
        glm::vec3 elpos= glm::vec3(0.0f, 0.0f, 0.0f);
        float angle = 0.0f; // The current direction of movement
        float nextChange = 0.0f; // The next time to change direction
        BoundingBox moonbb = moon.calculateBoundingBox();
        glm::vec3 center = 0.5f * (moonbb.min + moonbb.max);
        std::cout << center.x << " y : " << center.y << " Z: " << center.z << endl;
        center.y = 200.0f;


        glm::vec3 fogColor = glm::vec3(0.5f, 0.5f, 0.5f); // Fog color (gray in this case)
        float fogStart = 50.0f; // The distance where the fog starts
        float fogEnd = 100; // The distance where the fog ends

        //imgui
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer bindings
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
        BoundingBox bbb = bench.calculateBoundingBox();
        while (!glfwWindowShouldClose(window))
        {
            /* Render here */
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            // input
            // -----
            processInput(window);
            glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_FALSE);
            glfwSetMouseButtonCallback(window, mouse_button_callback);

            // render
            // ------
            glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            //hands
            // Calculate the hand's position and orientation based on the camera's parameters
            glm::mat4 handMatrix = camera.GetViewMatrix();
            handMatrix = glm::inverse(handMatrix); // Convert from view to world space
            //handMatrix = glm::translate(handMatrix); // Offset the hand relative to the camera
            handMatrix = glm::scale(handMatrix, glm::vec3(0.1f)); // Scale the hand to a suitable size

            // Render the hand using the calculated transformation matrix
            //HandShader.use(); // Assuming shader is your Shader object
            //HandShader.setMat4("model", glm::inverse(handMatrix));
            //handModel.Draw(HandShader);


            // don't forget to enable shader before setting uniforms
            Modelshder.use();

            // view/projection transformations
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
            glm::mat4 view = camera.GetViewMatrix();
            Modelshder.setMat4("projection", projection);
            Modelshder.setMat4("view", view);

            //render the loaded model
            glm::mat4 model = glm::mat4(1.0f);
            //model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
            //model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));	// it's a bit too big for our scene, so scale it down
            Modelshder.setMat4("model", model);

            satellite_bar.Draw(Modelshder);
            model = glm::mat4(1.0f);

            Modelshder.setMat4("model", model);
            satellite_floor.Draw(Modelshder);

            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            maze.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            fake_floor.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            moon_plat.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            moon_ring.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            moon.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            moon_cons3.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            moon_cons2.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            moon_cons1.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            planet.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            planet_rings.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            planet_meteor1.Draw(Modelshder);
            Modelshder.setMat4("model", model);

            planet_meteor2.Draw(Modelshder);

            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            garage_floor.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            tree.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            dustbin.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            satellite.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            screen.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            garage.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            platform.Draw(Modelshder);
            model = glm::mat4(1.0f);
            Modelshder.setMat4("model", model);
            bench.Draw(Modelshder);

            if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && picked) {
                // Get the current transformation for the picked object
                glm::mat4 finalTransformation = pickableObjects[pickedObjectIndex].transformationMatrix;

                // Set the transformation matrix for the picked object to the current transformation


                // Reset picking state
                picked = false;
                pickedObjectIndex = -1;
                if (scoreManager.keyPicked && isColliding(camera.bb, bbb)) {
                    cout << "passed" << endl;
                    scoreManager.deliverToBench();
                }
                else {
                    scoreManager.notdeli();
                }
            }
            if (picked && pickedObjectIndex >= 0) {
                scoreManager.pickObject(pickedObjectIndex);
                // Get the camera's position and direction
                glm::vec3 cameraPosition = camera.Position;
                glm::vec3 cameraDirection = camera.Front;

                // Compute the desired position for the key, slightly in front of the camera
                float frontOffset =- 0.5f; // Distance in front of the camera, adjust as needed
               

                // Set the transformation matrix directly to the desired position
                pickableObjects[pickedObjectIndex].transformationMatrix = glm::translate(glm::mat4(1.0f), elpos );
                float followSpeed = 2.5f;
                glm::vec3 cameraOffset = glm::vec3(0.5f, 1.5f, 0.6f);
                elpos = glm::mix(elpos, camera.Position- cameraOffset, followSpeed * deltaTime);
                // Apply the updated transformation to the picked object
               
            }
            if (scoreManager.objectsToPick.empty()) {
                scoreManager.text = "You WON!!AND SAVED THE PLANET";
            }



            for (PickableObject a : pickableObjects) {

                Modelshder.setMat4("model", a.transformationMatrix);
                /* glm::vec3 translation = glm::vec3(a.transformationMatrix[3][0], a.transformationMatrix[3][1], a.transformationMatrix[3][2]);
                 std::cout << "value during drawing : (" << translation.x << ", " << translation.y << ", " << translation.z << ")" << std::endl;

                 cout << "drawing key" << endl;*/
                a.model.Draw(Modelshder);
                /*     glm::vec3 cameraPosition = camera.Position;
                     std::cout << "Camera Position: (" << cameraPosition.x << ", " << cameraPosition.y << ", " << cameraPosition.z << ")" << std::endl;*/
            }
            glm::vec3 lightPos = center;// A position in the distance
            glm::vec3 lightColor = glm::vec3(1.7f, 1.7f, 1.7f); // White light
            Modelshder.setVec3("lightPos", lightPos);
            Modelshder.setVec3("lightColor", lightColor);
            Modelshder.setBool("useBlinnPhong", useBlinnPhong);
           
            // Set the fog uniforms
            Modelshder.setVec3("fogColor", fogColor);
            Modelshder.setFloat("fogStart", fogStart);
            Modelshder.setFloat("fogEnd", fogEnd);

            // key 
         

            if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
                followCamera = !followCamera;

            // Update transformations
            float time = glfwGetTime(); // Get the current time

            if (followCamera) {
                // Calculate the direction from the robot to the camera
                glm::vec3 direction = glm::normalize(camera.Position - bodyPosition);
                // Calculate the target angle the robot should turn to
                float targetAngle = atan2(direction.x, direction.z);

                // Gradually turn the robot towards the target angle
                float currentAngle = atan2(baseMatrix[2][0], baseMatrix[2][2]);
                float angleDifference = targetAngle - currentAngle;
                if (angleDifference > glm::pi<float>())
                    angleDifference -= 2 * glm::pi<float>();
                else if (angleDifference < -glm::pi<float>())
                    angleDifference += 2 * glm::pi<float>();
                currentAngle += angleDifference * deltaTime; // Adjust the speed as needed

                baseMatrix = glm::translate(glm::mat4(1.0f), bodyPosition); // Translate to body position
                baseMatrix = glm::rotate(baseMatrix, currentAngle, glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate to face the camera

                // Gradually move the robot towards the camera
                float followSpeed = 0.5f; // This determines how fast the robot moves towards the camera
                glm::vec3 cameraOffset = glm::vec3(2.0f, 0.6f, 0.0f);
                bodyPosition = glm::mix(bodyPosition, camera.Position - cameraOffset, followSpeed * deltaTime); // Lerp between the robot's current position and the camera's position
            }
            // Lerp between the robot's current position and the camera's position

            else {
                // Make the robot move around randomly
                if (glfwGetTime() > nextChange) {
                    // Change direction every 2 seconds
                    nextChange = glfwGetTime() + 2.0f;
                    // Generate a random angle
                    angle = ((float)rand() / (float)RAND_MAX) * 2.0f * glm::pi<float>();
                }
                // Calculate movement vector from angle
                glm::vec3 movement(cos(angle), 0, sin(angle));
                // Move the robot
                bodyPosition += movement * speed * deltaTime;
                // Rotate the base matrix to face the direction of movement
                baseMatrix = glm::translate(glm::mat4(1.0f), bodyPosition); // Translate to body position
                baseMatrix = glm::rotate(baseMatrix, angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate to face the direction of movement
            }

            baseMatrix = glm::scale(baseMatrix, glm::vec3(0.2f)); // Scale the whole robot down

            // Make the head nod
            headMatrix = glm::rotate(glm::mat4(1.0f), sin(time) * glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f));

            // Make the hands wave
            lefthandMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Translate along the arm's length
            lefthandMatrix = glm::rotate(lefthandMatrix, sin(time) * glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around the "shoulder" axis

            righthandMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Translate along the arm's length
            righthandMatrix = glm::rotate(righthandMatrix, -sin(time) * glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around the "shoulder" axis

            // Draw the body
            RobotShader.use();
            RobotShader.setMat4("projection", projection);
            RobotShader.setMat4("view", view);
            RobotShader.setMat4("model", baseMatrix * glm::translate(glm::mat4(1.0f), bodyPosition));
            base.Draw(RobotShader);

            // Draw the head, left hand, and right hand relative to the body
            RobotShader.setMat4("model", baseMatrix * glm::translate(glm::mat4(1.0f), bodyPosition) * headMatrix);
            head.Draw(RobotShader);

            RobotShader.setMat4("model", baseMatrix * glm::translate(glm::mat4(1.0f), bodyPosition) * lefthandMatrix);
            lefthand.Draw(RobotShader);

            RobotShader.setMat4("model", baseMatrix * glm::translate(glm::mat4(1.0f), bodyPosition) * righthandMatrix);
            righthand.Draw(RobotShader);

            RobotShader.setVec3("lightPos", lightPos);
            RobotShader.setVec3("lightColor", lightColor);
            RobotShader.setBool("useBlinnPhong", useBlinnPhong);
            RobotShader.setVec3("fogColor", fogColor);
            RobotShader.setFloat("fogStart", fogStart);
            RobotShader.setFloat("fogEnd", fogEnd);
            //object picking  
            if (showInstructionCube) {
            glDepthFunc(GL_LEQUAL);
            instruction.use();

            // Create transformation matrices
             model = glm::mat4(1.0f);
             glm::mat4 model = glm::mat4(1.0f);
             model = glm::translate(model, camera.Position + camera.Front * 1.0f); // Adjust the distance from the camera
             model = glm::rotate(model, glm::radians(-camera.Yaw), glm::vec3(0.0f, 1.0f, 0.0f)); // Align with camera directionthe cube
             model = glm::scale(model, glm::vec3(0.5f));
              model = glm::rotate(model, glm::radians(instructionCubeRotationX), glm::vec3(1.0f, 0.0f, 0.0f));
             model = glm::rotate(model, glm::radians(instructionCubeRotationY), glm::vec3(0.0f, 1.0f, 0.0f));
            instruction.setMat4("model", model);

            view = camera.GetViewMatrix(); // Assuming you have a camera class
           projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

            // Set transformation matrices in the shader
            instruction.setMat4("model", model);
            instruction.setMat4("view", view);
            instruction.setVec3("fogColor", fogColor);
            instruction.setFloat("fogStart", fogStart);
            instruction.setFloat("fogEnd", fogEnd);
            instruction.setMat4("projection", projection);

            // Bind the instruction texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, instructionCubeMap);

            // Bind the cube VAO and draw it
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);

            glDepthFunc(GL_LESS);
        }
        // Draw the skybox as last
        glDepthFunc(GL_LEQUAL); 
        // Change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(view)); // Remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        //instructioncube 
       // Activate the shader for the instruction cube
       

        // Skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // Set depth function back to default
        //gui
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowSize(ImVec2(1000, 100), ImGuiCond_FirstUseEver);
        ImGui::Begin("Options"); // Create a new ImGui window named "Options"

        if (ImGui::Checkbox("Gravity", &gravity)) {
       
            gravity = !gravity;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Eva:Robot ", &followCamera)) {

            followCamera = !followCamera;
        }

        if (ImGui::Checkbox("Blinn", &useBlinnPhong)) {
       
            useBlinnPhong = !useBlinnPhong;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("pick", &picked)) {

            picked = !picked;
        }
        

       
        ImGui::SameLine();

        ImGui::End();
        ImGui::SetNextWindowSize(ImVec2(400, 100));
        ImGui::Begin("Game Info");

        // Display the score
        ImGui::Text("Score: %d", scoreManager.score);
        ImGui::Text("%s", scoreManager.text.c_str());
        // Check the game state and display the corresponding message
        if (scoreManager.keyPicked) {
            ImGui::Text("Key picked!");
        }
        
        // Add more conditions as needed

        ImGui::End();
        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }


  
    glDeleteVertexArrays(1, &skyboxVAO);

    glDeleteBuffers(1, &skyboxVBO);
    glDeleteVertexArrays(1, &cubeVAO);

    glDeleteBuffers(1, &cubeVAO);
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();


    glfwTerminate();
    return 0;
}
