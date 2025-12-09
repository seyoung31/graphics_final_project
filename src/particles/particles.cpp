#include "particles.h"
#include <QImage>
#include <QDebug>
#include <algorithm>
#include <cstdlib>
// --- ctor/dtor
Particles::Particles() {}
Particles::~Particles() { cleanup(); }

// --- helper: find unused particle quickly (tutorial method)
int Particles::FindUnusedParticle() {
    for (int i = LastUsedParticle; i < MaxParticles; ++i) {
        if (m_particlesContainer[i].life < 0.0f) {
            LastUsedParticle = i;
            return i;
        }
    }
    for (int i = 0; i < LastUsedParticle; ++i) {
        if (m_particlesContainer[i].life < 0.0f) {
            LastUsedParticle = i;
            return i;
        }
    }
    LastUsedParticle = 0;
    return 0;
}

// --- spawn new particles inside a fixed axis-aligned box centered at m_spawnCenter
void Particles::spawnNewParticles(int newCount) {
    for (int i = 0; i < newCount; ++i) {
        int idx = FindUnusedParticle();
        Particle &p = m_particlesContainer[idx];

        // Life
        p.life = 4.0f + (static_cast<float>(rand())/RAND_MAX) * 6.0f; // 4-10s

        // Sample position uniformly inside AABB defined by spawn center +/- extents
        float rx = (static_cast<float>(rand())/RAND_MAX * 2.0f - 1.0f) * m_spawnExtents.x;
        float ry = (static_cast<float>(rand())/RAND_MAX * 2.0f - 1.0f) * m_spawnExtents.y;
        float rz = (static_cast<float>(rand())/RAND_MAX * 2.0f - 1.0f) * m_spawnExtents.z;
        p.pos = m_spawnCenter + glm::vec3(rx, ry, rz);

        // velocity: mostly downward in world-space Y, with small drift
        float downSpeed = 0.15f + (static_cast<float>(rand())/RAND_MAX) * 0.35f;
        float horizX = (static_cast<float>(rand())/RAND_MAX - 0.5f) * 0.3f; // sideways drift X
        float horizZ = (static_cast<float>(rand())/RAND_MAX - 0.5f) * 0.3f; // sideways drift Z

        p.speed = glm::vec3(horizX, -downSpeed, horizZ);

        // visual attributes
        unsigned char alpha = static_cast<unsigned char>(200 + (rand() % 56));
        p.r = 255; p.g = 255; p.b = 255; p.a = alpha;
        p.size = 0.15f + (static_cast<float>(rand())/RAND_MAX) * 0.06f;

        // we will compute cameradistance later in render() so leave it for now
        p.cameradistance = -1.0f;
    }
}


// --- init: create CPU containers, VBOs, VAO, and load texture (unchanged)
void Particles::init(GLuint shaderProgram) {
    m_shader = shaderProgram;

    // CPU containers
    m_particlesContainer.resize(MaxParticles);
    for (int i = 0; i < MaxParticles; ++i) m_particlesContainer[i].life = -1.0f;
    g_particule_position_size_data.assign(MaxParticles * 4, 0.0f);
    g_particule_color_data.assign(MaxParticles * 4, 0);


    // create instanced buffers & VAO
    initInstancedBuffers();

    // load texture for snow
    loadTextureFromResource(":/resources/images/snowflake_only.png");

    // set sampler uniform to texture unit 0 if shader exists
    if (m_shader) {
        glUseProgram(m_shader);
        GLint loc = glGetUniformLocation(m_shader, "uParticleTex");
        if (loc >= 0) glUniform1i(loc, 0);
        glUseProgram(0);
    }

}

void Particles::initInstancedBuffers() {
    // billboard base quad (4 verts - triangle strip)
    static const GLfloat g_vertex_buffer_data[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,
        0.5f,  0.5f, 0.0f
    };

    glGenBuffers(1, &m_billboard_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_billboard_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    // streaming instance buffers
    glGenBuffers(1, &m_positions_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_positions_vbo);
    glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

    glGenBuffers(1, &m_colors_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_colors_vbo);
    glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);

    // create VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // attrib 0: base quad vertex positions
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_billboard_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // attrib 1: per-instance vec4 (x,y,z,size)
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, m_positions_vbo);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);

    // attrib 2: per-instance color normalized
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, m_colors_vbo);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4 * sizeof(GLubyte), (void*)0);

    // set divisors: 0 (same per vertex) 1 (advances per instance)
    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);

    // cleanup
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

}

