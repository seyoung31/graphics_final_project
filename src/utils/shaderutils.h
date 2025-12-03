#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include "utils/scenedata.h"
#include "realtime.h"
#include <vector>

namespace ShaderUtils {

    void uploadCamera(GLuint m_shader, const glm::mat4 &m_view, const glm::mat4 &m_proj);

    void uploadGlobals(GLuint m_shader, const RenderData &m_renderData);

    void uploadLights(GLuint m_shader, const RenderData &m_renderData);

    void drawShapes(GLuint m_shader,
                    const RenderData &m_renderData,
                    const GLuint vaos[],
                    const int vertexCounts[],
                    const GLuint instanceVBOs[]);
}


