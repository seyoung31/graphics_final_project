#include "Shape.h"
#include <cmath>

void Shape::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}

void Shape::computeTangents() {
    // Compute tangents per triangle from UV coordinates
    for (size_t i = 0; i < m_vertices.size(); i += 3) {
        if (i + 2 >= m_vertices.size()) break;
        
        Vertex& v0 = m_vertices[i];
        Vertex& v1 = m_vertices[i + 1];
        Vertex& v2 = m_vertices[i + 2];
        
        // Calculate edges
        glm::vec3 deltaPos1 = v1.position - v0.position;
        glm::vec3 deltaPos2 = v2.position - v0.position;
        
        // Calculate UV deltas
        glm::vec2 deltaUV1 = v1.uv - v0.uv;
        glm::vec2 deltaUV2 = v2.uv - v0.uv;
        
        // Calculate tangent and bitangent
        float denom = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
        
        glm::vec3 tangent(1.0f, 0.0f, 0.0f);
        glm::vec3 bitangent(0.0f, 1.0f, 0.0f);
        
        if (std::abs(denom) > 1e-6f) {
            float r = 1.0f / denom;
            tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
            bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
        }
        
        // Assign to all three vertices of the triangle
        v0.tangent = tangent;
        v0.bitangent = bitangent;
        v1.tangent = tangent;
        v1.bitangent = bitangent;
        v2.tangent = tangent;
        v2.bitangent = bitangent;
    }
    
    // Normalize and ensure orthogonality using Gram-Schmidt
    for (Vertex& v : m_vertices) {
        v.normal = glm::normalize(v.normal);
        
        if (glm::length(v.tangent) > 1e-6f) {
            v.tangent = glm::normalize(v.tangent);
        } else {
            // Fallback: create arbitrary tangent perpendicular to normal
            glm::vec3 up = glm::abs(v.normal.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
            v.tangent = glm::normalize(glm::cross(up, v.normal));
        }
        
        // Gram-Schmidt orthogonalization
        v.tangent = glm::normalize(v.tangent - glm::dot(v.tangent, v.normal) * v.normal);
        v.bitangent = glm::normalize(glm::cross(v.normal, v.tangent));
    }
}

void Shape::flattenVertices() {
    m_vertexData.clear();
    for (const Vertex& v : m_vertices) {
        // Position (3 floats)
        m_vertexData.push_back(v.position.x);
        m_vertexData.push_back(v.position.y);
        m_vertexData.push_back(v.position.z);
        // Normal (3 floats)
        m_vertexData.push_back(v.normal.x);
        m_vertexData.push_back(v.normal.y);
        m_vertexData.push_back(v.normal.z);
        // UV (2 floats)
        m_vertexData.push_back(v.uv.x);
        m_vertexData.push_back(v.uv.y);
        // Tangent (3 floats)
        m_vertexData.push_back(v.tangent.x);
        m_vertexData.push_back(v.tangent.y);
        m_vertexData.push_back(v.tangent.z);
        // Bitangent (3 floats)
        m_vertexData.push_back(v.bitangent.x);
        m_vertexData.push_back(v.bitangent.y);
        m_vertexData.push_back(v.bitangent.z);
    }
}
