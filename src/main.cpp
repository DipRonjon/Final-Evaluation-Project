/**
 * Solar System Simulation — OpenGL / C++ Scenario-Based Project
 * =============================================================
 *
 * Scene description
 * -----------------
 * The Sun sits at the origin.  Six planets (Mercury, Venus, Earth, Mars,
 * Jupiter, Saturn) orbit it at different distances and angular speeds.
 * Earth has a Moon orbiting it (hierarchical transformation).
 * Saturn has a simple ring rendered as a flat torus.
 *
 * Controls
 * --------
 *   W / S          Move camera forward / backward
 *   A / D          Strafe left / right
 *   Q / E          Fly up / down
 *   Mouse          Look around (captured by default)
 *   Scroll wheel   Zoom (change FOV)
 *   F              Toggle wireframe mode
 *   SPACE          Pause / resume orbital animation
 *   ESC            Quit
 *
 * Rendering techniques demonstrated
 * ----------------------------------
 *   - Procedural UV-sphere geometry (VAO / VBO / EBO)
 *   - Blinn-Phong lighting (ambient + diffuse + specular)
 *   - Emissive sun rendered with a separate unlit shader
 *   - Hierarchical model matrices (moon orbiting Earth)
 *   - Delta-time animation
 *   - Perspective projection via GLM
 */

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <vector>
#include <iostream>
#include <stdexcept>

#include "Shader.h"
#include "Camera.h"

// ---------------------------------------------------------------------------
// Window dimensions
// ---------------------------------------------------------------------------
static constexpr int SCREEN_W = 1280;
static constexpr int SCREEN_H = 720;

// ---------------------------------------------------------------------------
// Global camera
// ---------------------------------------------------------------------------
static Camera camera(glm::vec3(0.0f, 20.0f, 80.0f));
static float lastX      = SCREEN_W / 2.0f;
static float lastY      = SCREEN_H / 2.0f;
static bool  firstMouse = true;

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------
static float deltaTime = 0.0f;
static float lastFrame = 0.0f;

// ---------------------------------------------------------------------------
// Simulation state
// ---------------------------------------------------------------------------
static bool wireframe = false;
static bool paused    = false;

// ---------------------------------------------------------------------------
// Geometry helpers
// ---------------------------------------------------------------------------

/**
 * Build a UV-sphere of the given radius.
 * Vertices interleaved as [x, y, z, nx, ny, nz].
 */
static void buildSphere(float radius, int sectors, int stacks,
                        std::vector<float>&        vertices,
                        std::vector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();

    const float pi = glm::pi<float>();

    for (int i = 0; i <= stacks; ++i) {
        float phi = pi / 2.0f - static_cast<float>(i) * pi / static_cast<float>(stacks);
        float y   = radius * std::sin(phi);
        float xzr = radius * std::cos(phi);

        for (int j = 0; j <= sectors; ++j) {
            float theta = static_cast<float>(j) * 2.0f * pi / static_cast<float>(sectors);
            float x = xzr * std::cos(theta);
            float z = xzr * std::sin(theta);

            // position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // normal (unit sphere — normal == position / radius)
            vertices.push_back(x / radius);
            vertices.push_back(y / radius);
            vertices.push_back(z / radius);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;
        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(static_cast<unsigned int>(k1));
                indices.push_back(static_cast<unsigned int>(k2));
                indices.push_back(static_cast<unsigned int>(k1 + 1));
            }
            if (i != stacks - 1) {
                indices.push_back(static_cast<unsigned int>(k1 + 1));
                indices.push_back(static_cast<unsigned int>(k2));
                indices.push_back(static_cast<unsigned int>(k2 + 1));
            }
        }
    }
}

/** Upload geometry to GPU, return VAO id.  EBO index count stored in *count. */
static GLuint uploadSphere(const std::vector<float>&        verts,
                           const std::vector<unsigned int>& idxs,
                           GLsizei&                         count)
{
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                 verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(idxs.size() * sizeof(unsigned int)),
                 idxs.data(), GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    count = static_cast<GLsizei>(idxs.size());
    return vao;
}

