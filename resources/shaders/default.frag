#version 330 core

// Declare 'in' variables received post-interpolation from the vertex shader
in vec3 w_pos; // world-space position
in vec3 w_normal; // world-space normal
in vec2 fragUV;
in mat3 TBN;

// Declare an 'out' vec4 for output color
out vec4 fragColor;

// Material parameters
uniform vec3 k_a;
uniform vec3 k_d;
uniform vec3 k_s;
uniform float shininess;

// Lighting
// LightType enum is: 0 = LIGHT_POINT, 1 = LIGHT_DIRECTIONAL, 2 = LIGHT_SPOT
const int MAX_LIGHTS = 8;
uniform int numLights;

uniform int lightType[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];
uniform vec3 lightPos[MAX_LIGHTS];
uniform vec3 lightDir[MAX_LIGHTS];
uniform vec3 lightFunc[MAX_LIGHTS];
uniform float lightAngle[MAX_LIGHTS];
uniform float lightPenumbra[MAX_LIGHTS];

uniform vec4 camPos; // world-space camera pos
uniform mat4 view; // view matrix for TBN calculations

// Texture samplers
uniform sampler2D DiffuseTextureSampler;
uniform sampler2D NormalTextureSampler;

// Texture control flags
uniform bool useNormalMapping;
uniform bool useTextureMap;
uniform float textureRepeatU;
uniform float textureRepeatV;

//SHADOW MAPPING VARS
in vec4 lightSpacePos[8]; //for shadow mapping
uniform sampler2D shadowMaps[8];  // 2D maps for dir/spot
uniform int use_shadow_mapping; //boolean for shadow mapping

float shadowFactor(int i) { //check how in shadow things are
    vec3 proj = lightSpacePos[i].xyz / lightSpacePos[i].w; //get perspective

    proj = proj * 0.5 + 0.5; // bound from 0,1 instead of -1,1

    // lit if not in the map
    if (proj.x < 0.0 || proj.x > 1.0 ||
        proj.y < 0.0 || proj.y > 1.0 ||
        proj.z < 0.0 || proj.z > 1.0) {
        return 1.0;
    }

    float closest = texture(shadowMaps[i], proj.xy).r;
    float current = proj.z;

    // to get rid of shadow acne
    float alpha = 0.02;

    if (current - alpha > closest) {
        return 0.0;
    } else {
        return 1.0;
    }
}

void main() {
    // Normalize normals (for interpolation)
    vec3 N = normalize(w_normal);
    vec3 V = normalize(vec3(camPos) - w_pos); // view dir
    
    // Sample diffuse texture if available
    vec3 diffuseColor = k_d;
    if (useTextureMap) {
        vec2 scaledUV = fragUV * vec2(textureRepeatU, textureRepeatV);
        vec4 diffuseTexSample = texture(DiffuseTextureSampler, scaledUV);
        diffuseColor = diffuseTexSample.rgb;
    }

    vec3 color = k_a * vec3(1.0);

    for (int i=0; i < numLights; ++i) {
        int t = lightType[i];

        vec3 L; // direction from surface to light
        float attenuation = 1.0;
        float spotFactor = 1.0;

        if (t == 1) {
            // Directional light
            L = normalize(-lightDir[i]);
        } else {
            // Point or spot light
            vec3 toLight = lightPos[i] - w_pos;
            float d = length(toLight);

            if (d > 0.0) {
                L = toLight / d;
            } else {
                L = vec3(0.0, 1.0, 0.0);
            }

            // Attenuation calculation
            vec3 func = lightFunc[i];
            attenuation = 1.0 / max(func.x + func.y * d + func.z * d * d, 0.0001);

            if (t == 2) {
                // Spot light: extra angular falloff
                vec3 spotDir = normalize(lightDir[i]);
                float cosTheta = dot(spotDir, -L);

                // Convert angles to radians
                float x = acos(clamp(cosTheta, -1.0, 1.0));

                float outer = lightAngle[i];
                float inner = max(lightAngle[i] - lightPenumbra[i], 0.0f);
                inner = min(inner, outer - 1e-4);

                if (x <= inner) {
                    spotFactor = 1.0;
                } else if (x >= outer) {
                    spotFactor = 0.0;
                } else {
                    float u = (x - inner) / (outer - inner);
                    float fall = -2.0 * pow(u, 3.0) + 3.0 * pow(u, 2.0);
                    spotFactor = 1.0 - fall;
                }

            }
        }

        float diff;
        float specIntensity;
        
        // If using normal mapping, do lighting in tangent space
        if (useNormalMapping) {
            mat3 View3x3 = mat3(view);
            vec3 L_cameraspace = normalize(View3x3 * L);
            vec3 l_tangentspace = TBN * L_cameraspace;
            // Transform eye direction to tangent space
            vec3 E_tangentspace = TBN * normalize(View3x3 * V);
            
            // Get normal in tangent space (from the normal map)
            vec3 n = normalize(texture(NormalTextureSampler, fragUV).rgb * 2.0 - 1.0);
            
            // Diffuse lighting: clamp(dot(n,l), 0, 1) with n and l in tangent space
            diff = clamp(dot(n, l_tangentspace), 0.0, 1.0);
            
            // Specular lighting: clamp(dot(E,R), 0, 1) with E and R in tangent space
            vec3 R_tangentspace = reflect(-l_tangentspace, n);
            float rdotv = clamp(dot(E_tangentspace, R_tangentspace), 0.0, 1.0);
            
            if (diff > 0.0 && rdotv > 0.0 && shininess > 0.0) {
                specIntensity = pow(rdotv, shininess);
            } else {
                specIntensity = 0.0;
            }
        } else {
            // Standard Phong shading in world space
            diff = max(dot(N, L), 0.0);
            vec3 R = reflect(-L, N);
            specIntensity = pow(max(dot(R, V), 0.0), shininess);
        }

        vec3 diffuse = diffuseColor * diff * lightColor[i];
        vec3 specular = k_s * specIntensity * lightColor[i];

        // shadow mapping
        float s = 1.0;
        if (use_shadow_mapping == 1 && lightType[i] != 0) { //not dealing with point lights :)
            //here we modify the diffuse/spec factor by the shadow factor!
            s = shadowFactor(i);
            diffuse *= s;
            specular *= s;

        }

        color += (diffuse + specular) * attenuation * spotFactor;

    }

    fragColor = vec4(color, 1.0);

}
