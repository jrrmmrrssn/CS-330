#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"    // Image loading Utility functions
#include "Sphere.h"
// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//#include "shader.h"
#include "camera.h" // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Jeremy Morrison"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbo;         // Handle for the vertex buffer object
        GLuint nVertices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh;
    GLMesh gPlaneMesh;
    GLMesh gFloorMesh;
    GLMesh gLightMesh;
    // Texture id
    //GLuint gTextureId;
    GLuint gPlane;
    GLuint gFloor;
    GLuint gWalls;
    GLuint gPlanet1;
    
    // Shader program
    GLuint gProgramId;
    GLuint gCubeProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    
    // Subject position and scale
    glm::vec3 gCubePosition(0.0f, 0.0f, 0.0f);
    glm::vec3 gCubeScale(2.0f);

    // Cube and light color
    //m::vec3 gObjectColor(0.6f, 0.5f, 0.75f);
    glm::vec3 gObjectColor(1.0f, 0.2f, 0.0f);
    glm::vec3 gLightColor(0.20f, 0.92f, 0.91f);
    //glm::vec3 gLight2Color(1.0f, 1.0f, 1.0f);

    // Light position and scale
    glm::vec3 gLightPosition(0.5f, -1.1f, 1.0f);
    //glm::vec3 gLight2Position(-1.5f, 1.0f, 0.0f);
    glm::vec3 gLightScale(0.3f);
    //glm::vec3 gLight2Scale(0.3f);

    // Lamp animation
    bool gIsLampOrbiting = false;

    
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UCreatePlaneMesh(GLMesh& mesh);
void UCreateFloorMesh(GLMesh& mesh);
void UCreateLightMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

Sphere S(1, 30, 30);

/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

        //Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

/* Vertex Shader Source Code
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 aPos;

//Global variables for the transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
);


// Fragment Shader Source Code
const GLchar* fragmentShaderSource = GLSL(440,
    out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0); // set alle 4 vector values to 1.0

}
);
*/
/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position;
layout(location = 2) in vec2 textureCoordinate;

out vec2 vertexTextureCoordinate;


//Global variables for the transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
    vertexTextureCoordinate = textureCoordinate;
}
);

/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
    in vec2 vertexTextureCoordinate;

out vec4 fragmentColor;

uniform sampler2D uTexture;

