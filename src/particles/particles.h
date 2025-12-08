#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "iostream"
#include <glm/gtx/norm.hpp> // Required for glm::length2

class Particles {
public:
    Particles();
    ~Particles();

    void init(GLuint shaderProgram);
    void update(float dt, const glm::vec3& cameraPos); // simulate & stream to GPU
    void render(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &cameraPos);
    void cleanup();

    void setCameraPos(const glm::vec3 &pos) { m_lastCameraPos = pos; }
    // Public setter (add to public section)
    void setCameraData(const glm::vec3 &camPos,
                       const glm::vec3 &camForward,
                       const glm::vec3 &camRight,
                       const glm::vec3 &camUp,
                       float camFovRadians,
                       float camAspect,
                       float spawnDistance = 8.0f) {
        m_camPos = camPos;
        m_camForward = camForward;
        m_camRight = camRight;
        m_camUp = camUp;
        m_camFov = camFovRadians;
        m_camAspect = camAspect;
        m_spawnDistance = spawnDistance;};


private:
    // CPU particle struct (matches tutorial)
    struct Particle {
        glm::vec3 pos;
        glm::vec3 speed;
        unsigned char r,g,b,a;
        float size;
        float life;
        float cameradistance;
        bool operator<(const Particle& that) const {
            // Dead particles go to the end
            if (life < 0.0f && that.life < 0.0f) return false;
            if (life < 0.0f) return false;
            if (that.life < 0.0f) return true;
            // Sort living particles by distance (far to near)
            return cameradistance > that.cameradistance;
        }
    };

    // configuration
    static const int MaxParticles = 100;

    // CPU containers
    std::vector<Particle> m_particlesContainer;
    std::vector<float> g_particule_position_size_data; // MaxParticles * 4
    std::vector<unsigned char> g_particule_color_data; // MaxParticles * 4

    // GL resources
    GLuint m_shader = 0u;         // program (not owned)
    GLuint m_vao = 0u;
    GLuint m_billboard_vbo = 0u;  // base quad (4 verts)
    GLuint m_positions_vbo = 0u;  // per-instance vec4 (x,y,z,size)
    GLuint m_colors_vbo = 0u;     // per-instance ubyte4 (r,g,b,a)
    GLuint m_tex = 0u;

    // runtime counters / state
    int LastUsedParticle = 0;
    int ParticlesCount = 0;
    glm::vec3 m_lastCameraPos = glm::vec3(0.0f);

    // helpers
    int FindUnusedParticle();
    void spawnNewParticles(int newCount);
    void initInstancedBuffers();
    void uploadInstanceBuffers();
    void loadTextureFromResource(const char* resourcePath);
    // Snow tuning (add to private section)
    float snow_spawn_width = 6.0f;   // total width in world units across which flakes spawn
    float snow_spawn_depth = 2.0f;   // depth range (along view axis) particles spawn in
    float snow_spawn_height = 4.0f;  // height above origin to spawn
    glm::vec3 snow_wind = glm::vec3(0.25f, 0.0f, 0.0f); // horizontal wind velocity applied to flakes
    float snow_gravity = -0.5f;      // gentle downward accel
    bool snow_kill_on_ground = true; // kill flakes when they hit groundY
    float groundY = -1.0f;           // ground plane world Y coord
    // --- camera-follow spawn parameters (add to private section)
    glm::vec3 m_camPos = glm::vec3(0.0f);
    glm::vec3 m_camForward = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_camRight = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 m_camUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float m_camFov = glm::radians(45.0f); // vertical FOV in radians (height angle)
    float m_camAspect = 1.3333f;          // width / height
    float m_spawnDistance = 8.0f;         // how far in front of camera to spawn flakes (world units)



};