// --- texture loader (unchanged)
void Particles::loadTextureFromResource(const char* resourcePath) {
    QImage img(resourcePath);
    if (img.isNull()) {
        std::cerr << "Particles::loadTextureFromResource: failed to open " << resourcePath << std::endl;
        return;
    }
    img = img.convertToFormat(QImage::Format_RGBA8888).mirrored();

    glGenTextures(1, &m_tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

}

// --- update: spawn & simulate, no camera position required
void Particles::update(float dt) {
    if (dt <= 0.0f) return;

    // spawn rate (flakes per second) can be tweaked
    int newparticles = static_cast<int>(dt * 800.0f); // 800 flakes/sec default
    if (newparticles > 200) newparticles = 200;
    spawnNewParticles(newparticles);

    // small time-based oscillation frequency for drifting motion
    const float driftFreq = 1.0f;
    static float timeAccum = 0.0f;
    timeAccum += dt;
    ParticlesCount = 0;

    // update physics for all particles
    for (int i = 0; i < MaxParticles; ++i) {
        Particle &p = m_particlesContainer[i];
        if (p.life > 0.0f) {
            p.life -= dt;
            if (p.life > 0.0f) {
                // physics (gravity + some sway)
                p.speed.y += snow_gravity * dt;
                float swayX = 0.25f * sinf(timeAccum * driftFreq + float(i) * 0.13f);
                float swayZ = 0.25f * cosf(timeAccum * driftFreq + float(i) * 0.11f);
                p.pos.x += (p.speed.x + snow_wind.x * 0.15f + swayX) * dt;
                p.pos.y += p.speed.y * dt;
                p.pos.z += (p.speed.z + snow_wind.z * 0.15f + swayZ) * dt;
                // leave cameradistance calculation for render()
            } else {
                // mark dead for reuse
                p.life = -1.0f;
                p.cameradistance = -1.0f;
            }
        }
    }

    // don't sort or upload here — render will compute distances, sort, and upload
}

// --- render: compute camera distances, sort, fill GPU arrays and draw
void Particles::render(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 &cameraPos) {
    if (m_shader == 0u) return;

    // update last camera pos
    m_lastCameraPos = cameraPos;

    // compute camera distances for alive particles (squared)
    for (int i = 0; i < MaxParticles; ++i) {
        Particle &p = m_particlesContainer[i];
        if (p.life > 0.0f) {
            p.cameradistance = glm::length2(p.pos - cameraPos);
        } else {
            p.cameradistance = -1.0f;
        }
    }

    // --- sort AFTER computing distances so ordering is correct for this frame ---
    std::stable_sort(m_particlesContainer.begin(), m_particlesContainer.end());

    // --- fill the GPU-side arrays using sorted list (far -> near) ---
    ParticlesCount = 0;
    for (int i = 0; i < MaxParticles; ++i) {
        Particle &p = m_particlesContainer[i];
        if (p.life > 0.0f) {
            g_particule_position_size_data[4 * ParticlesCount + 0] = p.pos.x;
            g_particule_position_size_data[4 * ParticlesCount + 1] = p.pos.y;
            g_particule_position_size_data[4 * ParticlesCount + 2] = p.pos.z;
            g_particule_position_size_data[4 * ParticlesCount + 3] = p.size;

            g_particule_color_data[4 * ParticlesCount + 0] = p.r;
            g_particule_color_data[4 * ParticlesCount + 1] = p.g;
            g_particule_color_data[4 * ParticlesCount + 2] = p.b;
            g_particule_color_data[4 * ParticlesCount + 3] = p.a;

            ++ParticlesCount;
        }
    }

    if (ParticlesCount == 0) return;
    if (m_shader == 0u) return;

    glUseProgram(m_shader);

    // set view/proj
    GLint locView = glGetUniformLocation(m_shader, "uView");
    GLint locProj = glGetUniformLocation(m_shader, "uProj");
    if (locView >= 0) glUniformMatrix4fv(locView, 1, GL_FALSE, &view[0][0]);
    if (locProj >= 0) glUniformMatrix4fv(locProj, 1, GL_FALSE, &proj[0][0]);

    // camera basis for billboards (assumes view matrix is orthonormal)
    GLint locRight = glGetUniformLocation(m_shader, "uCameraRight");
    GLint locUp = glGetUniformLocation(m_shader, "uCameraUp");
    glm::vec3 camRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
    glm::vec3 camUp    = glm::vec3(view[0][1], view[1][1], view[2][1]);
    if (locRight >= 0) glUniform3f(locRight, camRight.x, camRight.y, camRight.z);
    if (locUp >= 0) glUniform3f(locUp, camUp.x, camUp.y, camUp.z);

    // ensure tint/cutoff safe defaults (shader expects them)
    GLint locTint = glGetUniformLocation(m_shader, "uTint");
    if (locTint >= 0) glUniform4f(locTint, 1.0f, 1.0f, 1.0f, 1.0f);
    GLint locAlphaCut = glGetUniformLocation(m_shader, "uAlphaCutoff");
    if (locAlphaCut >= 0) glUniform1f(locAlphaCut, 0.1f);

    // bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex);

    // --- upload streaming buffers (orphan + subdata) ---
    glBindBuffer(GL_ARRAY_BUFFER, m_positions_vbo);
    glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    if (ParticlesCount > 0)
        glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * 4 * sizeof(GLfloat), g_particule_position_size_data.data());

    glBindBuffer(GL_ARRAY_BUFFER, m_colors_vbo);
    glBufferData(GL_ARRAY_BUFFER, MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);
    if (ParticlesCount > 0)
        glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * 4 * sizeof(GLubyte), g_particule_color_data.data());

    // render state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // draw instanced triangle strip (4 verts)
    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

// --- cleanup (unchanged)
void Particles::cleanup() {
    if (m_billboard_vbo) { glDeleteBuffers(1, &m_billboard_vbo); m_billboard_vbo = 0; }
    if (m_positions_vbo) { glDeleteBuffers(1, &m_positions_vbo); m_positions_vbo = 0; }
    if (m_colors_vbo) { glDeleteBuffers(1, &m_colors_vbo); m_colors_vbo = 0; }
    if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_tex) { glDeleteTextures(1, &m_tex); m_tex = 0; }
}