void main()
{
    fragmentColor = texture(uTexture, vertexTextureCoordinate); // Sends texture to the GPU for rendering
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object
    UCreatePlaneMesh(gPlaneMesh);
    UCreateFloorMesh(gFloorMesh);
    //create light mesh
    UCreateLightMesh(gLightMesh);

 
     // Create the shader programs
     //if (!UCreateShaderProgram(cubeVertexShaderSource, cubeFragmentShaderSource, gCubeProgramId))
      //  return EXIT_FAILURE;
     if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    // Load wall texture
    const char* texFilename = "purple.jpg";
    if (!UCreateTexture(texFilename, gWalls))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // Load plane texture
    const char* texFilename2 = "stars.jpg";
    if (!UCreateTexture(texFilename2, gPlane))
    {
        cout << "Failed to load texture " << texFilename2 << endl;
        return EXIT_FAILURE;
    }
    // Load plane texture
    const char* texFilename3 = "floor.jpeg";
    if (!UCreateTexture(texFilename3, gFloor))
    {
        cout << "Failed to load texture " << texFilename3 << endl;
        return EXIT_FAILURE;
    }
    //load planet1 texture
    const char* texFilename4 = "mars.jpg";
    if (!UCreateTexture(texFilename4, gPlanet1))
    {
        cout << "Failed to load texture " << texFilename4 << endl;
        return EXIT_FAILURE;
    }

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    glUseProgram(gLampProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);
    glUniform1i(glGetUniformLocation(gLampProgramId, "uTexture"), 0);



    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);
    UDestroyMesh(gPlaneMesh);
    UDestroyMesh(gFloorMesh);
    UDestroyMesh(gLightMesh);

    // Release texture
    UDestroyTexture(gWalls);
    UDestroyTexture(gPlane);
    UDestroyTexture(gFloor);

    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UPWARDS, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWNWARDS, gDeltaTime);




    
    // Pause and resume lamp orbiting
    static bool isLKeyDown = false;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !gIsLampOrbiting)
        gIsLampOrbiting = true;
    else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && gIsLampOrbiting)
        gIsLampOrbiting = false;

        
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// Functioned called to render a frame
void URender()
{
   

    /*
    // Lamp orbits around the origin
    const float angularVelocity = glm::radians(45.0f);
    if (gIsLampOrbiting)
    {
        glm::vec4 newPosition = glm::rotate(angularVelocity * gDeltaTime, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(gLightPosition, 1.0f);
        gLightPosition.x = newPosition.x;
        gLightPosition.y = newPosition.y;
        gLightPosition.z = newPosition.z;
    }
    */
    
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Scales the object by 2
    glm::mat4 scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
    // 2. Rotates shape by 0 degrees in the x axis
    glm::mat4 rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gWalls);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices);


    //activate, bind, draw plane mesh & texture
    glBindVertexArray(gPlaneMesh.vao);
    glUseProgram(gProgramId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPlane);
    glDrawArrays(GL_TRIANGLES, 0, gPlaneMesh.nVertices);

    //activate, bind, draw floor mesh & texture
    glBindVertexArray(gFloorMesh.vao);
    glUseProgram(gProgramId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gFloor);
    glDrawArrays(GL_TRIANGLES, 0, gFloorMesh.nVertices);

    
     // LAMP: draw bed lamp
    //----------------

    // Activate the light VAO (used by lamp)
    glBindVertexArray(gLightMesh.vao);
    glUseProgram(gLampProgramId);
    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);
    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");
    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glDrawArrays(GL_TRIANGLES, 0, gLightMesh.nVertices);

    //draw sphere1
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPlanet1);
    glBindVertexArray(gProgramId);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    S.draw();

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    // Vertex data
    GLfloat verts[] = {
        //Positions          //Texture Coordinates
        -0.2f,  0.3f, -0.5f,  1.0f,  1.0f,  0.0f, 0.0f, -1.0f,		//backwall base right
         1.0f,  0.3f, -0.5f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//backwall base left
         1.0f,  0.5f, -0.5f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backwall top left
         1.0f,  0.5f, -0.5f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backwall top left
        -0.2f,  0.5f, -0.5f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//backwall top right
        -0.2f,  0.3f, -0.5f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backwall bottom right

        -0.3f,  0.3f, -0.6f,  1.0f,  1.0f, 	0.0f, 0.0f, 1.0f,	//backwall top lr2 base right
         1.0f,  0.3f, -0.6f,  1.0f,  0.0f, 	0.0f, 0.0f, 1.0f,	//backwall top lr2 base left
         1.0f,  0.5f, -0.6f,  0.0f,  1.0f, 	0.0f, 0.0f, 1.0f,	//backwall top lr2 top left
         1.0f,  0.5f, -0.6f,  1.0f,  1.0f, 	0.0f, 0.0f, 1.0f,	//backwall top lr2 top left
        -0.3f,  0.5f, -0.6f,  1.0f,  0.0f, 	0.0f, 0.0f, 1.0f,	//backwall top lr2 top right
        -0.3f,  0.3f, -0.6f,  0.0f,  1.0f, 	0.0f, 0.0f, 1.0f,	//backwall top lr2 bottom right

         0.9f,  0.3f, -0.6f,  1.0f,  1.0f, 	-1.0f, 0.0f, 0.0f,	//windowwallright lr2 top left
         1.0f,  0.3f, -0.6f,  1.0f,  0.0f, 	-1.0f, 0.0f, 0.0f,	//windowwallright lr2 top right
         1.0f, -0.5f, -0.6f,  0.0f,  1.0f, 	-1.0f, 0.0f, 0.0f,	//windowwallright lr2 bottom right
         1.0f, -0.5f, -0.6f,  1.0f,  1.0f, 	-1.0f, 0.0f, 0.0f,	//windowwallright lr2 bottom right
         0.9f, -0.5f, -0.6f,  1.0f,  0.0f, 	-1.0f, 0.0f, 0.0f,	//windowwallright lr2 bottom left
         0.9f,  0.3f, -0.6f,  0.0f,  1.0f, 	-1.0f, 0.0f, 0.0f,	//windowwallright lr2 top left

        -0.5f, -0.5f, -0.5f,  1.0f,  1.0f, 	1.0f, 0.0f, 0.0f,	//backwall lr1 par base right
        -0.2f, -0.5f, -0.5f,  1.0f,  0.0f, 	1.0f, 0.0f, 0.0f,	//backwall lr1 par base left
        -0.2f,  0.2f, -0.5f,  0.0f,  1.0f, 	1.0f, 0.0f, 0.0f,	//backwall lr1 par top left
        -0.5f,  0.2f, -0.5f,  1.0f,  1.0f, 	1.0f, 0.0f, 0.0f,	//backwall lr1 par top left
        -0.2f,  0.2f, -0.5f,  1.0f,  0.0f, 	1.0f, 0.0f, 0.0f,	//backwall lr1 par top right
        -0.5f, -0.5f, -0.5f,  0.0f,  1.0f, 	1.0f, 0.0f, 0.0f,	//backwall lr1 par bottom right	

        -0.5f,  0.2f, -0.5f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backwall lr1 par base right
        -0.2f,  0.2f, -0.5f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//backwall lr1 par base left
        -0.2f,  0.5f, -0.5f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backwall lr1 par top left

        -0.6f, -0.5f, -0.6f,  1.0f,  1.0f, 	0.0f, -1.0f, 0.0f,	//backwall lr2 par base right
        -0.2f, -0.5f, -0.6f,  1.0f,  0.0f, 	0.0f, -1.0f, 0.0f,	//backwall lr2 par base left
        -0.2f,  0.2f, -0.6f,  0.0f,  1.0f, 	0.0f, -1.0f, 0.0f,	//backwall lr2 par top left
        -0.2f,  0.2f, -0.6f,  1.0f,  1.0f, 	0.0f, -1.0f, 0.0f,	//backwall lr2 par top left
        -0.6f,  0.2f, -0.6f,  1.0f,  0.0f, 	0.0f, -1.0f, 0.0f,	//backwall lr2 par top right
        -0.6f, -0.5f, -0.6f,  0.0f,  1.0f, 	0.0f, -1.0f, 0.0f,	//backwall lr2 par bottom right	

        -0.6f,  0.2f, -0.6f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backwall lr2 par base right
        -0.3f,  0.2f, -0.6f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//backwall lr2 par base left
        -0.3f,  0.5f, -0.6f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backwall lr2 par top left

        -0.6f,  0.2f, -0.6f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backwall lr2 par base right
        -0.2f,  0.2f, -0.6f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//backwall lr2 par base left
        -0.2f,  0.5f, -0.6f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backwall lr2 par top left

         1.0f,  0.5f, -0.5f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallright top left
         1.0f,  0.5f, -0.6f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallright top right
         1.0f, -0.5f, -0.6f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallright bottom right
         1.0f, -0.5f, -0.6f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallright bottom right
         1.0f, -0.5f, -0.5f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallright bottom left
         1.0f,  0.5f, -0.5f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallright top left

         0.9f,  0.3f, -0.5f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallright inner top left
         0.9f,  0.3f, -0.6f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallright inner top right
         0.9f, -0.5f, -0.6f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallright inner bottom right
         0.9f, -0.5f, -0.6f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallright inner bottom right
         0.9f, -0.5f, -0.5f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallright inner bottom left
         0.9f,  0.3f, -0.5f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallright inner top left

        -0.2f,  0.3f, -0.5f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallleft inner top left
        -0.2f,  0.3f, -0.6f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallleft inner top right
        -0.2f, -0.5f, -0.6f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallleft inner bottom right
        -0.2f, -0.5f, -0.6f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallleft inner bottom right
        -0.2f, -0.5f, -0.5f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallleft inner bottom left
        -0.2f,  0.3f, -0.5f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwallleft inner top left

         0.9f,  0.3f, -0.5f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwall topinner top left
         0.9f,  0.3f, -0.6f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//backmidwall topinner top right
        -0.2f,  0.3f, -0.6f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwall topinner bottom right
        -0.2f,  0.3f, -0.6f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwall topinner bottom right
        -0.2f,  0.3f, -0.5f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwall topinner bottom left
         0.9f,  0.3f, -0.5f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//backmidwall topinner top left

         0.9f,  0.3f, -0.5f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//windowlegright inner top left
         0.9f,  0.3f, -0.6f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//windowlegright inner top right
         0.9f, -0.5f, -0.6f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//windowlegright inner bottom right
         0.9f, -0.5f, -0.6f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//windowlegright inner bottom right
         0.9f, -0.5f, -0.5f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//windowlegright inner bottom left
         0.9f,  0.3f, -0.5f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//windowlegright inner top left

         0.9f,  0.3f, -0.5f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//windowwallright top left
         1.0f,  0.3f, -0.5f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//windowwallright top right
         1.0f, -0.5f, -0.5f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//windowwallright bottom right
         1.0f, -0.5f, -0.5f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//windowwallright bottom right
         0.9f, -0.5f, -0.5f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//windowwallright bottom left
         0.9f,  0.3f, -0.5f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//windowwallright top left


        -0.5f,  0.2f,  1.0f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//left wall top right
        -0.5f,  0.2f, -0.5f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//left wall top left
        -0.5f, -0.5f, -0.5f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//left wall bottom left
        -0.5f, -0.5f, -0.5f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//left wall bottom left
        -0.5f, -0.5f,  1.0f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//left wall bottom right
        -0.5f,  0.2f,  1.0f,  0.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//left wall top right

        -0.6f,  0.2f,  1.0f,  1.0f,  1.0f, 	0.0f, 0.0f, -1.0f,	//left wall lr2 top right
        -0.6f,  0.2f, -0.6f,  1.0f,  0.0f, 	0.0f, 0.0f, -1.0f,	//left wall lr2 top left
        -0.6f, -0.5f, -0.6f,  0.0f,  1.0f,  0.0f, 0.0f, -1.0f,	//left wall lr2 bottom left
        -0.6f, -0.5f, -0.6f,  1.0f,  1.0f,  0.0f, 0.0f, -1.0f,	//left wall lr2 bottom left
        -0.6f, -0.5f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, -1.0f,	//left wall lr2 bottom right
        -0.6f,  0.2f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, -1.0f,	//left wall lr2 top right

        -0.5f,  0.2f, -0.5f,  1.0f,  1.0f, 1.0f, 0.0f, 0.0f,	//cieling bottom left
        -0.2f,  0.5f, -0.5f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f,	//cieling top left
        -0.2f,  0.5f,  1.0f,  0.0f,  1.0f, 1.0f, 0.0f, 0.0f,	//cieling top right
        -0.2f,  0.5f,  1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 0.0f,	//cieling top right
        -0.5f,  0.2f,  1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f,	//cieling bottom right
        -0.5f,  0.2f, -0.5f,  0.0f,  1.0f, 1.0f, 0.0f, 0.0f,	//cieling bottom left

        -0.6f,  0.2f, -0.6f,  1.0f,  1.0f, 1.0f, 0.0f, 0.0f,	//cieling lr2 bottom left
        -0.3f,  0.5f, -0.6f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f,	//cieling lr2 top left
        -0.3f,  0.5f,  1.0f,  0.0f,  1.0f, 1.0f, 0.0f, 0.0f,	//cieling lr2 top right
        -0.3f,  0.5f,  1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 0.0f,	//cieling lr2 top right
        -0.6f,  0.2f,  1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f,	//cieling lr2 bottom right
        -0.6f,  0.2f, -0.6f,  0.0f,  1.0f, 1.0f, 0.0f, 0.0f,	//cieling lr2 bottom left

        -0.6f, -0.5f,  1.0f,  1.0f,  1.0f, 1.0f, 1.0f, 0.0f,	//lefmidwall(right) left
        -0.5f, -0.5f,  1.0f,  1.0f,  0.0f, 1.0f, 1.0f, 0.0f,	//lefmidwall(right) right
        -0.5f,  0.2f,  1.0f,  0.0f,  1.0f, 1.0f, 1.0f, 0.0f,	//lefmidwall(right) right
        -0.5f,  0.2f,  1.0f,  1.0f,  1.0f, 1.0f, 1.0f, 0.0f,	//lefmidwall(right) right
        -0.6f,  0.2f,  1.0f,  1.0f,  0.0f, 1.0f, 1.0f, 0.0f,	//lefmidwall(right) left
        -0.6f, -0.5f,  1.0f,  0.0f,  1.0f, 1.0f, 1.0f, 0.0f,	//lefmidwall(right) left

        -0.3f,  0.5f,  1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 0.0f,	//lefmidwallangle(right) left top
        -0.2f,  0.5f,  1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f,	//lefmidwallangle(right) right top
        -0.5f,  0.2f,  1.0f,  0.0f,  1.0f, 1.0f, 0.0f, 0.0f,	//lefmidwallangle(right) right ?bottom
        -0.6f,  0.2f,  1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 0.0f,	//lefmidwallangle(right) right ?bottom
        -0.5f,  0.2f,  1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f,	//lefmidwallangle(right) left ?bottom
        -0.3f,  0.5f,  1.0f,  0.0f,  1.0f, 1.0f, 0.0f, 0.0f,	//lefmidwallangle(right) left top
    
    };


    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerUV = 2;
    const GLuint floatsPerNorm = 3;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerUV + floatsPerNorm));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU


    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerUV + floatsPerNorm);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(1, floatsPerNorm, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerUV)));
    glEnableVertexAttribArray(1);

}


