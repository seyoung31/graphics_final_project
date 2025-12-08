#version 330 core

// Input variables from vertex shader
in vec3 worldPosition;
in vec3 worldNormal;
in vec2 fragUV;
in mat3 TBN;
in vec3 tangent_cameraspace;
in vec3 bitangent_cameraspace;
in vec3 normal_cameraspace;

// Output color
out vec4 fragColor;

// Bump map sampler (grayscale height map)
uniform sampler2D BumpTextureSampler;

// Diffuse texture sampler
uniform sampler2D DiffuseTextureSampler;
uniform bool useTextureMap;
uniform float textureRepeatU;
uniform float textureRepeatV;

// Rendering pass mode (kept for compatibility, but we use single-pass now)
uniform int bumpPassMode;

// Material parameters
uniform vec3 k_a;
uniform vec3 k_d;
uniform vec3 k_s;
uniform float shininess;

// Camera and view
uniform vec4 camPos;
uniform mat4 view;

// Lighting
const int MAX_LIGHTS = 8;
uniform int numLights;
uniform int lightType[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform vec3 lightPos[MAX_LIGHTS];
uniform vec3 lightDir[MAX_LIGHTS];
uniform vec3 lightFunc[MAX_LIGHTS];
uniform float lightAngle[MAX_LIGHTS];
uniform float lightPenumbra[MAX_LIGHTS];

// Compute perturbed normal from height map using finite differences
vec3 computeBumpNormal(vec2 uv, vec3 N) {
    float bumpStrength = 1.5;
    vec2 texelSize = vec2(1.0 / 512.0); // Approximate texel size
    
    // Sample height at neighboring points
    float h = texture(BumpTextureSampler, uv).r;
    float hU = texture(BumpTextureSampler, uv + vec2(texelSize.x, 0.0)).r;
    float hV = texture(BumpTextureSampler, uv + vec2(0.0, texelSize.y)).r;
    
    // Compute gradient (partial derivatives)
    float dU = (hU - h) * bumpStrength;
    float dV = (hV - h) * bumpStrength;
    
    // Perturb normal in tangent space, then transform to world space
    vec3 bumpNormal = normalize(N + tangent_cameraspace * dU + bitangent_cameraspace * dV);
    return bumpNormal;
}

void main() {
    vec2 scaledUV = fragUV * vec2(textureRepeatU, textureRepeatV);
    
    // Get diffuse color
    vec3 diffuseColor = k_d;
    if (useTextureMap) {
        vec4 diffuseTexSample = texture(DiffuseTextureSampler, scaledUV);
        diffuseColor = diffuseTexSample.rgb;
    }
    
    vec3 V = normalize(vec3(camPos) - worldPosition);
    vec3 N = normalize(worldNormal);
    
    // Compute perturbed normal from bump map
    vec3 bumpN = computeBumpNormal(scaledUV, N);
    
    // Start with ambient
    vec3 color = k_a * diffuseColor * 0.3;
    
    for (int i = 0; i < numLights; i++) {
        int t = lightType[i];
        
        vec3 L;
        float attenuation = 1.0;
        float spotFactor = 1.0;
        
        if (t == 1) {
            // Directional light
            L = normalize(-lightDir[i]);
        } else {
            // Point or spot light
            vec3 toLight = lightPos[i] - worldPosition;
            float d = length(toLight);
            
            if (d > 0.0) {
                L = toLight / d;
            } else {
                L = vec3(0.0, 1.0, 0.0);
            }
            
            vec3 func = lightFunc[i];
            attenuation = 1.0 / max(func.x + func.y * d + func.z * d * d, 0.0001);
            
            if (t == 2) {
                vec3 spotDir = normalize(lightDir[i]);
                float cosAngle = dot(-L, spotDir);
                float cosInner = cos(lightAngle[i] - lightPenumbra[i]);
                float cosOuter = cos(lightAngle[i]);
                
                if (cosAngle < cosOuter) {
                    spotFactor = 0.0;
                } else if (cosAngle < cosInner) {
                    float t_val = (cosAngle - cosInner) / (cosOuter - cosInner);
                    spotFactor = 1.0 - (-2.0 * t_val * t_val * t_val + 3.0 * t_val * t_val);
                }
            }
        }
        
        // Diffuse using perturbed normal
        float diff = max(dot(bumpN, L), 0.0);
        vec3 diffuse = diffuseColor * diff * lightColor[i];
        
        // Specular using perturbed normal
        vec3 R = reflect(-L, bumpN);
        float spec = pow(max(dot(R, V), 0.0), shininess);
        vec3 specular = k_s * spec * lightColor[i];
        
        if (t != 1) {
            diffuse *= attenuation * spotFactor;
            specular *= attenuation * spotFactor;
        }
        
        color += diffuse + specular;
    }
    
    fragColor = vec4(color, 1.0);
}

