#include "postprocess.h"

#include <QImage>
#include <QDebug>
#include "src/shaderloader.h"     // same loader you use in Realtime :contentReference[oaicite:1]{index=1}
#include <glm/gtc/type_ptr.hpp>

void PostProcess::init(int screenWidth, int screenHeight,
                       const QString &lutPath)
{
    m_fboWidth  = screenWidth;
    m_fboHeight = screenHeight;

    m_postprocessShader = ShaderLoader::createShaderProgram(
        ":/resources/shaders/texture.vert",
        ":/resources/shaders/texture.frag"
        );

    cacheUniformLocations();
    makeFullscreenQuad();
    makeFBO();
    loadLUT(lutPath);
}

void PostProcess::resize(int screenWidth, int screenHeight) {
    m_fboWidth = screenWidth;
    m_fboHeight = screenHeight;
    makeFBO();
}

void PostProcess::destroy() {

    // Clean up post-processing resources
    if (m_fbo_texture) {
        glDeleteTextures(1, &m_fbo_texture);
        m_fbo_texture = 0;
    }
    if (m_fbo_renderbuffer) {
        glDeleteRenderbuffers(1, &m_fbo_renderbuffer);
        m_fbo_renderbuffer = 0;
    }
    if (m_fbo) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }

    if (m_fullscreen_vbo) {
        glDeleteBuffers(1, &m_fullscreen_vbo);
        m_fullscreen_vbo = 0;
    }
    if (m_fullscreen_vao) {
        glDeleteVertexArrays(1, &m_fullscreen_vao);
        m_fullscreen_vao = 0;
    }

    if (m_lutTexture) {
        glDeleteTextures(1, &m_lutTexture);
        m_lutTexture = 0;
    }

    if (m_postprocessShader) {
        glDeleteProgram(m_postprocessShader);
        m_postprocessShader = 0;
    }

    if (m_fbo_depth_texture) {
        glDeleteTextures(1, &m_fbo_depth_texture);
        m_fbo_depth_texture = 0;
    }
}

void PostProcess::makeFullscreenQuad() {
    // Build fullscreen quad geometry (from lab 11)
    std::vector<GLfloat> fullscreen_quad_data =
        {
            -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, // top-left (0,1)
            -1.0f, -1.0f,  0.0f,  0.0f,  0.0f, // bottom-left (0,0)
            1.0f, -1.0f,  0.0f,  1.0f,  0.0f, // bottom-right (1,0)

            1.0f,  1.0f,  0.0f,  1.0f,  1.0f, // top-right (1,1)
            -1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  // top-left (0,1)
            1.0f, -1.0f,  0.0f,  1.0f,  0.0f  // bottom-right (1,0)
        };

    glGenBuffers(1, &m_fullscreen_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_fullscreen_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 fullscreen_quad_data.size() * sizeof(GLfloat),
                 fullscreen_quad_data.data(),
                 GL_STATIC_DRAW);

    glGenVertexArrays(1, &m_fullscreen_vao);
    glBindVertexArray(m_fullscreen_vao);

    // Position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(0));

    // UV attribute (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void PostProcess::makeFBO() {
    if (m_fbo_texture)      glDeleteTextures(1, &m_fbo_texture);
    if (m_fbo_renderbuffer) glDeleteRenderbuffers(1, &m_fbo_renderbuffer);
    if (m_fbo)              glDeleteFramebuffers(1, &m_fbo);
    if (m_fbo_depth_texture) glDeleteTextures(1, &m_fbo_depth_texture); //dof

    // Generate and bind an empty texture, set its min/mag filter interpolation, then unbind
    glGenTextures(1, &m_fbo_texture);
    // glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fbo_texture);

    // Allocate empty texture storage (no data, just size and format
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_fboWidth, m_fboHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0); // done configuring texture

    //Depth Tex!!
    glGenTextures(1, &m_fbo_depth_texture);
    glBindTexture(GL_TEXTURE_2D, m_fbo_depth_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
                 m_fboWidth, m_fboHeight, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);


    // Generate and bind a renderbuffer of the right size, set its format, then unbind
    glGenRenderbuffers(1, &m_fbo_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_fbo_renderbuffer);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_fboWidth, m_fboHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Generate and bind an FBO
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    //Add our texture as a color attachment, and our renderbuffer as a depth+stencil attachment, to our FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, m_fbo_depth_texture, 0);

    // Leave whatever was bound before up to caller
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcess::loadLUT(const QString &lutPath) {

    QImage lutImage(lutPath);

    if (lutImage.isNull()) {
        qWarning() << "Failed to load LUT texture from" << lutPath;
        return;
    }

    lutImage = lutImage.convertToFormat(QImage::Format_RGBA8888).mirrored();
    glGenTextures(1, &m_lutTexture);

    // Use texture unit 1 for LUT (0 for scene)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_lutTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 lutImage.width(), lutImage.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, lutImage.bits());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
}

