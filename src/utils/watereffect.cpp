#include "watereffect.h"

#include <QImage>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

WaterEffect::WaterEffect()
    : m_time(0.0f)
    , m_waterColorTex(0)
    , m_waterDispTex(0)
    , m_waterTexScale(4.0f, 4.0f)
    , m_dispTexScale(1.5f, 1.5f)
    , m_dispScrollDir(0.7f, 0.3f)
    , m_dispScrollSpeed(0.15f)
    , m_dispStrength(0.05f)
    , m_dispContrast(1.5f)
    , m_planeY(-0.55f)         // top of your platform cube in world space
    , m_planeThickness(0.03f)  // tolerance band around that height
    , m_enabled(true)
{}

void WaterEffect::init(GLuint waterColorTex, GLuint waterDispTex) {

    m_waterColorTex = waterColorTex;
    m_waterDispTex = waterDispTex;

    if (!m_waterColorTex || !m_waterDispTex) {
        std::cerr << "[WaterEffect] failed to load water/displacement texture map";
    }
}

void WaterEffect::update(float deltaSeconds) {
    m_time += deltaSeconds;
}

void WaterEffect::apply(GLuint shaderProgram) {

    // If disabled or texture not loaded, tell shader to turn it off
    GLint enableLoc = glGetUniformLocation(shaderProgram, "u_enableWater");
    if (!m_enabled || !m_waterColorTex || !m_waterDispTex) {
        if (enableLoc >= 0) {
            glUniform1i(enableLoc, 0);
        }
        return;
    }

    if (enableLoc >= 0) {
        glUniform1i(enableLoc, 1);
    }

    // Time
    GLuint timeLoc = glGetUniformLocation(shaderProgram, "u_time");
    if (timeLoc >= 0) glUniform1f(timeLoc, m_time);

    // Plane location
    GLint planeYLoc = glGetUniformLocation(shaderProgram, "u_waterPlaneY");
    GLint thickLoc = glGetUniformLocation(shaderProgram, "u_waterPlaneThickness");
    if (planeYLoc >= 0) glUniform1f(planeYLoc, m_planeY);
    if (thickLoc >= 0) glUniform1f(thickLoc, m_planeThickness);

    // Water & displacement parameters
    GLint waterScaleLoc = glGetUniformLocation(shaderProgram, "u_waterTexScale");
    GLint dispScaleLoc = glGetUniformLocation(shaderProgram, "u_dispTexScale");
    GLint dispScrollDirLoc = glGetUniformLocation(shaderProgram, "u_dispScrollDir");
    GLint dispScrollSpeedLoc = glGetUniformLocation(shaderProgram, "u_dispScrollSpeed");
    GLint dispStrengthLoc = glGetUniformLocation(shaderProgram, "u_dispStrength");
    GLint dispContrastLoc = glGetUniformLocation(shaderProgram, "u_dispContrast");

    glUniform2fv(waterScaleLoc, 1, glm::value_ptr(m_waterTexScale));
    glUniform2fv(dispScaleLoc, 1, glm::value_ptr(m_dispTexScale));
    glUniform2fv(dispScrollDirLoc, 1, glm::value_ptr(m_dispScrollDir));
    glUniform1f(dispScrollSpeedLoc, m_dispScrollSpeed);
    glUniform1f(dispStrengthLoc, m_dispStrength);
    glUniform1f(dispContrastLoc, m_dispContrast);

    // Bind textures
    glActiveTexture(GL_TEXTURE0 + 10);
    glBindTexture(GL_TEXTURE_2D, m_waterColorTex);
    GLint waterTexLoc = glGetUniformLocation(shaderProgram, "u_waterTex");
    glUniform1i(waterTexLoc, 10);

    glActiveTexture(GL_TEXTURE0 + 11);
    glBindTexture(GL_TEXTURE_2D, m_waterDispTex);
    GLint waterDispTexLoc = glGetUniformLocation(shaderProgram, "u_dispTex");
    glUniform1i(waterDispTexLoc, 11);

}

void WaterEffect::cleanup()
{
    if (m_waterColorTex) {
        glDeleteTextures(1, &m_waterColorTex);
        m_waterColorTex = 0;
    }
    if (m_waterDispTex) {
        glDeleteTextures(1, &m_waterDispTex);
        m_waterDispTex = 0;
    }
}