// Implements the UCreatePlaneMesh function
void UCreatePlaneMesh(GLMesh& mesh)
{
    // Vertex data
    GLfloat verts[] = {
        //Positions          //Texture Coordinates
        -5.0f, -0.6f, -5.0f,  1.0f,  1.0f, 		//plane top left
         5.0f, -0.6f, -5.0f,  1.0f,  0.0f, 		//plane top right
         5.0f, -0.6f,  5.0f,  0.0f,  1.0f, 		//plane bottom right
         5.0f, -0.6f,  5.0f,  1.0f,  1.0f, 		//plane bottom right
        -5.0f, -0.6f,  5.0f,  1.0f,  0.0f, 		//plane bottom left
        -5.0f, -0.6f, -5.0f,  0.0f,  1.0f, 		//plane top left
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerUV)); //sets a variable for the number of verices

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU


    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerUV);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);
    

}

// Implements the UCreateMesh function
void UCreateFloorMesh(GLMesh& mesh)
{
    // Vertex data
    GLfloat verts[] = {
        //Positions          //Texture Coordinates
        -0.5f, -0.5f, -0.5f,  0.0f,  1.0f, 		//floor toplr top left
         1.0f, -0.5f, -0.5f,  1.0f,  1.0f, 		//floor toplr top right
         1.0f, -0.5f,  1.0f,  1.0f,  0.0f, 		//floor toplr bottom right
         1.0f, -0.5f,  1.0f,  0.0f,  1.0f, 		//floor toplr bottom right
        -0.5f, -0.5f,  1.0f,  1.0f,  1.0f, 		//floor toplr bottom left
        -0.5f, -0.5f, -0.5f,  1.0f,  0.0f, 		//floor toplr top left

        -0.6f, -0.6f, -0.6f,  0.0f,  1.0f, 		//floor bottomlr top left
         1.0f, -0.6f, -0.6f,  1.0f,  1.0f, 		//floor bottomlr top right
         1.0f, -0.6f,  1.0f,  1.0f,  0.0f, 		//floor bottomlr bottom right
         1.0f, -0.6f,  1.0f,  0.0f,  1.0f, 		//floor bottomlr bottom right
        -0.6f, -0.6f,  1.0f,  1.0f,  1.0f, 		//floor bottomlr bottom left
        -0.6f, -0.6f, -0.6f,  1.0f,  0.0f, 		//floor bottomlr top left

        -0.6f, -0.5f, -0.6f,  0.0f,  1.0f, 		//backmidfloor base left
         1.0f, -0.5f, -0.6f,  1.0f,  1.0f, 		//backmidfloor base right
         1.0f, -0.6f, -0.6f,  1.0f,  0.0f, 		//backmidfloor top right
         1.0f, -0.6f, -0.6f,  0.0f,  1.0f, 		//backmidfloor top right
        -0.6f, -0.6f, -0.6f,  1.0f,  1.0f, 		//backmidfloor top left
        -0.6f, -0.5f, -0.6f,  1.0f,  0.0f, 		//backmidfloor bottom left

        -0.6f, -0.6f,  1.0f,  0.0f,  1.0f, 		//leftmidfloor top right
        -0.6f, -0.6f, -0.6f,  1.0f,  1.0f, 		//leftmidfloor top left
        -0.6f, -0.5f, -0.6f,  1.0f,  0.0f, 		//leftmidfloor bottom left
        -0.6f, -0.5f, -0.6f,  0.0f,  1.0f, 		//leftmidfloor bottom left
        -0.6f, -0.5f,  1.0f,  1.0f,  1.0f, 		//leftmidfloor bottom right
        -0.6f, -0.6f,  1.0f,  1.0f,  0.0f, 		//leftmidfloor top right

        -0.6f, -0.5f,  1.0f,  0.0f,  1.0f, 		//midfloor face bottom left
         1.0f, -0.5f,  1.0f,  1.0f,  1.0f, 		//midfloor face bottom right
         1.0f, -0.6f,  1.0f,  1.0f,  0.0f, 		//midfloor face top right
         1.0f, -0.6f,  1.0f,  0.0f,  1.0f, 		//midfloor face top right
        -0.6f, -0.6f,  1.0f,  1.0f,  1.0f, 		//midfloor face top left
        -0.6f, -0.5f,  1.0f,  1.0f,  0.0f, 		//midfloor face bottom left

         1.0f, -0.6f,  1.0f,  0.0f,  1.0f, 		//rightmidfloor top left
         1.0f, -0.6f, -0.6f,  1.0f,  1.0f, 		//rightmidfloor top right
         1.0f, -0.5f, -0.6f,  1.0f,  0.0f, 		//rightmidfloor bottom right
         1.0f, -0.5f, -0.6f,  0.0f,  1.0f, 		//rightmidfloor bottom right
         1.0f, -0.5f,  1.0f,  1.0f,  1.0f, 		//rightmidfloor bottom left
         1.0f, -0.6f,  1.0f,  1.0f,  0.0f, 		//rightmidfloor top left

    };


    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create VBO
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU


    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerUV);

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);

}

