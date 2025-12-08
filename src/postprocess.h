#pragma once

#include <GL/glew.h>
#include <QString>

class PostProcess{
public:
    // Create shader, fullscreen quad, FBO, and LUT texture.
    void init(int screenWidth, int screenHeight, const QString &lutPath);

    // Call whenever the window size changes to recreate FBO
    void resize(int screenWidth, int screenHeight);

    // Free GL resources
    void destroy();

    // Draw fullscreen quad using the scene texture and LUT
    void drawToScreen(float gradeStrength, bool enableCG,
                  bool dofEnabled,
                  float nearPlane,
                  float farPlane,
                  float focusPlane,
                  bool settingsCG,
                  bool watercolorEnabled,
                  bool pixelatedEnabled,
                  float pixelSize,
                      bool isNight);

    // Access the offscreen FBO so Realtime can render into it
    GLuint fbo() const { return m_fbo; };

    // The color texture that contains the rendered scene
    GLuint sceneTexture() const { return m_fbo_texture; }

private:
    void makeFullscreenQuad();
    void makeFBO();
    void loadLUT(const QString &lutPath);
    void cacheUniformLocations();

    int m_fboWidth  = 0;
    int m_fboHeight = 0;

    GLuint m_fbo = 0;
    GLuint m_fbo_texture = 0;
    GLuint m_fbo_renderbuffer = 0;
    GLuint m_fbo_depth_texture = 0;  // used for DOF

    GLuint m_fullscreen_vao = 0;
    GLuint m_fullscreen_vbo = 0;

    GLuint m_lutTexture = 0;

    GLuint m_postprocessShader = 0;
    GLint  m_loc_tOrig = -1;
    GLint  m_loc_tLUT = -1;
    GLint  m_loc_gradeStr = -1;
    GLint  m_loc_enableCG = -1;
};
