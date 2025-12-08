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

// SHADOW MAPPING VARS
in vec4 lightSpacePos[8]; //for shadow mapping
uniform sampler2D shadowMaps[8];  // 2D maps for dir/spot
uniform int use_shadow_mapping; //boolean for shadow mapping

// SCROLLING TEXTURE FOR WATER VARS
uniform sampler2D u_waterTex;
uniform sampler2D u_dispTex;

uniform float u_time;
uniform int u_enableWater;

uniform vec2  u_waterTexScale;
uniform vec2  u_dispTexScale;
uniform vec2  u_dispScrollDir;
uniform float u_dispScrollSpeed;
uniform float u_dispStrength;
uniform float u_dispContrast;

uniform float u_waterPlaneY;
uniform float u_waterPlaneThickness;

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

float shadowFactorPCF(int i) {
    //shadow factor with some offset now to make soft

    vec3 proj = lightSpacePos[i].xyz / lightSpacePos[i].w;
    proj = proj * 0.5 + 0.5;

    if (proj.x < 0.0 || proj.x > 1.0 ||
        proj.y < 0.0 || proj.y > 1.0 ||
        proj.z < 0.0 || proj.z > 1.0) {
        return 1.0;
    }

    float current = proj.z;
    float alpha = 0.02; // bias

    // texel size in UV space
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMaps[i], 0));

    // kernel radius in texels (tune 1–3)
    int radius = 2;

    float sum = 0.0;
    float count = 0.0;

    // float softness = 1;
    float distSoft = mix(1.5, 3.0, proj.z); // proj.z in [0,1]
    float softness = (lightType[i] == 1) ? distSoft : 2.0;
    softness = 2.5;
    // softness = (lightType[i] == 2) ? distSoft : 3.0;



    //softening
    for (int x = -radius; x <= radius; x++) {
        for (int y = -radius; y <= radius; y++) {
            vec2 offset = vec2(x, y) * texelSize * softness;
            vec2 uvTap = clamp(proj.xy + offset, vec2(0.0), vec2(1.0));
            float closest = texture(shadowMaps[i], uvTap).r;
            // float closest = texture(shadowMaps[i], proj.xy + offset).r;

            sum += (current - alpha > closest) ? 0.0 : 1.0;
            count += 1.0;
        }
    }

    return sum / count; // smooth edge
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
                vec3 spotDir = normalize(lightDir[i]);
                float cosTheta = dot(spotDir, -L);

                // x in DEGREES
                float x = degrees(acos(clamp(cosTheta, -1.0, 1.0)));

                // lightAngle / lightPenumbra currently arriving in RADIANS -> convert to DEGREES
                float outer = degrees(lightAngle[i]);
                float inner = degrees(max(lightAngle[i] - lightPenumbra[i], 0.0));
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
            
            // Get normal in tangent space (from the normal map) - use same UV scaling as diffuse
            vec2 normalUV = fragUV * vec2(textureRepeatU, textureRepeatV);
            vec3 n = normalize(texture(NormalTextureSampler, normalUV).rgb * 2.0 - 1.0);
            
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
            s = shadowFactorPCF(i);
            diffuse *= s;
            specular *= s;

        }

        color += (diffuse + specular) * attenuation * spotFactor;

    }

    fragColor = vec4(color, 1.0);

}
