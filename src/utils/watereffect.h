#ifndef WATEREFFECT_H
#define WATEREFFECT_H

#pragma once

#include <glm/glm.hpp>
#include <GL/glew.h>

// Header class for:
// - loading water color texture and displacement texture
// - keeping track of internal time
// - uploading all uniforms and binding textures each frame

class WaterEffect
{
public:
    WaterEffect();

    // Call once after OpenGL context is ready
    void init(GLuint waterDispTex);

    // Call once per frame with delta time (seconds)
    void update(float deltaSeconds);

    void apply(GLuint shaderProgram);

    void cleanup();

private:
    float m_time;
    GLuint m_waterDispTex;

    // Config params
    glm::vec2 m_waterTexScale;
    glm::vec2 m_dispTexScale;
    glm::vec2 m_dispScrollDir;
    float     m_dispScrollSpeed;
    float     m_dispStrength;
    float     m_dispContrast;

};

#endif // WATEREFFECT_H
