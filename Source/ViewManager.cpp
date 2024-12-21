///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ===============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// Global constants
namespace {
    constexpr int WINDOW_WIDTH = 1000;
    constexpr int WINDOW_HEIGHT = 800;
    const char* VIEW_NAME = "view";
    const char* PROJECTION_NAME = "projection";

    // Camera and projection settings
    Camera* g_pCamera = nullptr;
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    float gDeltaTime = 0.0f;
    float gLastFrame = 0.0f;

    // Projection toggle
    bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
    ShaderManager* pShaderManager)
{
    // initialize the member variables
    m_pShaderManager = pShaderManager;
    m_pWindow = NULL;
    g_pCamera = new Camera();
    // default camera view parameters
    g_pCamera->Position = glm::vec3(0.0f, 2.0f, 12.0f);
    g_pCamera->Front = glm::vec3(0.0f, -0.15f, -4.0f);
    g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
    g_pCamera->Zoom = 50;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
    // free up allocated memory
    m_pShaderManager = NULL;
    m_pWindow = NULL;
    if (NULL != g_pCamera)
    {
        delete g_pCamera;
        g_pCamera = NULL;
    }
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
    GLFWwindow* window = nullptr;

    // try to create the displayed OpenGL window
    window = glfwCreateWindow(
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        windowTitle,
        NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return NULL;
    }
    glfwMakeContextCurrent(window);

    // this callback is used to receive mouse moving events
    glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

    // Set the scroll callback for zoom functionality
    glfwSetScrollCallback(window, &ViewManager::Scroll_Callback);

    // tell GLFW to capture all mouse events
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // enable blending for supporting transparent rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_pWindow = window;

    return(window);
}


/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos) {
    // Disable mouse movement when in orthographic projection
    if (bOrthographicProjection) return;
    
    if (gFirstMouse) {
        gLastX = xMousePos;
        gLastY = yMousePos;
        gFirstMouse = false;
    }

    float xOffset = xMousePos - gLastX;
    float yOffset = gLastY - yMousePos; // Inverted y-coordinates

    gLastX = xMousePos;
    gLastY = yMousePos;

    g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method captures and processes different inputs.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents() {
    if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_pWindow, true);
    }

    if (!g_pCamera) return;

    // Process camera movement
    if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS) g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS) g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS) g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS) g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS) g_pCamera->ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS) g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);

    // Switch between orthographic and perspective projections
    if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS) {
        // Switch to orthographic projection
        bOrthographicProjection = true;

        // Set the camera to face the front in orthographic view
        g_pCamera->Position = glm::vec3(-2.0f, 2.0f, 10.0f);
        g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
        g_pCamera->Front = glm::vec3(0.0f, 0.0f, -2.0f);
        g_pCamera->Zoom = 100.0f;
    }

    if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS) {
        // Switch to perspective projection
        bOrthographicProjection = false;

        // Set the camera to face the front in perspective view
        g_pCamera->Position = glm::vec3(-1.5f, 3.5f, 8.0f);
        g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
        g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
        g_pCamera->Zoom = 80.0f;                            
    }
}

/***********************************************************
 *  Scroll_Callback()
 *
 *  Callback function for handling scroll events.
 *  Adjusts the zoom level of the camera.
 ***********************************************************/

void ViewManager::Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset) {
    g_pCamera->ProcessMouseScroll((float)yOffset);
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
    glm::mat4 view;
    glm::mat4 projection;

    // per-frame timing
    float currentFrame = glfwGetTime();
    gDeltaTime = currentFrame - gLastFrame;
    gLastFrame = currentFrame;

    // process any keyboard events that may be waiting in the 
    // event queue
    ProcessKeyboardEvents();

    // get the current view matrix from the camera
    view = g_pCamera->GetViewMatrix();

    // define the current projection matrix
    if (bOrthographicProjection == false)
    {
        // perspective projection
        projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else
    {
        // front-view orthographic projection with correct aspect ratio
        double scale = 0.0;
        if (WINDOW_WIDTH > WINDOW_HEIGHT)
        {
            scale = (double)WINDOW_HEIGHT / (double)WINDOW_WIDTH;
            projection = glm::ortho(-5.0f, 5.0f, -5.0f * (float)scale, 5.0f * (float)scale, 0.1f, 100.0f);
        }
        else if (WINDOW_WIDTH < WINDOW_HEIGHT)
        {
            scale = (double)WINDOW_WIDTH / (double)WINDOW_HEIGHT;
            projection = glm::ortho(-5.0f * (float)scale, 5.0f * (float)scale, -5.0f, 5.0f, 0.1f, 100.0f);
        }
        else
        {
            projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
        }
    }

    // if the shader manager object is valid
    if (NULL != m_pShaderManager)
    {
        // set the view matrix into the shader for proper rendering
        m_pShaderManager->setMat4Value(VIEW_NAME, view);
        // set the view matrix into the shader for proper rendering
        m_pShaderManager->setMat4Value(PROJECTION_NAME, projection);
        // set the view position of the camera into the shader for proper rendering
        m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
    }
}