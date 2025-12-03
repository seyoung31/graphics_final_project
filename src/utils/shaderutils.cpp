#include "src/utils/shaderutils.h"
#include <glm/gtc/type_ptr.hpp>
#include <random>
#include "realtime.h"

using namespace glm;

static const int MAX_LIGHTS = 8;

// CPU-side arrays matching the GLSL uniforms
int lightTypes[MAX_LIGHTS];
glm::vec3 lightColors[MAX_LIGHTS];
glm::vec3 lightPositions[MAX_LIGHTS];
glm::vec3 lightDirections[MAX_LIGHTS];
glm::vec3 lightFuncs[MAX_LIGHTS];
float lightAngles[MAX_LIGHTS];
float lightPenumbras[MAX_LIGHTS];

namespace ShaderUtils {

    void uploadCamera(GLuint m_shader, const glm::mat4 &m_view, const glm::mat4 &m_proj) {

        glUseProgram(m_shader);

        GLuint viewLoc = glGetUniformLocation(m_shader, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_view));

        GLuint projLoc = glGetUniformLocation(m_shader, "proj");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_proj));

        // Camera position in world space
        glm::vec3 camPos3 = glm::vec3(glm::inverse(m_view)[3]);
        glm::vec4 camPos4(camPos3, 1.0f);
        glUniform4fv(glGetUniformLocation(m_shader, "camPos"),
                     1, glm::value_ptr(camPos4));
    }

    void uploadGlobals(GLuint m_shader, const RenderData &m_renderData) {
        glUseProgram(m_shader);
        glUniform1f(glGetUniformLocation(m_shader, "k_a"), m_renderData.globalData.ka);
        glUniform1f(glGetUniformLocation(m_shader, "k_d"), m_renderData.globalData.kd);
        glUniform1f(glGetUniformLocation(m_shader, "k_s"), m_renderData.globalData.ks);
    };

    void uploadLights(GLuint m_shader, const RenderData &m_renderData) {

        glUseProgram(m_shader);

        int numLights = std::min(int(m_renderData.lights.size()), MAX_LIGHTS);

        for (int i=0; i < numLights; ++i) {
            const SceneLightData &L = m_renderData.lights[i];

            lightTypes[i] = (int)(L.type);
            lightColors[i] = glm::vec3(L.color);
            lightFuncs[i] = L.function;

            glm::vec3 dir = glm::normalize(glm::vec3(L.dir));
            lightDirections[i] = dir;

            if (L.type == LightType::LIGHT_DIRECTIONAL) {
                lightPositions[i] = glm::vec3(0.0f); // unused
            } else {
                lightPositions[i] = glm::vec3(L.pos);
            }

            lightAngles[i] = L.angle;
            lightPenumbras[i] = L.penumbra;
        };

        glUniform1i(glGetUniformLocation(m_shader, "numLights"), numLights);

        // Send arrays
        if (numLights > 0) {
            glUniform1iv(glGetUniformLocation(m_shader, "lightType"),
                         numLights, lightTypes);

            glUniform3fv(glGetUniformLocation(m_shader, "lightColor"),
                         numLights, glm::value_ptr(lightColors[0]));

            glUniform3fv(glGetUniformLocation(m_shader, "lightPos"),
                         numLights, glm::value_ptr(lightPositions[0]));

            glUniform3fv(glGetUniformLocation(m_shader, "lightDir"),
                         numLights, glm::value_ptr(lightDirections[0]));

            glUniform3fv(glGetUniformLocation(m_shader, "lightFunc"),
                         numLights, glm::value_ptr(lightFuncs[0]));

            glUniform1fv(glGetUniformLocation(m_shader, "lightAngle"),
                         numLights, lightAngles);

            glUniform1fv(glGetUniformLocation(m_shader, "lightPenumbra"),
                         numLights, lightPenumbras);
        }
    };

    void drawShapes(GLuint m_shader,
                    const RenderData &m_renderData,
                    const GLuint vaos[],
                    const int vertexCounts[],
                    const GLuint instanceVBOs[]) {

        glUseProgram(m_shader);

        // Global material coefficients
        const SceneGlobalData &global = m_renderData.globalData;

        // 1) Group shapes by primitive type and remember a representative material
        static bool initialized = false;
        static int lastShapeCount = -1;
        static std::vector<mat4> instanceModels[Realtime::PRIM_COUNT];
        static SceneMaterial batchMaterial[Realtime::PRIM_COUNT];
        static bool hasMaterial[Realtime::PRIM_COUNT] = {false, false, false, false};

        if (!initialized || int(m_renderData.shapes.size()) != lastShapeCount) {
            for (int i=0; i < Realtime::PRIM_COUNT; ++i) {
                instanceModels[i].clear();
                hasMaterial[i] = false;
            }

            // Randomization
            std::mt19937 rng{std::random_device{}()};
            std::uniform_real_distribution<float> posJitter(-0.5f, 0.5f);
            std::uniform_real_distribution<float> scaleJitter(0.8f, 1.2f);

            for (const RenderShapeData &shapeData : m_renderData.shapes) {
                const ScenePrimitive &prim = shapeData.primitive;

                // Choose VAO based on primitive type
                Realtime::PrimitiveIndex idx;

                switch (prim.type) {
                case PrimitiveType::PRIMITIVE_SPHERE:
                    idx = Realtime::PRIM_SPHERE;
                    break;
                case PrimitiveType::PRIMITIVE_CUBE:
                    idx = Realtime::PRIM_CUBE;
                    break;
                case PrimitiveType::PRIMITIVE_CYLINDER:
                    idx = Realtime::PRIM_CYLINDER;
                    break;
                case PrimitiveType::PRIMITIVE_CONE:
                    idx = Realtime::PRIM_CONE;
                    break;
                default:
                    continue;
                }

                if (!hasMaterial[idx]) {
                    batchMaterial[idx] = prim.material;
                    hasMaterial[idx] = true;
                }

                mat4 baseModel = shapeData.ctm;

                // One-time random jitter in position and uniform scale
                float jx = posJitter(rng);
                float jy = posJitter(rng);
                float jz = posJitter(rng);
                float s  = scaleJitter(rng);

                mat4 randomTransform(1.0f);
                randomTransform = translate(randomTransform, vec3(jx, jy, jz));
                randomTransform = scale(randomTransform, vec3(s));

                // Apply jitter on top of the existing placement
                mat4 finalModel = baseModel * randomTransform;

                instanceModels[idx].push_back(finalModel);
            }

            initialized = true;
            lastShapeCount = int(m_renderData.shapes.size());

        }

        // 2) Upload per-instance data and issue instanced draws
        GLint k_a_loc = glGetUniformLocation(m_shader, "k_a");
        GLint k_d_loc = glGetUniformLocation(m_shader, "k_d");
        GLint k_s_loc = glGetUniformLocation(m_shader, "k_s");
        GLint shininess_loc = glGetUniformLocation(m_shader, "shininess");

        for (int idx=0; idx < Realtime::PRIM_COUNT; ++idx) {
            const auto &models = instanceModels[idx];
            if (models.empty() || !hasMaterial[idx]) {
                continue;
            }

            const SceneMaterial &mat = batchMaterial[idx];

            glm::vec3 k_a = global.ka * glm::vec3(mat.cAmbient);
            glm::vec3 k_d = global.kd * glm::vec3(mat.cDiffuse);
            glm::vec3 k_s = global.ks * glm::vec3(mat.cSpecular);

            float shininess = mat.shininess;
            if (shininess <= 0.f) {
                shininess = 1.f;
            }

            glUniform3fv(k_a_loc, 1, glm::value_ptr(k_a));
            glUniform3fv(k_d_loc, 1, glm::value_ptr(k_d));
            glUniform3fv(k_s_loc, 1, glm::value_ptr(k_s));
            glUniform1f(shininess_loc, shininess);

            // Upload all instance model matrices into the instance VBO
            glBindVertexArray(vaos[idx]);
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBOs[idx]);
            glBufferData(GL_ARRAY_BUFFER, models.size() * sizeof(glm::mat4), models.data(), GL_DYNAMIC_DRAW);

            // One instanced draw call per primitive type
            glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCounts[idx], static_cast<GLsizei>(models.size()));
            glBindVertexArray(0);
        }
    }
}
