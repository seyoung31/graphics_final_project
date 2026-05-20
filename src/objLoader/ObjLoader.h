#pragma once

#include "Shape.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

// Material data parsed from MTL file
struct ObjMaterial {
    std::string name;
    glm::vec3 ambient = glm::vec3(0.2f);   // Ka
    glm::vec3 diffuse = glm::vec3(0.8f);   // Kd
    glm::vec3 specular = glm::vec3(0.5f);  // Ks
    glm::vec3 emissive = glm::vec3(0.0f);  // Ke
    float shininess = 32.0f;               // Ns
    float opacity = 1.0f;                  // d
    float ior = 1.5f;                      // Ni (index of refraction)
    int illum = 2;                         // illumination model
    std::string diffuseTexture;            // map_Kd
    std::string normalMap;                 // map_Bump or bump
};

// Mesh group with associated material
struct ObjMeshGroup {
    std::string name;
    std::string materialName;
    std::vector<Vertex> vertices;
    std::vector<float> vertexData;  // Flattened for OpenGL
};

class ObjLoader : public Shape {
public:
    ObjLoader();
    ~ObjLoader() = default;

    // Load OBJ file from path
    bool loadFromFile(const std::string& filepath);
    
    // Clear loaded data
    void clear();
    
    // Get all mesh groups
    const std::vector<ObjMeshGroup>& getMeshGroups() const { return m_meshGroups; }
    
    // Get materials
    const std::unordered_map<std::string, ObjMaterial>& getMaterials() const { return m_materials; }
    
    // Get all vertices flattened (combines all groups)
    std::vector<float> generateShape() { return m_vertexData; }
    
    // Get number of vertices total
    int getVertexCount() const { return static_cast<int>(m_vertexData.size() / 14); }
    
    // Get the filepath of the loaded OBJ
    const std::string& getFilepath() const { return m_filepath; }
    
    // Get directory of the loaded OBJ (for resolving relative paths)
    const std::string& getDirectory() const { return m_directory; }
    
    // Public access to mesh groups for per-material rendering
    std::vector<ObjMeshGroup>& meshGroups() { return m_meshGroups; }

protected:
    void buildVertices() override;

private:
    // Parse OBJ file
    bool parseObjFile(const std::string& filepath);
    
    // Parse MTL file
    bool parseMtlFile(const std::string& filepath);
    
    // Process a face line (handles triangulation)
    void processFace(const std::string& line, 
                     const std::vector<glm::vec3>& positions,
                     const std::vector<glm::vec2>& texCoords,
                     const std::vector<glm::vec3>& normals,
                     ObjMeshGroup& currentGroup);
    
    // Parse a face vertex (v/vt/vn format)
    void parseFaceVertex(const std::string& vertex, int& posIdx, int& texIdx, int& normIdx);
    
    // Triangulate a polygon (fan triangulation)
    void triangulateFace(const std::vector<int>& posIndices,
                         const std::vector<int>& texIndices,
                         const std::vector<int>& normIndices,
                         const std::vector<glm::vec3>& positions,
                         const std::vector<glm::vec2>& texCoords,
                         const std::vector<glm::vec3>& normals,
                         ObjMeshGroup& group);
    
    // Flatten mesh group vertices into float array
    void flattenMeshGroup(ObjMeshGroup& group);
    
    // Combine all mesh groups into single vertex data
    void combineAllVertexData();
    
    // Compute tangent and bitangent for a triangle
    void computeTangentBitangent(Vertex& v0, Vertex& v1, Vertex& v2);
    
    // Helper to extract directory from filepath
    std::string extractDirectory(const std::string& filepath);
    
    // Helper to normalize path (resolve .. and .)
    std::string normalizePath(const std::string& baseDir, const std::string& relativePath);
    
    // Helper to trim whitespace
    std::string trim(const std::string& str);
    
    // Helper to extract texture path from line with options (e.g., -bm 1.0 path/to/tex.jpg)
    std::string extractTexturePathFromOptions(const std::string& line);

    std::string m_filepath;
    std::string m_directory;
    std::vector<ObjMeshGroup> m_meshGroups;
    std::unordered_map<std::string, ObjMaterial> m_materials;
};

