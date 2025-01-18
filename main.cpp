#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "Skybox.hpp"

#include <iostream>

// window initialization
gps::Window myWindow;
const unsigned int SCREEN_WIDTH = 1024, SCREEN_HEIGHT = 768;

// transformations matrices
glm::mat4 model; // model matrix for world space transformations
glm::mat4 view; // view matrix for camera positioning
glm::mat4 projection; // projection matrix for perspective
glm::mat3 normalMatrix; // normal matrix for lighting calculations

// light parameters
glm::vec3 lightDir; // direction of the main light source
glm::vec3 lightColor; // color of the main light
glm::mat4 lightRotation; // matrix for rotating the light
GLuint lightRotationLoc; // shader location for light rotation
GLfloat lightAngle; // current angle of the light rotation

// basic shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;

// trees shader uniform locations
GLint treesModelLoc;
GLint treesViewLoc;
GLint treesProjectionLoc;
GLint treesNormalMatrixLoc;
GLint treesLightDirLoc;
GLint treesLightColorLoc;

// camera setup
//gps::Camera myCamera(
//    glm::vec3(27.25f, 7.64f, -3.42f), // camera position
//    glm::vec3(-17.75f, 3.99f, -6.11f), // camera target
//    glm::vec3(0.0f, 1.0f, 0.0f)); // up vector
// camera setup
glm::vec3 initialCameraPosition(27.25f, 7.64f, -3.42f);
glm::vec3 initialCameraTarget(-17.75f, 3.99f, -6.11f);
glm::vec3 initialCameraUp(0.0f, 1.0f, 0.0f);

gps::Camera myCamera(initialCameraPosition, initialCameraTarget, initialCameraUp);

GLfloat cameraSpeed = 0.5f;

// array to track keyboard input
GLboolean pressedKeys[1024]; 

// 3D model objects
gps::Model3D scene;
gps::Model3D trees;
gps::Model3D tractor;
gps::Model3D screenQuad;
gps::Model3D lightCube1;
gps::Model3D lightCube2;
gps::Model3D balloon;
gps::Model3D lake;
gps::Model3D raindrop;

gps::SkyBox skyBox; // Skybox object

// shader programs
gps::Shader basicShader;
gps::Shader skyboxShader;
gps::Shader shadowShader;
gps::Shader treesShader;
gps::Shader screenQuadShader;
gps::Shader lightCubeShader;

// shadow mapping parameters
const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
GLuint depthMapFBO; // framebuffer for shadow mapping
GLuint depthMapTexture; // depth texture for shadow mapping
float near_plane = 0.1f, far_plane = 70.0f;
glm::mat4 lightSpaceMatrix; // matrix for light space transformation
GLint lightSpaceMatrixLoc;
bool showDepthMap = false;

// point light parameters
glm::vec3 pointLightPos;
glm::vec3 pointLightColor;
glm::vec3 pointLightPos1;
glm::vec3 pointLightColor1;
glm::vec3 pointLightPos2;
glm::vec3 pointLightColor2;
glm::vec3 pointLightPos3;
glm::vec3 pointLightColor3;
bool pointLightFlag;

//alpha channel for transparent objects
float alpha;
GLuint alphaLoc;

// fog parameters
bool fogFlag = true;
float fogDensity = 0.02f;

bool isRaining = false;

struct Rain {
	glm::vec3 position;
	glm::vec3 velocity;
};

std::vector<Rain> raindrops;

GLint isNight = 0; // toggle for day/night state

glm::vec3 balloonPosition(-6.32f, 5.0f, 1.49f); // initial position for the balloon

float lastFrameTime = 0.0f;

// tour animation variable
bool inTour = false;

bool captureMouse = false;

void initRain() {
	for (int i = 0; i < 3000; i++) {
		Rain raindrop;
		raindrop.position = glm::vec3((rand() % 100) - 50, 15.0f + (rand() % 5), (rand() % 100) - 50);
		raindrop.velocity = glm::vec3(-0.02f, -0.3f, 0.01f);
		raindrops.push_back(raindrop);
	}
}