// ---------------------------------------------------------------------------
// Ring geometry (flat torus approximation for Saturn's ring)
// ---------------------------------------------------------------------------
static GLuint buildRing(float innerR, float outerR, int segments, GLsizei& count)
{
    std::vector<float>        verts;
    std::vector<unsigned int> idxs;

    const float pi = glm::pi<float>();
    const glm::vec3 normal(0.0f, 1.0f, 0.0f);

    for (int i = 0; i <= segments; ++i) {
        float theta = static_cast<float>(i) * 2.0f * pi / static_cast<float>(segments);
        float c = std::cos(theta);
        float s = std::sin(theta);

        // outer vertex
        verts.push_back(outerR * c); verts.push_back(0.0f); verts.push_back(outerR * s);
        verts.push_back(normal.x);   verts.push_back(normal.y); verts.push_back(normal.z);

        // inner vertex
        verts.push_back(innerR * c); verts.push_back(0.0f); verts.push_back(innerR * s);
        verts.push_back(normal.x);   verts.push_back(normal.y); verts.push_back(normal.z);
    }

    for (int i = 0; i < segments; ++i) {
        unsigned int base = static_cast<unsigned int>(i) * 2;
        idxs.push_back(base);     idxs.push_back(base + 2); idxs.push_back(base + 1);
        idxs.push_back(base + 1); idxs.push_back(base + 2); idxs.push_back(base + 3);
    }

    return uploadSphere(verts, idxs, count);
}

// ---------------------------------------------------------------------------
// Planet descriptor
// ---------------------------------------------------------------------------
struct Planet {
    std::string name;
    float       radius;        // sphere radius (AU-inspired scale)
    float       orbitRadius;   // distance from parent
    float       orbitSpeed;    // radians per simulated second
    float       selfRotSpeed;  // radians per simulated second
    glm::vec3   color;
    float       angle;         // current orbital angle (radians)
    float       selfAngle;     // current self-rotation angle
};

// ---------------------------------------------------------------------------
// GLFW callbacks
// ---------------------------------------------------------------------------
static void framebufferSizeCallback(GLFWwindow* /*win*/, int w, int h)
{
    glViewport(0, 0, w, h);
}

static void mouseCallback(GLFWwindow* /*win*/, double xpos, double ypos)
{
    auto fx = static_cast<float>(xpos);
    auto fy = static_cast<float>(ypos);

    if (firstMouse) {
        lastX = fx;
        lastY = fy;
        firstMouse = false;
    }

    float xoffset = fx - lastX;
    float yoffset = lastY - fy;   // reversed: y-coords go bottom-to-top
    lastX = fx;
    lastY = fy;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

static void scrollCallback(GLFWwindow* /*win*/, double /*xoffset*/, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

static void keyCallback(GLFWwindow* win, int key, int /*scancode*/,
                        int action, int /*mods*/)
{
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, true);
        if (key == GLFW_KEY_F) {
            wireframe = !wireframe;
            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        }
        if (key == GLFW_KEY_SPACE) paused = !paused;
    }
}

