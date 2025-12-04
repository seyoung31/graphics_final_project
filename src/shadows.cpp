// #include "shadows.h"
// #include "glm/glm.hpp"
// #include <glm/gtc/matrix_transform.hpp>

// #include "utils/sceneparser.h"
// #include "settings.h"
// #include "realtime.h"

// glm::mat4 shadows::getLightVP(const SceneLightData& light) {

//     if (light.type == LightType::LIGHT_DIRECTIONAL) {
//         float backDist = 5.f;   // have to put the light somewhere, as dir doesnt have a position
//         float B = 5.f;          // orthogonal vals so we can use half-width/height

//         glm::vec3 dir = glm::normalize(glm::vec3(light.dir));

//         //look at, f
//         glm::vec3 target = glm::vec3(0.f);

//         glm::vec3 pos = target - dir * backDist;

//         // Up choice with safety fallback:
//         glm::vec3 up = glm::vec3(0,1,0);
//         if (std::abs(glm::dot(up, dir)) > 0.95f) {
//             up = glm::vec3(1,0,0); // avoid near-parallel up
//         }

//         glm::mat4 V = glm::lookAt(pos, target, up); //view mat

//         float nearL = settings.nearPlane;
//         float farL  = settings.farPlane;

//         glm::mat4 P = glm::ortho(-B, B, -B, B, nearL, farL); //proj mat

//         return P * V;
//     }

//     if (light.type == LightType::LIGHT_SPOT) {
//         glm::vec3 pos = glm::vec3(light.pos);
//         glm::vec3 dir = glm::normalize(glm::vec3(light.dir));
//         glm::vec3 target = pos + dir;

//         glm::vec3 up = glm::vec3(0,1,0);
//         if (std::abs(glm::dot(up, dir)) > 0.95f) {
//             up = glm::vec3(1,0,0);
//         }

//         glm::mat4 V = glm::lookAt(pos, target, up); //view mat

//         float fov = 2.f * light.angle; // angle already radians
//         float aspect = 1.f;            // shadow map is square
//         float nearL = settings.nearPlane;
//         float farL  = settings.farPlane;

//         glm::mat4 P = glm::perspective(fov, aspect, nearL, farL); //proj mat

//         return P * V;
//     }

//     if (light.type == LightType::LIGHT_POINT) {
//         return glm::mat4(1.f); //dont care about this case SOZ!
//     }

//     return glm::mat4(1.f);
// }

// void shadows::makeShadowFBO(){
//     //purge old stuff
//     for (int i = 0; i < 8; i++) {
//         if (m_shadow_depth_texs[i] != 0) glDeleteTextures(1, &m_shadow_depth_texs[i]);
//         if (m_shadow_fbos[i] != 0) glDeleteFramebuffers(1, &m_shadow_fbos[i]);
//         m_shadow_depth_texs[i] = 0;
//         m_shadow_fbos[i] = 0;
//     }

//     m_shadow_res = 2048;

//     for (int i = 0; i < 8; i++) {

//         //DEPTH TEX for shadows
//         glGenTextures(1, &m_shadow_depth_texs[i]); //
//         glBindTexture(GL_TEXTURE_2D, m_shadow_depth_texs[i]);
//         glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_shadow_res, m_shadow_res, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//         glBindTexture(GL_TEXTURE_2D, 0);

//         // Shadow FBO
//         glGenFramebuffers(1, &m_shadow_fbos[i]);
//         glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_fbos[i]);
//         glDrawBuffer(GL_NONE);
//         glReadBuffer(GL_NONE);

//         glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
//                                GL_TEXTURE_2D, m_shadow_depth_texs[i], 0);

//         // GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
//         // if (status != GL_FRAMEBUFFER_COMPLETE) {
//         //     std::cerr << "Shadow FBO " << i << " incomplete, status = " << status << std::endl;
//         // }
//     }

//     glBindFramebuffer(GL_FRAMEBUFFER, m_defaultFBO);
// }

// void shadows::paintLightView(SceneLightData light, const glm::mat4& lightVP){
//     //we render from the lights view not the cameras view

//     glClear(GL_DEPTH_BUFFER_BIT);
//     glUseProgram(m_shadow_shader);

//     GLint vpLoc = glGetUniformLocation(m_shadow_shader, "light_view_proj");
//     glUniformMatrix4fv(vpLoc, 1, GL_FALSE, &lightVP[0][0]);

//     //shape rendering
//     GLuint cur_vao;
//     std::vector<float> cur_data;

//     for (RenderShapeData shape : m_renderData.shapes){
//         if (shape.primitive.type == PrimitiveType::PRIMITIVE_SPHERE){
//             // std::cout << "painting sphere!" << std::endl;
//             cur_vao = m_sphere_vao;
//             cur_data = m_sphereData;
//         } else if (shape.primitive.type == PrimitiveType::PRIMITIVE_CUBE){
//             // std::cout << "painting cube!" << std::endl;
//             cur_vao = m_cube_vao;
//             cur_data = m_cubeData;
//         } else if (shape.primitive.type == PrimitiveType::PRIMITIVE_CONE){
//             // std::cout << "painting cone!" << std::endl;
//             cur_vao = m_cone_vao;
//             cur_data = m_coneData;
//         } else if (shape.primitive.type == PrimitiveType::PRIMITIVE_CYLINDER){
//             // std::cout << "painting cylinder!" << std::endl;
//             cur_vao = m_cylinder_vao;
//             cur_data = m_cylinderData;
//         }

//         glBindVertexArray(cur_vao); //should be looping and checking shape type


//         //need to pass in uniforms here
//         GLint model_matrix_loc = __glewGetUniformLocation(m_shadow_shader, "model_matrix");

//         //send in all matrix data
//         glUniformMatrix4fv(model_matrix_loc, 1, GL_FALSE, &shape.ctm[0][0]);

//         glEnable(GL_DEPTH_TEST);
//         glDepthMask(GL_TRUE);
//         glDepthFunc(GL_LESS);

//         glDrawArrays(GL_TRIANGLES, 0, cur_data.size()/6);


//     }

//     glBindVertexArray(0);
//     glUseProgram(0);
// }