void updateRain() {
    for (int i = 0; i < raindrops.size(); i++) {
		raindrops[i].position += raindrops[i].velocity; // update raindrop position
		if (raindrops[i].position.y < -1.0f) { // reset raindrop when it falls below ground level
            raindrops[i].position = glm::vec3((rand() % 100) - 50, 15.0f + (rand() % 5), (rand() % 100) - 50);
        }
    }
}

// Calculate delta time between frames
float getDeltaTime() {
    float currentFrameTime = glfwGetTime();
    float deltaTime = currentFrameTime - lastFrameTime;
    lastFrameTime = currentFrameTime;
    return deltaTime;
}

// Check for OpenGL errors
GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM: error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE: error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY: error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

// Callback function for window resize events
void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);

    // Update viewport and window dimensions
    glViewport(0, 0, width, height);
    myWindow.setWindowDimensions({ width, height });

    // Update projection matrix for new aspect ratio
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(glm::perspective(glm::radians(45.0f), width / (float)height, 0.1f, 70.0f)));
}

// Initialize the OpenGL window
void initOpenGLWindow() {
    myWindow.Create(SCREEN_WIDTH, SCREEN_HEIGHT, "GP Project");
}

// Initialize the OpenGL state
void initOpenGLState() {
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB); // enable gamma correction
    glEnable(GL_DEPTH_TEST);  // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    glEnable(GL_CULL_FACE); // enable face culling
    glCullFace(GL_BACK); // cull back faces
    glFrontFace(GL_CCW); // set front faces as counter-clockwise
}

// Load 3D models
void initModels() {
    scene.LoadModel("models/scene/scene.obj");
    trees.LoadModel("models/scene/trees.obj");
    screenQuad.LoadModel("models/quad/quad.obj");
    lightCube1.LoadModel("models/cube/cube.obj");
    lightCube2.LoadModel("models/cube/cube.obj");
    balloon.LoadModel("models/balloon/balloon1.obj");
	raindrop.LoadModel("models/rain/drop.obj");
	//lake.LoadModel("models/lake/lake.obj");
}

// Initialize shader programs
void initShaders() {
    basicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    skyboxShader.loadShader("shaders/skybox.vert", "shaders/skybox.frag");
    shadowShader.loadShader("shaders/shadow.vert", "shaders/shadow.frag");
    treesShader.loadShader("shaders/trees.vert", "shaders/trees.frag");
    screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
    lightCubeShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
}

// Initialize skybox with appropriate textures
void initSkybox(bool isNightTime) {
    std::vector<const GLchar*> faces;
    if (!isNightTime) {
        // Day time skybox textures
        faces.push_back("skybox/posx.tga");
        faces.push_back("skybox/negx.tga");
        faces.push_back("skybox/posy.tga");
        faces.push_back("skybox/negy.tga");
        faces.push_back("skybox/posz.tga");
        faces.push_back("skybox/negz.tga");
    }
    else {
        // Night time skybox textures
        faces.push_back("skybox/night_right.tga");
        faces.push_back("skybox/night_left.tga");
        faces.push_back("skybox/night_top.tga");
        faces.push_back("skybox/night_bottom.tga");
        faces.push_back("skybox/night_back.tga");
        faces.push_back("skybox/night_front.tga");
    }
    skyBox.Load(faces);
}

// Callback function for mouse movement
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!captureMouse) return;  // don't process mouse movement if cursor is not captured

    //myCamera.mouse_callback(xpos, ypos);
    basicShader.useShaderProgram();
    static double currentX = 0.0;
    static double currentY = 0.0;
    const float sensitivity = 0.10f;
    static int enterWindow = 1;

    // Initialize mouse position on first entry
    if (enterWindow == 1) {
        currentY = ypos;
        currentX = xpos;

        enterWindow = 0;
    }

    // Calculate mouse movement
    double yoffset = currentY - ypos;
    double xoffset = xpos - currentX;

    // Apply sensitivity
    yoffset *= sensitivity;
    xoffset *= sensitivity;

    // Update current position
    currentY = ypos;
    currentX = xpos;

    // Rotate camera based on mouse movement
    myCamera.rotate(-yoffset, -xoffset);
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