void PostProcess::drawToScreen(float gradeStrength,
                               bool enableCG,
                               bool dofEnabled,
                               float nearPlane,
                               float farPlane,
                               float focusPlane,
                               bool settingsCG,
                               bool watercolorEnabled,
                               bool pixelatedEnabled,
                               float pixelSize,
                               bool isNight) {
    glUseProgram(m_postprocessShader);

    // Scene color texture (the one attached to our FBO) -> unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
    glUniform1i(m_loc_tOrig, 0);

    GLint locSceneColor = glGetUniformLocation(m_postprocessShader, "sceneColorTex");
    if (locSceneColor >= 0) {
        glUniform1i(locSceneColor, 0); // new name: sceneColorTex
    }

    // Scene depth texture -> unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_fbo_depth_texture);
    GLint locSceneDepth = glGetUniformLocation(m_postprocessShader, "sceneDepthTex");
    if (locSceneDepth >= 0) {
        glUniform1i(locSceneDepth, 1);
    }

    // LUT texture -> unit 2
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_lutTexture);
    glUniform1i(m_loc_tLUT, 2);

    // Optional grading controls
    if (m_loc_gradeStr >= 0) {
        glUniform1f(m_loc_gradeStr, gradeStrength);
    }
    if (m_loc_enableCG >= 0) {
        glUniform1i(m_loc_enableCG, enableCG ? 1 : 0);
    }

    //pass in settings CG
    GLint useCGLoc   = glGetUniformLocation(m_postprocessShader, "useColorGrade");
    if (useCGLoc >= 0)   glUniform1i(useCGLoc,   settingsCG ? 1 : 0);

    //dof uniforms
    GLint locDof   = glGetUniformLocation(m_postprocessShader, "dof");
    if (locDof >= 0)   glUniform1i(locDof,   dofEnabled ? 1 : 0);

    GLint locNear  = glGetUniformLocation(m_postprocessShader, "uNear");
    GLint locFar   = glGetUniformLocation(m_postprocessShader, "uFar");
    GLint locFocus = glGetUniformLocation(m_postprocessShader, "uFocus");
    if (locNear  >= 0) glUniform1f(locNear,  nearPlane);
    if (locFar   >= 0) glUniform1f(locFar,   farPlane);
    if (locFocus >= 0) glUniform1f(locFocus, focusPlane);

    // Real inverse screen size for blur radius
    GLint locInvScreen = glGetUniformLocation(m_postprocessShader, "uInvScreenSize");
    if (locInvScreen >= 0) {
        glUniform2f(locInvScreen,
                    1.0f / float(m_fboWidth),
                    1.0f / float(m_fboHeight));
    }

    // Watercolor effect
    GLint locWatercolor = glGetUniformLocation(m_postprocessShader, "watercolor");
    if (locWatercolor >= 0) {
        glUniform1i(locWatercolor, watercolorEnabled ? 1 : 0);
    }

    // Pixelated effect
    GLint locPixelated = glGetUniformLocation(m_postprocessShader, "pixelated");
    if (locPixelated >= 0) {
        glUniform1i(locPixelated, pixelatedEnabled ? 1 : 0);
    }

    // Pixel size uniform
    GLint locPixelSize = glGetUniformLocation(m_postprocessShader, "uPixelSize");
    if (locPixelSize >= 0) {
        glUniform1f(locPixelSize, pixelSize);
    }

    // Night time uniform
    GLint locNightTime = glGetUniformLocation(m_postprocessShader, "isNight");
    if (locNightTime >= 0) {
        glUniform1i(locNightTime, isNight ? 1 : 0);
    }

    glBindVertexArray(m_fullscreen_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Clean up texture bindings
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(0);
}

void PostProcess::cacheUniformLocations() {
    glUseProgram(m_postprocessShader);
    m_loc_tOrig         = glGetUniformLocation(m_postprocessShader, "tOrig");
    m_loc_tLUT          = glGetUniformLocation(m_postprocessShader, "tLUT");
    m_loc_gradeStr      = glGetUniformLocation(m_postprocessShader, "u_GradeStrength");
    m_loc_enableCG      = glGetUniformLocation(m_postprocessShader, "u_EnableColorGrading");
    glUseProgram(0);
}
