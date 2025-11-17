#include "shader.h"
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "utils/scenedata.h"
#include "utils/sceneparser.h"
#include "realtime.h"

Shader::Shader() {}

void Shader::handleLighting(GLuint &m_shader, RenderData &scene, GLight lights[8]){
    GLint light_loc_num = glGetUniformLocation(m_shader, "numLights");
    glUniform1i(light_loc_num, scene.lights.size());

    for (int i = 0; i < 8; i++){
        GLint light_loc_type = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "]" + ".type").c_str());
        glUniform1i(light_loc_type, lights[i].type);

        GLint light_loc_color = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "]" + ".color").c_str());
        glUniform3f(light_loc_color, lights[i].color.x, lights[i].color.y, lights[i].color.z);

        GLint light_loc_pos = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "]" + ".pos").c_str());
        glUniform3f(light_loc_pos, lights[i].pos.x, lights[i].pos.y, lights[i].pos.z);

        GLint light_loc_dir = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "]" + ".dir").c_str());
        glUniform3f(light_loc_dir, lights[i].dir.x, lights[i].dir.y, lights[i].dir.z);

        GLint light_loc_function = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "]" + ".function").c_str());
        glUniform3f(light_loc_function, lights[i].function.x, lights[i].function.y, lights[i].function.z);

        GLint light_loc_angle = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "]" + ".angle").c_str());
        glUniform1f(light_loc_angle, lights[i].angle);

        GLint light_loc_penumbra = glGetUniformLocation(m_shader, ("lights[" + std::to_string(i) + "]" + ".penumbra").c_str());
        glUniform1f(light_loc_penumbra, lights[i].penumbra);
        // GLint loc = glGetUniformLocation(shaderHandle, "myVectors[" + std::to_string(j) + "]");
        // glUniform3f(loc, x, y, z);
    }
}

void Shader::handleGlobals(GLuint &m_shader, RenderData &scene){
    auto loco_ambient = glGetUniformLocation(m_shader, "k_a");
    glUniform1f(loco_ambient, scene.globalData.ka);
    // Task 13: pass light position and m_kd into the fragment shader as a uniform
    auto loco_diffuse = glGetUniformLocation(m_shader, "k_d");
    glUniform1f(loco_diffuse, scene.globalData.kd);

    auto loco_specular = glGetUniformLocation(m_shader, "k_s");
    glUniform1f(loco_specular, scene.globalData.ks);
}

void Shader::handleShapes(GLuint &m_shader, Camera &m_camera, std::vector<std::unique_ptr<GLShape>> &myPrimitives, glm::mat4 &m_proj, GLData &m_sphere, GLData &m_cone,  GLData &m_cube, GLData &m_cylinder ){
    for (std::unique_ptr<GLShape> &shape: myPrimitives){


        GLuint vaoToBind;
        size_t vertexSize;
        switch (shape->primType) {
        case PrimitiveType::PRIMITIVE_SPHERE:   vaoToBind = m_sphere.vao;   vertexSize = m_sphere.vertex_data.size(); break;
        case PrimitiveType::PRIMITIVE_CUBE:     vaoToBind = m_cube.vao;     vertexSize = m_cube.vertex_data.size();break;
        case PrimitiveType::PRIMITIVE_CONE:     vaoToBind = m_cone.vao;     vertexSize = m_cone.vertex_data.size();break;
        case PrimitiveType::PRIMITIVE_CYLINDER: vaoToBind = m_cylinder.vao; vertexSize = m_cylinder.vertex_data.size();break;
        default: continue;
        }


        glBindVertexArray(vaoToBind);//only this will be active

        // Task 2: activate the shader program by calling glUseProgram with `m_shader`

        glm::vec3 mat_a = shape->material.cAmbient;
        GLint mat_ambient_loc = glGetUniformLocation(m_shader, ("matAmbient"));
        glUniform3f(mat_ambient_loc, mat_a.x, mat_a.y, mat_a.z);

        glm::vec3 mat_d = shape->material.cDiffuse;
        GLint mat_diffuse_loc = glGetUniformLocation(m_shader, ("matDiffuse"));
        glUniform3f(mat_diffuse_loc, mat_d.x, mat_d.y, mat_d.z);

        glm::vec3 mat_s = shape->material.cSpecular;
        GLint mat_specular_loc = glGetUniformLocation(m_shader, ("matSpecular"));
        glUniform3f(mat_specular_loc, mat_s.x, mat_s.y, mat_s.z);

        GLint mat_shiny_loc = glGetUniformLocation(m_shader, ("shininess"));
        glUniform1f(mat_shiny_loc, shape->material.shininess);

        // Task 6: pass in m_model as a uniform into the shader program
        auto loco = glGetUniformLocation(m_shader, "model_mat");
        glUniformMatrix4fv(loco, 1, GL_FALSE, &shape->model[0][0]);
        // In paintGL(), right after setting the model matrix uniform:
        glm::vec4 origin = shape->model * glm::vec4(0, 0, 0, 1);

        // Task 7: pass in m_view and m_proj
        auto loco_view = glGetUniformLocation(m_shader, "view_mat");
        glUniformMatrix4fv(loco_view, 1, GL_FALSE, &m_camera.getViewMatrix()[0][0]);

        auto loco_proj = glGetUniformLocation(m_shader, "proj_mat");
        glUniformMatrix4fv(loco_proj, 1, GL_FALSE, &m_proj[0][0]);

        glDrawArrays(GL_TRIANGLES, 0, vertexSize / 6);

    }

}