// Implements the UCreateLightMesh function
void UCreateLightMesh(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts[] = {
        //Positions          //Normals
        // ------------------------------------------------------
        //Back Face          //Negative Z Normal  Texture Coords.
       -5.0f,  0.3f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        2.0f,  0.3f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        2.0f,  0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        2.0f,  0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
       -5.0f,  0.5f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
       -5.0f,  0.3f, -1.0f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

       //Front Face         //Positive Z Normal
      -5.0f,  0.3f,  2.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
       2.0f,  0.3f,  2.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
       2.0f,  0.5f,  2.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
       2.0f,  0.5f,  2.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
      -5.0f,  0.5f,  2.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
      -5.0f,  0.3f,  2.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

      //Left Face          //Negative X Normal
     -5.0f,  0.5f,  2.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     -5.0f,  0.5f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     -5.0f,  0.3f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     -5.0f,  0.3f, -1.0f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     -5.0f,  0.3f,  2.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     -5.0f,  0.5f,  2.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

     //Right Face         //Positive X Normal
     2.0f,  0.5f,  2.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     2.0f,  0.5f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     2.0f,  0.3f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     2.0f,  0.3f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     2.0f,  0.3f,  2.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     2.0f,  0.5f,  2.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

     //Bottom Face        //Negative Y Normal
    -5.0f, 0.5f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     2.0f, 0.5f, -1.0f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     2.0f, 0.5f,  2.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     2.0f, 0.5f,  2.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -5.0f, 0.5f,  2.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -5.0f, 0.5f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    //Top Face           //Positive Y Normal
   -5.0f,  0.3f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
    2.0f,  0.3f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
    2.0f,  0.3f,  2.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    2.0f,  0.3f,  2.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
   -5.0f,  0.3f,  2.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
   -5.0f,  0.3f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
}




void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
}


/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);


        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}