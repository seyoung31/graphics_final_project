#version 330 core

// Declare 'in' variables received post-interpolation from the vertex shader
in vec3 w_pos; // world-space position
in vec3 w_normal; // world-space normal

// Declare an 'out' vec4 for outptu color
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
    float alpha = 0.002;

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

        // Phong shading
        float diff = max(dot(N, L), 0.0);
        vec3 R = reflect(-L, N);
        float specIntensity = pow(max(dot(R, V), 0.0), shininess);

        vec3 diffuse = k_d * diff * lightColor[i];
        vec3 specular = k_s * specIntensity * lightColor[i];

        //shadow mapping
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