// Callback function for keyboard input
void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        // Close the window when the Escape key is pressed
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
		pointLightFlag = !pointLightFlag;  // toggle flag for point lights

        if (!pointLightFlag) {
            // turn off all lights
            pointLightColor = glm::vec3(0.0f);
            pointLightColor1 = glm::vec3(0.0f);
            pointLightColor2 = glm::vec3(0.0f);
            pointLightColor3 = glm::vec3(0.0f);
        }
        else {
            pointLightColor = glm::vec3(1.0f); // white
            pointLightColor1 = glm::vec3(1.50f, 0.0f, 1.0f); // purple
            pointLightColor2 = glm::vec3(1.0f, 1.0f, 0.0f); // yellow
            pointLightColor3 = glm::vec3(1.0f, 0.5f, 0.0f); // orange
        }
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		fogFlag = !fogFlag; // toggle fog flag
        if (!fogFlag) {
            fogDensity = 0.0f;
        }
        else {
            fogDensity = 0.02f;
        }
    }

	if (key == GLFW_KEY_0 && action == GLFW_PRESS) {
		inTour = !inTour; // toggle tour animation
	}

	if (key == GLFW_KEY_M && action == GLFW_PRESS) {
		showDepthMap = !showDepthMap; // toggle depth map visualization
	}

    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
		if (isNight == 0) // toggle day/night state
            isNight = 1;
        else
            isNight = 0;
        if(isNight == 0) // initialize the skybox for the corresponding state
		{
			initSkybox(false);
		}
		else {
			initSkybox(true);
		}
    }

	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		isRaining = !isRaining; // toggle rain
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        // Print the current camera position when the C key is pressed
		printf("Camera position: x = %.2f, y = %.2f, z = %.2f\n", myCamera.getCameraPosition().x, myCamera.getCameraPosition().y, myCamera.getCameraPosition().z);
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {  
        captureMouse = !captureMouse;
        if (captureMouse) {
            glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
            glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
            glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void setWindowCallbacks() {
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(),  mouseCallback); 
}

void processMovement() {
    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        view = myCamera.getViewMatrix();
        basicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        view = myCamera.getViewMatrix();
        basicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        view = myCamera.getViewMatrix();
        basicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        view = myCamera.getViewMatrix();
        basicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        myCamera.move(gps::MOVE_UP, cameraSpeed);
        view = myCamera.getViewMatrix();
        basicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_R]) {
        myCamera.move(gps::MOVE_DOWN, cameraSpeed);
        view = myCamera.getViewMatrix();
        basicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_RIGHT]) {
        myCamera.rotate(0.0f, -cameraSpeed * getDeltaTime() * 100.0f);
    }
    if (pressedKeys[GLFW_KEY_LEFT]) {
        myCamera.rotate(0.0f, cameraSpeed * getDeltaTime() * 100.0f);
    }
    if (pressedKeys[GLFW_KEY_UP]) {
        myCamera.rotate(-cameraSpeed * getDeltaTime() * 100.0f, 0.0f);
    }
    if (pressedKeys[GLFW_KEY_DOWN]) {
        myCamera.rotate(cameraSpeed * getDeltaTime() * 100.0f, 0.0f); 
    }

    if (pressedKeys[GLFW_KEY_1]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframe mode
    }

    if (pressedKeys[GLFW_KEY_2]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); //view point objects
    }

    if (pressedKeys[GLFW_KEY_3]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // solid mode
    }

    if (pressedKeys[GLFW_KEY_4]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_SMOOTH); // smooth mode
    }

    if (pressedKeys[GLFW_KEY_5]) {
		lightAngle -= 1.0f;
    }

    if (pressedKeys[GLFW_KEY_6]) {
        lightAngle += 1.0f;
    }

    if (fogFlag) {
        // increase fog density
        if (pressedKeys[GLFW_KEY_7]) {
            fogDensity = glm::min(fogDensity + 0.001f, 1.0f);
        }

        // decrease fog density
        if (pressedKeys[GLFW_KEY_8]) {
            fogDensity = glm::max(fogDensity - 0.001f, 0.0f);
        }
    }
}

