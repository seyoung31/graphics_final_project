#include "ObjLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>

ObjLoader::ObjLoader() {
}

void ObjLoader::clear() {
    m_meshGroups.clear();
    m_materials.clear();
    m_vertices.clear();
    m_vertexData.clear();
    m_filepath.clear();
    m_directory.clear();
}

bool ObjLoader::loadFromFile(const std::string& filepath) {
    clear();
    m_filepath = filepath;
    m_directory = extractDirectory(filepath);
    
    std::cout << "[ObjLoader] Attempting to load: " << filepath << std::endl;
    
    if (!parseObjFile(filepath)) {
        std::cerr << "[ObjLoader] Failed to parse OBJ file: " << filepath << std::endl;
        return false;
    }
    
    // Combine all vertex data for simple rendering
    combineAllVertexData();
    
    std::cout << "Loaded OBJ: " << filepath << std::endl;
    std::cout << "  Mesh groups: " << m_meshGroups.size() << std::endl;
    std::cout << "  Materials: " << m_materials.size() << std::endl;
    std::cout << "  Total vertices: " << getVertexCount() << std::endl;
    
    return true;
}

bool ObjLoader::parseObjFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Cannot open OBJ file: " << filepath << std::endl;
        return false;
    }
    
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    
    // Start with a default group
    ObjMeshGroup currentGroup;
    currentGroup.name = "default";
    currentGroup.materialName = "";
    
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            // Vertex position
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (prefix == "vt") {
            // Texture coordinate
            glm::vec2 tex;
            iss >> tex.x >> tex.y;
            texCoords.push_back(tex);
        }
        else if (prefix == "vn") {
            // Vertex normal
            glm::vec3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            normals.push_back(glm::normalize(norm));
        }
        else if (prefix == "f") {
            // Face - delegate to processFace which handles triangulation
            processFace(line, positions, texCoords, normals, currentGroup);
        }
        else if (prefix == "usemtl") {
            // Switch material - save current group if it has vertices
            if (!currentGroup.vertices.empty()) {
                flattenMeshGroup(currentGroup);
                m_meshGroups.push_back(currentGroup);
                currentGroup.vertices.clear();
                currentGroup.vertexData.clear();
            }
            iss >> currentGroup.materialName;
        }
        else if (prefix == "o" || prefix == "g") {
            // Object or group name
            if (!currentGroup.vertices.empty()) {
                flattenMeshGroup(currentGroup);
                m_meshGroups.push_back(currentGroup);
                currentGroup.vertices.clear();
                currentGroup.vertexData.clear();
            }
            iss >> currentGroup.name;
        }
        else if (prefix == "mtllib") {
            // Material library file
            std::string mtlFile;
            iss >> mtlFile;
            std::string mtlPath = m_directory + "/" + mtlFile;
            parseMtlFile(mtlPath);
        }
    }
    
    // Don't forget the last group
    if (!currentGroup.vertices.empty()) {
        flattenMeshGroup(currentGroup);
        m_meshGroups.push_back(currentGroup);
    }
    
    file.close();
    return true;
}