// ---------------------------------------------------------------------------
// Input processing (held keys)
// ---------------------------------------------------------------------------
static void processInput(GLFWwindow* win)
{
    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::FORWARD,  deltaTime);
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::BACKWARD, deltaTime);
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::LEFT,     deltaTime);
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::RIGHT,    deltaTime);
    if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::DOWN,     deltaTime);
    if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::UP,       deltaTime);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main()
{
    // -----------------------------------------------------------------------
    // Init GLFW
    // -----------------------------------------------------------------------
    if (!glfwInit()) {
        std::cerr << "Failed to initialise GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);   // 4× MSAA

    GLFWwindow* window = glfwCreateWindow(SCREEN_W, SCREEN_H,
                                          "Solar System – OpenGL", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // -----------------------------------------------------------------------
    // Init GLEW
    // -----------------------------------------------------------------------
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialise GLEW\n";
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glViewport(0, 0, SCREEN_W, SCREEN_H);

    // -----------------------------------------------------------------------
    // Load shaders
    // -----------------------------------------------------------------------
    Shader planetShader("shaders/planet.vert", "shaders/planet.frag");
    Shader sunShader   ("shaders/sun.vert",    "shaders/sun.frag");

    // -----------------------------------------------------------------------
    // Build sphere geometry (shared by all bodies)
    // -----------------------------------------------------------------------
    std::vector<float>        sphereVerts;
    std::vector<unsigned int> sphereIdx;
    buildSphere(1.0f, 36, 18, sphereVerts, sphereIdx);

    GLsizei sphereCount = 0;
    GLuint  sphereVAO   = uploadSphere(sphereVerts, sphereIdx, sphereCount);

    // Saturn ring
    GLsizei ringCount = 0;
    GLuint  ringVAO   = buildRing(1.6f, 2.6f, 64, ringCount);

    // -----------------------------------------------------------------------
    // Planet definitions
    // -----------------------------------------------------------------------
    //                name        r    orbitR  orbitSpd  selfSpd          colour
    Planet mercury = {"Mercury", 0.4f,  8.0f,  1.607f,  10.83f, {0.72f, 0.60f, 0.50f}, 0.0f, 0.0f};
    Planet venus   = {"Venus",   0.9f, 13.0f,  1.174f,  -6.52f, {0.92f, 0.75f, 0.40f}, 1.0f, 0.0f};
    Planet earth   = {"Earth",   1.0f, 19.0f,  1.000f,  10.00f, {0.25f, 0.55f, 0.87f}, 2.0f, 0.0f};
    Planet moon    = {"Moon",    0.27f, 2.6f,  3.650f,   3.65f, {0.75f, 0.75f, 0.75f}, 0.0f, 0.0f};
    Planet mars    = {"Mars",    0.55f,26.0f,  0.802f,  10.04f, {0.87f, 0.38f, 0.20f}, 3.5f, 0.0f};
    Planet jupiter = {"Jupiter", 2.8f, 40.0f,  0.434f,  25.37f, {0.83f, 0.72f, 0.62f}, 0.5f, 0.0f};
    Planet saturn  = {"Saturn",  2.3f, 55.0f,  0.323f,  22.58f, {0.90f, 0.83f, 0.65f}, 4.0f, 0.0f};
    Planet uranus  = {"Uranus",  1.5f, 68.0f,  0.228f, -14.29f, {0.56f, 0.89f, 0.95f}, 1.2f, 0.0f};
    Planet neptune = {"Neptune", 1.4f, 80.0f,  0.182f,  15.97f, {0.22f, 0.41f, 0.87f}, 5.0f, 0.0f};

    // Convenience list for planets that orbit the Sun directly
    std::vector<Planet*> solarPlanets = {
        &mercury, &venus, &earth, &mars, &jupiter, &saturn, &uranus, &neptune
    };

    // Sun properties
    const float     sunRadius = 5.0f;
    const glm::vec3 sunColor  = {1.0f, 0.85f, 0.20f};
    const glm::vec3 sunPos    = {0.0f, 0.0f,  0.0f};
    const glm::vec3 lightColor= {1.0f, 0.98f, 0.92f};

    // Simulation speed multiplier (increase to speed things up)
    const float simSpeed = 0.5f;

    // -----------------------------------------------------------------------
    // Render loop
    // -----------------------------------------------------------------------
    std::cout << "=== Solar System Simulation ===\n"
              << "  WASD / QE   : move camera\n"
              << "  Mouse       : look around\n"
              << "  Scroll      : zoom\n"
              << "  F           : toggle wireframe\n"
              << "  SPACE       : pause / resume\n"
              << "  ESC         : quit\n\n";

    while (!glfwWindowShouldClose(window)) {
        // --- timing --------------------------------------------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // --- input ---------------------------------------------------------
        processInput(window);

        // --- update orbital angles ----------------------------------------
        if (!paused) {
            float dt = deltaTime * simSpeed;
            for (Planet* p : solarPlanets) {
                p->angle     += p->orbitSpeed * dt;
                p->selfAngle += p->selfRotSpeed * dt;
            }
            moon.angle     += moon.orbitSpeed * dt;
            moon.selfAngle += moon.selfRotSpeed * dt;
        }

        // --- clear ---------------------------------------------------------
        glClearColor(0.01f, 0.01f, 0.05f, 1.0f);   // near-black space
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- common matrices -----------------------------------------------
        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        float aspect = fbH > 0 ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0f;

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom), aspect, 0.1f, 500.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // --- draw Sun (unlit) ----------------------------------------------
        {
            sunShader.use();
            sunShader.setMat4("projection", projection);
            sunShader.setMat4("view",       view);

            glm::mat4 model = glm::scale(glm::mat4(1.0f),
                                         glm::vec3(sunRadius));
            sunShader.setMat4("model",    model);
            sunShader.setVec3("sunColor", sunColor);

            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, sphereCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }

        // --- draw planets (Blinn-Phong lit) ---------------------------------
        planetShader.use();
        planetShader.setMat4("projection", projection);
        planetShader.setMat4("view",       view);
        planetShader.setVec3("lightPos",   sunPos);
        planetShader.setVec3("viewPos",    camera.Position);
        planetShader.setVec3("lightColor", lightColor);

        glBindVertexArray(sphereVAO);

        for (Planet* p : solarPlanets) {
            // Position on orbit
            float px = p->orbitRadius * std::cos(p->angle);
            float pz = p->orbitRadius * std::sin(p->angle);
            glm::vec3 planetPos(px, 0.0f, pz);

            glm::mat4 model = glm::translate(glm::mat4(1.0f), planetPos);
            model = glm::rotate(model, p->selfAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale (model, glm::vec3(p->radius));

            planetShader.setMat4("model",       model);
            planetShader.setVec3("objectColor", p->color);

            glDrawElements(GL_TRIANGLES, sphereCount, GL_UNSIGNED_INT, nullptr);

            // --- Moon (child of Earth) ------------------------------------
            if (p == &earth) {
                float mx = planetPos.x + moon.orbitRadius * std::cos(moon.angle);
                float mz = planetPos.z + moon.orbitRadius * std::sin(moon.angle);
                glm::vec3 moonPos(mx, 0.0f, mz);

                glm::mat4 moonModel = glm::translate(glm::mat4(1.0f), moonPos);
                moonModel = glm::rotate(moonModel, moon.selfAngle,
                                        glm::vec3(0.0f, 1.0f, 0.0f));
                moonModel = glm::scale(moonModel, glm::vec3(moon.radius));

                planetShader.setMat4("model",       moonModel);
                planetShader.setVec3("objectColor", moon.color);

                glDrawElements(GL_TRIANGLES, sphereCount, GL_UNSIGNED_INT, nullptr);
            }

            // --- Saturn's ring --------------------------------------------
            if (p == &saturn) {
                glBindVertexArray(ringVAO);

                glm::mat4 ringModel = glm::translate(glm::mat4(1.0f), planetPos);
                ringModel = glm::rotate(ringModel, glm::radians(20.0f),
                                        glm::vec3(1.0f, 0.0f, 0.0f));
                ringModel = glm::scale(ringModel, glm::vec3(p->radius));

                planetShader.setMat4("model",       ringModel);
                planetShader.setVec3("objectColor", glm::vec3(0.78f, 0.72f, 0.60f));

                glDrawElements(GL_TRIANGLES, ringCount, GL_UNSIGNED_INT, nullptr);

                glBindVertexArray(sphereVAO);   // restore for next planet
            }
        }

        glBindVertexArray(0);

        // --- swap buffers --------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // -----------------------------------------------------------------------
    // Clean up
    // -----------------------------------------------------------------------
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteVertexArrays(1, &ringVAO);
    glfwTerminate();
    return 0;
}
