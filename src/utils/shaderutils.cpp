#include "src/utils/shaderutils.h"
#include <glm/gtc/type_ptr.hpp>
#include <random>
#include <iostream>
#include "realtime.h"
#include "settings.h"
#include "shapes/ObjLoader.h"

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
                    const GLuint instanceVBOs[],
                    Realtime* realtime) {

        glUseProgram(m_shader);

        // Global material coefficients
        const SceneGlobalData &global = m_renderData.globalData;

        // Get uniform locations
        GLint k_a_loc = glGetUniformLocation(m_shader, "k_a");
        GLint k_d_loc = glGetUniformLocation(m_shader, "k_d");
        GLint k_s_loc = glGetUniformLocation(m_shader, "k_s");
        GLint shininess_loc = glGetUniformLocation(m_shader, "shininess");
        GLint useTextureMapLoc = glGetUniformLocation(m_shader, "useTextureMap");
        GLint textureRepeatULoc = glGetUniformLocation(m_shader, "textureRepeatU");
        GLint textureRepeatVLoc = glGetUniformLocation(m_shader, "textureRepeatV");
        GLint diffuseTextureLoc = glGetUniformLocation(m_shader, "DiffuseTextureSampler");

        // Separate shapes into textured and non-textured groups
        std::vector<const RenderShapeData*> texturedShapes;
        std::vector<mat4> instanceModels[Realtime::PRIM_COUNT];
        SceneMaterial batchMaterial[Realtime::PRIM_COUNT];
        bool hasMaterial[Realtime::PRIM_COUNT] = {false, false, false, false};

        // Store mesh instances separately (keyed by filepath)
        std::unordered_map<std::string, std::vector<mat4>> meshInstances;
        std::unordered_map<std::string, SceneMaterial> meshMaterials;
        std::unordered_map<std::string, bool> meshUseScrollingTex;
        
        for (const RenderShapeData &shapeData : m_renderData.shapes) {
            const ScenePrimitive &prim = shapeData.primitive;

            // Handle mesh primitives separately
            if (prim.type == PrimitiveType::PRIMITIVE_MESH) {
                if (realtime != nullptr && !prim.meshfile.empty()) {
                    // Load mesh if not already loaded
                    if (!realtime->hasMesh(prim.meshfile)) {
                        std::cout << "[ShaderUtils] Loading mesh: " << prim.meshfile << std::endl;
                        const_cast<Realtime*>(realtime)->loadMesh(prim.meshfile);
                    }
                    
                    if (realtime->hasMesh(prim.meshfile)) {
                        meshInstances[prim.meshfile].push_back(shapeData.ctm);
                        if (meshMaterials.find(prim.meshfile) == meshMaterials.end()) {
                            meshMaterials[prim.meshfile] = prim.material;
                        }

                        meshUseScrollingTex[prim.meshfile] = prim.useScrollingTex;
                    }
                }
                continue;
            }

            // Check if shape has textures (normal map or diffuse texture with normal mapping enabled)
            bool hasTexture = prim.material.bumpMap.isUsed || prim.material.textureMap.isUsed;

            if (hasTexture && realtime != nullptr) {
                // Shapes with textures drawn individually
                texturedShapes.push_back(&shapeData);
            } else {
                // Shapes without textures - batch for instanced rendering
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

                instanceModels[idx].push_back(shapeData.ctm);
            }
        }

        // Set default texture state (no textures)
        glUniform1i(useTextureMapLoc, 0);
        glUniform1f(textureRepeatULoc, 1.0f);
        glUniform1f(textureRepeatVLoc, 1.0f);

        // 1) Draw non-textured shapes using instanced rendering
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

        // 2) Draw textured shapes individually (non-instanced)
        for (const RenderShapeData* shapePtr : texturedShapes) {
            const RenderShapeData &shapeData = *shapePtr;
            const ScenePrimitive &prim = shapeData.primitive;
            const SceneMaterial &mat = prim.material;

            // Get VAO and vertex count for this primitive type
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

            // Set material uniforms
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

            // Handle diffuse texture
            bool hasValidDiffuseTexture = false;
            if (mat.textureMap.isUsed && realtime != nullptr) {
                GLuint diffuseTexture = realtime->loadTexture(mat.textureMap.filename);
                if (diffuseTexture != 0) {
                    hasValidDiffuseTexture = true;
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, diffuseTexture);
                    glUniform1i(diffuseTextureLoc, 0);
                    glUniform1f(textureRepeatULoc, mat.textureMap.repeatU);
                    glUniform1f(textureRepeatVLoc, mat.textureMap.repeatV);
                }
            }
            glUniform1i(useTextureMapLoc, hasValidDiffuseTexture ? 1 : 0);
            if (!hasValidDiffuseTexture) {
                glUniform1f(textureRepeatULoc, 1.0f);
                glUniform1f(textureRepeatVLoc, 1.0f);
            }

            // Normal mapping disabled
            // Upload single instance model matrix
            glBindVertexArray(vaos[idx]);
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBOs[idx]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4), glm::value_ptr(shapeData.ctm), GL_DYNAMIC_DRAW);

            // Draw single instance
            glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCounts[idx], 1);
            glBindVertexArray(0);

            // Unbind textures
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        // 3) Draw mesh primitives - render each group with its own material
        if (realtime != nullptr) {
            for (const auto& meshPair : meshInstances) {
                const std::string& meshPath = meshPair.first;
                const std::vector<mat4>& models = meshPair.second;
                
                if (models.empty() || !realtime->hasMesh(meshPath)) {
                    continue;
                }
                
                GLuint meshVAO = realtime->getMeshVAO(meshPath);
                GLuint meshInstanceVBO = realtime->getMeshInstanceVBO(meshPath);
                
                if (meshVAO == 0) {
                    continue;
                }

                bool useScrollingTex = false;
                auto scrollIt = meshUseScrollingTex.find(meshPath);
                if (scrollIt != meshUseScrollingTex.end()) {
                    useScrollingTex = scrollIt->second;
                }

                GLuint useScrollingTexLoc = glGetUniformLocation(m_shader, "u_useScrollingTex");
                if (useScrollingTexLoc >= 0) glUniform1i(useScrollingTexLoc, useScrollingTex ? 1 : 0);

                GLuint diffuseTextureLoc = glGetUniformLocation(m_shader, "DiffuseTexutreSampler");
                
                // Get loader and group info for per-group rendering
                const ObjLoader* loader = realtime->getMeshLoader(meshPath);
                const auto* groupInfos = realtime->getMeshGroupInfos(meshPath);
                
                if (loader == nullptr || groupInfos == nullptr || groupInfos->empty()) {
                    // Fallback: render entire mesh with default material
                    int meshVertCount = realtime->getMeshVertexCount(meshPath);
                    if (meshVertCount == 0) continue;
                    
                    glm::vec3 k_a = global.ka * glm::vec3(0.3f);
                    glm::vec3 k_d = global.kd * glm::vec3(0.7f);
                    glm::vec3 k_s = global.ks * glm::vec3(0.3f);
                    
                    glUniform3fv(k_a_loc, 1, glm::value_ptr(k_a));
                    glUniform3fv(k_d_loc, 1, glm::value_ptr(k_d));
                    glUniform3fv(k_s_loc, 1, glm::value_ptr(k_s));
                    glUniform1f(shininess_loc, 32.f);
                    glUniform1i(useTextureMapLoc, 0);
                    
                    glBindVertexArray(meshVAO);
                    glBindBuffer(GL_ARRAY_BUFFER, meshInstanceVBO);
                    glBufferData(GL_ARRAY_BUFFER, models.size() * sizeof(glm::mat4), models.data(), GL_DYNAMIC_DRAW);
                    glDrawArraysInstanced(GL_TRIANGLES, 0, meshVertCount, static_cast<GLsizei>(models.size()));
                    glBindVertexArray(0);
                    continue;
                }
                
                const auto& materials = loader->getMaterials();
                
                // Bind VAO once for all groups
                glBindVertexArray(meshVAO);
                glBindBuffer(GL_ARRAY_BUFFER, meshInstanceVBO);
                glBufferData(GL_ARRAY_BUFFER, models.size() * sizeof(glm::mat4), models.data(), GL_DYNAMIC_DRAW);
                
                // Render each group with its own material
                for (const auto& groupInfo : *groupInfos) {
                    if (groupInfo.vertexCount == 0) continue;
                    
                    // Find material for this group
                    ObjMaterial objMat;
                    auto matIt = materials.find(groupInfo.materialName);
                    if (matIt != materials.end()) {
                        objMat = matIt->second;
                    } else {
                        // Default material if not found
                        objMat.ambient = glm::vec3(0.3f);
                        objMat.diffuse = glm::vec3(0.7f);
                        objMat.specular = glm::vec3(0.3f);
                        objMat.shininess = 32.0f;
                    }
                    
                    // Check if this material has a diffuse texture
                    bool hasValidDiffuseTexture = false;
                    GLuint diffuseTexture = 0;
                    if (!objMat.diffuseTexture.empty()) {
                        diffuseTexture = realtime->loadTexture(objMat.diffuseTexture);
                        hasValidDiffuseTexture = (diffuseTexture != 0);
                    }
                    
                    // Normal mapping disabled
                    bool hasValidNormalTexture = false;
                    
                    // When texture is used, use lower ambient to avoid washing out
                    glm::vec3 k_a, k_d, k_s;
                    if (hasValidDiffuseTexture) {
                        // Use texture for color, minimal ambient
                        k_a = glm::vec3(0.05f);  // Very low ambient so texture shows properly
                        k_d = glm::vec3(1.0f);   // Full diffuse - texture provides the color
                        k_s = global.ks * objMat.specular * 0.3f;  // Reduced specular
                    } else {
                        k_a = global.ka * objMat.ambient;
                        k_d = global.kd * objMat.diffuse;
                        k_s = global.ks * objMat.specular;
                    }
                    
                    float shininess = objMat.shininess;
                    if (shininess <= 0.f) shininess = 32.f;
                    
                    glUniform3fv(k_a_loc, 1, glm::value_ptr(k_a));
                    glUniform3fv(k_d_loc, 1, glm::value_ptr(k_d));
                    glUniform3fv(k_s_loc, 1, glm::value_ptr(k_s));
                    glUniform1f(shininess_loc, shininess);
                    
                    // Handle diffuse texture from material
                    if (hasValidDiffuseTexture) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, diffuseTexture);
                        glUniform1i(diffuseTextureLoc, 0);
                        glUniform1f(textureRepeatULoc, 1.0f);
                        glUniform1f(textureRepeatVLoc, 1.0f);
                    }
                    glUniform1i(useTextureMapLoc, hasValidDiffuseTexture ? 1 : 0);
                    
                    // Check if this is a water material for scrolling texture
                    // Check both material name and texture path for "water"
                    bool isWaterMaterial = (groupInfo.materialName.find("water") != std::string::npos ||
                                           groupInfo.materialName.find("Water") != std::string::npos ||
                                           objMat.diffuseTexture.find("water") != std::string::npos ||
                                           objMat.diffuseTexture.find("Water") != std::string::npos);
                    GLint useScrollingTexLoc = glGetUniformLocation(m_shader, "u_useScrollingTex");
                    if (isWaterMaterial && useScrollingTexLoc >= 0) {
                        glUniform1i(useScrollingTexLoc, 1);
                    }
                    
                    // Draw this group's vertices
                    glDrawArraysInstanced(GL_TRIANGLES, groupInfo.startVertex, groupInfo.vertexCount, 
                                          static_cast<GLsizei>(models.size()));
                    
                    // Unbind textures after each group
                    if (hasValidDiffuseTexture) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }

                    // Reset water scrolling state
                    if (isWaterMaterial && useScrollingTexLoc >= 0) {
                        glUniform1i(useScrollingTexLoc, 0);
                    }
                }
                
                glBindVertexArray(0);
            }
        }

        // Reset texture state
        glUniform1i(useTextureMapLoc, 0);
    }
}