bool ObjLoader::parseMtlFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Cannot open MTL file: " << filepath << std::endl;
        return false;
    }
    
    // Extract MTL file directory for resolving relative paths
    std::string mtlDirectory = extractDirectory(filepath);
    
    ObjMaterial currentMaterial;
    bool hasMaterial = false;
    
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "newmtl") {
            // Save previous material
            if (hasMaterial) {
                m_materials[currentMaterial.name] = currentMaterial;
            }
            // Start new material
            currentMaterial = ObjMaterial();
            iss >> currentMaterial.name;
            hasMaterial = true;
        }
        else if (prefix == "Ka") {
            iss >> currentMaterial.ambient.x >> currentMaterial.ambient.y >> currentMaterial.ambient.z;
        }
        else if (prefix == "Kd") {
            iss >> currentMaterial.diffuse.x >> currentMaterial.diffuse.y >> currentMaterial.diffuse.z;
        }
        else if (prefix == "Ks") {
            iss >> currentMaterial.specular.x >> currentMaterial.specular.y >> currentMaterial.specular.z;
        }
        else if (prefix == "Ke") {
            iss >> currentMaterial.emissive.x >> currentMaterial.emissive.y >> currentMaterial.emissive.z;
        }
        else if (prefix == "Ns") {
            iss >> currentMaterial.shininess;
        }
        else if (prefix == "Ni") {
            iss >> currentMaterial.ior;
        }
        else if (prefix == "d") {
            iss >> currentMaterial.opacity;
        }
        else if (prefix == "Tr") {
            // Transparency (1 - d)
            float tr;
            iss >> tr;
            currentMaterial.opacity = 1.0f - tr;
        }
        else if (prefix == "illum") {
            iss >> currentMaterial.illum;
        }
        else if (prefix == "map_Kd") {
            // Diffuse texture - get rest of line (may contain spaces)
            std::string texPath;
            std::getline(iss, texPath);
            texPath = trim(texPath);
            // Resolve relative paths relative to MTL file directory
            if (!texPath.empty() && texPath[0] != '/') {
                // Relative path - resolve relative to MTL file directory
                currentMaterial.diffuseTexture = normalizePath(mtlDirectory, texPath);
            } else {
                // Absolute path - use as-is
                currentMaterial.diffuseTexture = texPath;
            }
        }
        else if (prefix == "map_Bump" || prefix == "bump") {
            std::string texPath;
            std::getline(iss, texPath);
            texPath = trim(texPath);
            // Resolve relative paths relative to MTL file directory
            if (!texPath.empty() && texPath[0] != '/') {
                // Relative path - resolve relative to MTL file directory
                currentMaterial.normalMap = normalizePath(mtlDirectory, texPath);
            } else {
                // Absolute path - use as-is
                currentMaterial.normalMap = texPath;
            }
        }
    }
    
    // Save last material
    if (hasMaterial) {
        m_materials[currentMaterial.name] = currentMaterial;
    }
    
    file.close();
    return true;
}

void ObjLoader::processFace(const std::string& line,
                            const std::vector<glm::vec3>& positions,
                            const std::vector<glm::vec2>& texCoords,
                            const std::vector<glm::vec3>& normals,
                            ObjMeshGroup& currentGroup) {
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix; // Skip "f"
    
    std::vector<int> posIndices;
    std::vector<int> texIndices;
    std::vector<int> normIndices;
    
    std::string vertexStr;
    while (iss >> vertexStr) {
        int posIdx = 0, texIdx = 0, normIdx = 0;
        parseFaceVertex(vertexStr, posIdx, texIdx, normIdx);
        posIndices.push_back(posIdx);
        texIndices.push_back(texIdx);
        normIndices.push_back(normIdx);
    }
    
    // Triangulate the face
    triangulateFace(posIndices, texIndices, normIndices, 
                    positions, texCoords, normals, currentGroup);
}

void ObjLoader::parseFaceVertex(const std::string& vertex, int& posIdx, int& texIdx, int& normIdx) {
    posIdx = 0;
    texIdx = 0;
    normIdx = 0;
    
    std::istringstream viss(vertex);
    std::string part;
    int partIndex = 0;
    
    while (std::getline(viss, part, '/')) {
        if (!part.empty()) {
            int idx = std::stoi(part);
            switch (partIndex) {
                case 0: posIdx = idx; break;
                case 1: texIdx = idx; break;
                case 2: normIdx = idx; break;
            }
        }
        partIndex++;
    }
}