void initUniforms() {
    // scene matrices
    basicShader.useShaderProgram();

    // create model matrix
    model = glm::mat4(1.0f);
    modelLoc = glGetUniformLocation(basicShader.shaderProgram, "model");
   
    // get view matrix for current camera
    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(basicShader.shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view)); // send view matrix to shader

    // compute normal matrix
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(basicShader.shaderProgram, "normalMatrix");
	
    // create projection matrix
    projection = glm::perspective(glm::radians(45.0f), (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height, 0.1f, 70.0f);
    projectionLoc = glGetUniformLocation(basicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection)); // send projection matrix to shader

    // set the light direction (direction towards the light)
    lightDir = glm::vec3(4.0f, 4.0f, -9.0f);
    lightDirLoc = glGetUniformLocation(basicShader.shaderProgram, "lightDir");
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    // send light dir to shader
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(lightRotation)) * lightDir));
   
    //set light color
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    lightColorLoc = glGetUniformLocation(basicShader.shaderProgram, "lightColor");
    // send light color to shader
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    // point lights
    pointLightFlag = true;
    pointLightPos = glm::vec3(-15.3f, 1.4839f, -6.4227f);
    pointLightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    pointLightPos1 = glm::vec3(-2.65f, 1.18f, 3.66f);
    pointLightColor1 = glm::vec3(1.0f, 0.0f, 1.0f);
    pointLightPos2 = glm::vec3(-1.58f, 1.09f, 3.64f);
    pointLightColor2 = glm::vec3(1.0f, 1.0f, 0.0f);
    pointLightPos3 = glm::vec3(-12.61f, 1.15f, 12.50f);
    pointLightColor3 = glm::vec3(1.0f, 0.5f, 0.0f);

	// alpha channel for transparent objects
    alpha = 0.85;
	alphaLoc = glGetUniformLocation(basicShader.shaderProgram, "alpha");
	glUniform1f(alphaLoc, alpha);

    // shadows
    glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProjection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, near_plane, far_plane);
    lightSpaceMatrix = lightProjection * lightView;
    lightSpaceMatrixLoc = glGetUniformLocation(shadowShader.shaderProgram, "lightSpaceMatrix");

    // trees
    treesShader.useShaderProgram();
    treesModelLoc = glGetUniformLocation(treesShader.shaderProgram, "model");
    treesNormalMatrixLoc = glGetUniformLocation(treesShader.shaderProgram, "normalMatrix");
    treesViewLoc = glGetUniformLocation(treesShader.shaderProgram, "view");
    treesProjectionLoc = glGetUniformLocation(treesShader.shaderProgram, "projection");
    treesLightDirLoc = glGetUniformLocation(treesShader.shaderProgram, "lightDir");
    treesLightColorLoc = glGetUniformLocation(treesShader.shaderProgram, "lightColor");
    glUniformMatrix4fv(treesViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(treesProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(treesLightDirLoc, 1, glm::value_ptr(lightDir));
    glUniform3fv(treesLightColorLoc, 1, glm::value_ptr(lightColor));

    // lightCube
    lightCubeShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightCubeShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

glm::mat4 computeLightSpaceTrMatrix() {
    //TODO - Return the light-space transformation matrix
    glm::mat4 lightView = glm::lookAt(glm::inverseTranspose(glm::mat3(lightRotation)) * lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const GLfloat near_plane = 1.0f, far_plane = 20.0f;
    glm::mat4 lightProjection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, near_plane, far_plane);
    //glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
    return lightSpaceTrMatrix;
}

void renderMainScene(gps::Shader shader, bool depthPass) {
    // select active shader program
    shader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    if (!depthPass) {
		glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    scene.Draw(shader);
}

void renderTrees(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    if (!depthPass) {
        glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(treesNormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    glUniformMatrix4fv(treesViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniform3fv(treesLightDirLoc, 1, glm::value_ptr(glm::mat3(view) * lightDir));
    glUniform1f(glGetUniformLocation(shader.shaderProgram, "fogDensity"), fogDensity);
    glUniform1i(glGetUniformLocation(shader.shaderProgram, "isNight"), isNight);
    glUniform3fv(glGetUniformLocation(shader.shaderProgram, "pointLightColor"), 1, glm::value_ptr(pointLightColor));
    glUniform3fv(glGetUniformLocation(shader.shaderProgram, "pointLightColor1"), 1, glm::value_ptr(pointLightColor1));
    glUniform3fv(glGetUniformLocation(shader.shaderProgram, "pointLightColor1"), 1, glm::value_ptr(pointLightColor2));
    glUniform3fv(glGetUniformLocation(shader.shaderProgram, "pointLightColor3"), 1, glm::value_ptr(pointLightColor3));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
    glDisable(GL_CULL_FACE);
    trees.Draw(shader);
    glEnable(GL_CULL_FACE);
}

void renderLake(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();
    //enable blending for transparent objects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    if (!depthPass) {
        glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    //set alpha channel
    alpha = 0.5f;
    alphaLoc = glGetUniformLocation(shader.shaderProgram, "alpha");
    glUniform1f(alphaLoc, alpha);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    // Disable depth writing but keep depth testing
    glDepthMask(GL_FALSE);
    lake.Draw(shader);
    glDepthMask(GL_TRUE);
    // Disable blending after rendering
    glDisable(GL_BLEND);
}

void updateBalloon(float deltaTime) {
    static float angle = 0.0f;
    float radius = 1.0f; // Radius of the circular path
    float speed = 1.0f;  // Speed of the circular motion

    angle += speed * deltaTime;
    balloonPosition.x = radius * cos(angle);
    balloonPosition.z = radius * sin(angle);
    balloonPosition.y = 5.0f; // Fixed height in the scene
}

void renderBalloon(gps::Shader shader, bool depthPass) {
    shader.useShaderProgram();

    // Set the model matrix for the balloon
    glm::mat4 model = glm::translate(glm::mat4(1.0f), balloonPosition);
    model = glm::scale(model, glm::vec3(0.5f)); // apply scaling

    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    if (!depthPass) {
        glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    balloon.Draw(shader);
}

void renderRain(gps::Shader shader) {
	shader.useShaderProgram();

    // render the raindrops
	for (int i = 0; i < raindrops.size(); i++) {
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, raindrops[i].position);
        model = glm::scale(model, glm::vec3(0.1f, 0.5f, 0.1f));
		glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
		raindrop.Draw(shader);
	}
}

void renderScene() {
    // Clear the color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    shadowShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(shadowShader.shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

    // render scene = draw objects
	renderMainScene(shadowShader, true);
    renderTrees(shadowShader, true);
    // Update and render the balloon
    float deltaTime = getDeltaTime();
    updateBalloon(deltaTime);
    renderBalloon(shadowShader, true);
    renderLake(shadowShader, true);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (showDepthMap) {
        glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
        glClear(GL_COLOR_BUFFER_BIT);
        screenQuadShader.useShaderProgram();
        //bind the depth map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);
        glDisable(GL_DEPTH_TEST);
        screenQuad.Draw(screenQuadShader);
        glEnable(GL_DEPTH_TEST);
    } else {
        // final scene rendering pass (with shadows)
        glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        basicShader.useShaderProgram();

        glUniform1f(glGetUniformLocation(basicShader.shaderProgram, "fogDensity"), fogDensity);
        glUniform1i(glGetUniformLocation(basicShader.shaderProgram, "isNight"), isNight);

        glUniform3fv(glGetUniformLocation(basicShader.shaderProgram, "pointLightPos"), 1, glm::value_ptr(pointLightPos));
        glUniform3fv(glGetUniformLocation(basicShader.shaderProgram, "pointLightPos1"), 1, glm::value_ptr(pointLightPos1));
        glUniform3fv(glGetUniformLocation(basicShader.shaderProgram, "pointLightPos2"), 1, glm::value_ptr(pointLightPos2));
        glUniform3fv(glGetUniformLocation(basicShader.shaderProgram, "pointLightPos3"), 1, glm::value_ptr(pointLightPos3));
        glUniform3fv(glGetUniformLocation(basicShader.shaderProgram, "pointLightColor"), 1, glm::value_ptr(pointLightColor));
        glUniform3fv(glGetUniformLocation(basicShader.shaderProgram, "pointLightColor1"), 1, glm::value_ptr(pointLightColor1));
        glUniform3fv(glGetUniformLocation(basicShader.shaderProgram, "pointLightColor2"), 1, glm::value_ptr(pointLightColor2));
        glUniform3fv(glGetUniformLocation(basicShader.shaderProgram, "pointLightColor3"), 1, glm::value_ptr(pointLightColor3));

        view = myCamera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

        //bind the shadow map
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(basicShader.shaderProgram, "shadowMap"), 3);
        glUniformMatrix4fv(glGetUniformLocation(basicShader.shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

        renderMainScene(basicShader, false);
        renderTrees(treesShader, false);

        // Update and render the balloon
        float deltaTime = getDeltaTime(); 
        updateBalloon(deltaTime);
        renderBalloon(basicShader, false);

		if (isRaining) {
			renderRain(basicShader);
		}

        //renderLake(shadowShader, false);

        skyBox.Draw(skyboxShader, view, projection);
    }
}

void cleanup() {
    myWindow.Delete();
}

void initFBO() {
    //TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
    //generate FBO ID
    glGenFramebuffers(1, &depthMapFBO);
    //create depth texture for FBO
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    //attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Function to reset the camera to its initial position and orientation
void resetCamera() {
    myCamera.setCameraPosition(initialCameraPosition);
    myCamera.setCameraTarget(initialCameraTarget);
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

// Define tour waypoints for camera movement
struct Waypoint {
    glm::vec3 position;
    glm::vec3 target;
};

// Tour path definition with waypoints
std::vector<Waypoint> tourPath = {
    {{27.0f, 8.0f, -3.5f}, {0.0f, 2.0f, 0.0f}},
    //{{27.25f, 7.64f, -3.42f}, {0.0f, 2.0f, 0.0f}},
    //{{26.68f, 7.64f, 6.06f}, {0.0f, 2.0f, 0.0f}},
    {{20.0f, 7.0f, 5.0f}, {-2.0f, 2.0f, 0.0f}},
    {{13.46f, 3.83f, 15.18f}, {-3.0f, 2.0f, -2.0f}},
    //{{4.21f, 4.99f, 16.25f}, {-8.0f, 2.0f, -2.0f}},
    {{-3.87f, 4.99f, 20.11f}, {-10.0f, 2.0f, -3.0f}},
    //{{-9.83f, 4.99f, 20.76f}, {-10.0f, 2.0f, -3.0f}},
    {{-14.63f, 4.99f, 19.33f}, {3.15f, 2.97f, -2.40f}},
   // {{-18.04f, 3.37f, 12.93f}, {5.0f, 7.0f, -8.0f}},
    {{-17.06f, 2.0f, 7.32f}, {0.0f, 2.0f, 0.0f}},
    {{-23.73f, 1.05f, -10.42f}, {0.0f, 2.0f, 0.0f}},
    {{-20.88f, 1.05f, -16.81f}, {0.0f, 2.0f, 0.0f}},
    //{{-7.77f, 2.72f, -15.41f}, {0.0f, 2.0f, 0.0f}},
    {{-3.62f, 2.72f, -13.68f}, {0.0f, 2.0f, 0.0f}},
    {{27.0f, 8.0f, -3.5f}, {0.0f, 2.0f, 0.0f}}
};


glm::vec3 interpolate(glm::vec3 start, glm::vec3 end, float t) {
    return start * (1.0f - t) + end * t;
}

int main(int argc, const char* argv[]) {
    try {
        initOpenGLWindow();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    initModels();
    initShaders();
    initUniforms();
    setWindowCallbacks();
    initSkybox(false);
    initFBO();
    initRain();

    //glCheckError();

    // application loop
    while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
		updateRain();

        // Check if the tour flag is set to true
        if (inTour) {
            resetCamera();

            static int currentWaypoint = 0;
            static float t = 0.0f;

            if (currentWaypoint < tourPath.size() - 1) {
                glm::vec3 newPosition = interpolate(tourPath[currentWaypoint].position, tourPath[currentWaypoint + 1].position, t);
                glm::vec3 newTarget = tourPath[currentWaypoint + 1].target;  // Focus on next point
                myCamera.setCameraPosition(newPosition);
                myCamera.setCameraDirection(glm::normalize(newTarget - newPosition));

                t += 0.02f;  // Adjust speed for smooth transition
                if (t >= 1.0f) {
                    t = 0.0f;
                    currentWaypoint++;
                }
            }
            else {
                //currentWaypoint = 0;  // Restart the tour after completing
                inTour = false;
            }
        }

        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

        //glCheckError();
    }

    cleanup();

    return EXIT_SUCCESS;
}