void ObjLoader::triangulateFace(const std::vector<int>& posIndices,
                                const std::vector<int>& texIndices,
                                const std::vector<int>& normIndices,
                                const std::vector<glm::vec3>& positions,
                                const std::vector<glm::vec2>& texCoords,
                                const std::vector<glm::vec3>& normals,
                                ObjMeshGroup& group) {
    int numVerts = static_cast<int>(posIndices.size());
    if (numVerts < 3) return;
    
    // Fan triangulation: create triangles (0, i, i+1) for i from 1 to n-2
    for (int i = 1; i < numVerts - 1; i++) {
        // Triangle: vertex 0, i, i+1
        int indices[3] = {0, i, i + 1};
        
        Vertex triangle[3];
        
        for (int j = 0; j < 3; j++) {
            int vi = indices[j];
            
            // Position (OBJ indices are 1-based, can be negative)
            int posIdx = posIndices[vi];
            if (posIdx < 0) posIdx = static_cast<int>(positions.size()) + posIdx + 1;
            if (posIdx > 0 && posIdx <= static_cast<int>(positions.size())) {
                triangle[j].position = positions[posIdx - 1];
            }
            
            // Texture coordinate
            int texIdx = texIndices[vi];
            if (texIdx < 0) texIdx = static_cast<int>(texCoords.size()) + texIdx + 1;
            if (texIdx > 0 && texIdx <= static_cast<int>(texCoords.size())) {
                triangle[j].uv = texCoords[texIdx - 1];
            }
            
            // Normal
            int normIdx = normIndices[vi];
            if (normIdx < 0) normIdx = static_cast<int>(normals.size()) + normIdx + 1;
            if (normIdx > 0 && normIdx <= static_cast<int>(normals.size())) {
                triangle[j].normal = normals[normIdx - 1];
            } else {
                // Compute face normal if not provided
                if (j == 2) {
                    glm::vec3 edge1 = triangle[1].position - triangle[0].position;
                    glm::vec3 edge2 = triangle[2].position - triangle[0].position;
                    glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));
                    triangle[0].normal = faceNormal;
                    triangle[1].normal = faceNormal;
                    triangle[2].normal = faceNormal;
                }
            }
        }
        
        // Compute tangent and bitangent
        computeTangentBitangent(triangle[0], triangle[1], triangle[2]);
        
        // Add vertices to group
        group.vertices.push_back(triangle[0]);
        group.vertices.push_back(triangle[1]);
        group.vertices.push_back(triangle[2]);
    }
}

void ObjLoader::computeTangentBitangent(Vertex& v0, Vertex& v1, Vertex& v2) {
    glm::vec3 deltaPos1 = v1.position - v0.position;
    glm::vec3 deltaPos2 = v2.position - v0.position;
    
    glm::vec2 deltaUV1 = v1.uv - v0.uv;
    glm::vec2 deltaUV2 = v2.uv - v0.uv;
    
    float denom = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
    
    glm::vec3 tangent(1.0f, 0.0f, 0.0f);
    glm::vec3 bitangent(0.0f, 1.0f, 0.0f);
    
    if (std::abs(denom) > 1e-6f) {
        float r = 1.0f / denom;
        tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
    }
    
    // Normalize and orthogonalize tangent
    auto orthoTangent = [](const glm::vec3& n, const glm::vec3& t) -> glm::vec3 {
        if (glm::length(t) > 1e-6f) {
            glm::vec3 tn = glm::normalize(t);
            return glm::normalize(tn - glm::dot(tn, n) * n);
        }
        // Fallback: create arbitrary tangent perpendicular to normal
        glm::vec3 up = std::abs(n.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        return glm::normalize(glm::cross(up, n));
    };
    
    v0.tangent = orthoTangent(v0.normal, tangent);
    v1.tangent = orthoTangent(v1.normal, tangent);
    v2.tangent = orthoTangent(v2.normal, tangent);
    
    // Calculate bitangent using cross product, preserving handedness from UVs
    // Standard OpenGL convention: bitangent = cross(normal, tangent)
    // But we need to check if this matches the UV-based calculation
    glm::vec3 computedBitangent = glm::cross(v0.normal, v0.tangent);
    
    // If the computed bitangent doesn't align with the UV-based one, flip it
    // This handles mirrored UVs correctly
    if (glm::dot(computedBitangent, bitangent) < 0.0f) {
        computedBitangent = -computedBitangent;
    }
    
    v0.bitangent = glm::normalize(computedBitangent);
    v1.bitangent = glm::normalize(computedBitangent);
    v2.bitangent = glm::normalize(computedBitangent);
}

void ObjLoader::flattenMeshGroup(ObjMeshGroup& group) {
    group.vertexData.clear();
    for (const Vertex& v : group.vertices) {
        // Position (3 floats)
        group.vertexData.push_back(v.position.x);
        group.vertexData.push_back(v.position.y);
        group.vertexData.push_back(v.position.z);
        // Normal (3 floats)
        group.vertexData.push_back(v.normal.x);
        group.vertexData.push_back(v.normal.y);
        group.vertexData.push_back(v.normal.z);
        // UV (2 floats)
        group.vertexData.push_back(v.uv.x);
        group.vertexData.push_back(v.uv.y);
        // Tangent (3 floats)
        group.vertexData.push_back(v.tangent.x);
        group.vertexData.push_back(v.tangent.y);
        group.vertexData.push_back(v.tangent.z);
        // Bitangent (3 floats)
        group.vertexData.push_back(v.bitangent.x);
        group.vertexData.push_back(v.bitangent.y);
        group.vertexData.push_back(v.bitangent.z);
    }
}

void ObjLoader::combineAllVertexData() {
    m_vertexData.clear();
    m_vertices.clear();
    
    for (const ObjMeshGroup& group : m_meshGroups) {
        // Combine flattened data
        m_vertexData.insert(m_vertexData.end(), 
                            group.vertexData.begin(), 
                            group.vertexData.end());
        // Combine vertices
        m_vertices.insert(m_vertices.end(),
                          group.vertices.begin(),
                          group.vertices.end());
    }
}

void ObjLoader::buildVertices() {
    // This is called by Shape::setVertexData()
    // For OBJ loading, we load from file instead
    // So this is a no-op; use loadFromFile() instead
}

std::string ObjLoader::extractDirectory(const std::string& filepath) {
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash == std::string::npos) {
        return ".";
    }
    return filepath.substr(0, lastSlash);
}

std::string ObjLoader::normalizePath(const std::string& baseDir, const std::string& relativePath) {
    // Check if base directory is absolute
    bool isAbsolute = !baseDir.empty() && (baseDir[0] == '/' || (baseDir.length() > 1 && baseDir[1] == ':'));
    
    // Split both paths into components
    std::vector<std::string> baseParts;
    std::vector<std::string> relParts;
    
    // Split base directory
    std::string base = baseDir;
    size_t pos = 0;
    while ((pos = base.find_first_of("/\\")) != std::string::npos) {
        if (pos > 0) {
            baseParts.push_back(base.substr(0, pos));
        }
        base = base.substr(pos + 1);
    }
    if (!base.empty()) {
        baseParts.push_back(base);
    }
    
    // Split relative path
    std::string rel = relativePath;
    pos = 0;
    while ((pos = rel.find_first_of("/\\")) != std::string::npos) {
        if (pos > 0) {
            relParts.push_back(rel.substr(0, pos));
        }
        rel = rel.substr(pos + 1);
    }
    if (!rel.empty()) {
        relParts.push_back(rel);
    }
    
    // Process relative path components
    for (const std::string& part : relParts) {
        if (part == "..") {
            // Go up one directory
            if (!baseParts.empty()) {
                baseParts.pop_back();
            }
        } else if (part != "." && !part.empty()) {
            // Add directory/file component
            baseParts.push_back(part);
        }
        // "." means current directory, so we ignore it
    }
    
    // Reconstruct path
    if (baseParts.empty()) {
        return isAbsolute ? "/" : ".";
    }
    
    std::string result;
    if (isAbsolute) {
        result = "/";
    }
    for (size_t i = 0; i < baseParts.size(); ++i) {
        if (i > 0 || !isAbsolute) {
            result += "/";
        }
        result += baseParts[i];
    }
    
    return result;
}

std::string ObjLoader::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

