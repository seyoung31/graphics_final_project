#version 330 core


// in vec3 col;
// out vec4 fragColor;

// void main() {
//    fragColor = vec4(col, 1.0);
// }


//FROM LAB 10


// Task 5: declare "in" variables for the world-space position and normal,
//         received post-interpolation from the vertex shader
in vec3 world_pos;
in vec3 world_norm;
out vec4 fragColor; //output color

struct Light {
    int type; // 0 = directional, 1 = point, 2 = spot
    vec3 color;
    vec3 pos;
    vec3 dir;
    //attentuation params
    vec3 function;
    //spotlight angles
    float angle;
    float penumbra;
};
uniform Light lights[8];
uniform int numLights;

uniform vec3 world_cam;

//materials
uniform vec3 matAmbient;
uniform vec3 matDiffuse;
uniform vec3 matSpecular;
uniform float shininess;

// Task 12: declare relevant uniform(s) here, for ambient lighting
uniform float k_a;
// Task 13: declare relevant uniform(s) here, for diffuse lighting
uniform float k_d;
// Task 14: declare relevant uniform(s) here, for specular lighting
uniform float k_s;


float calcAttenuation(const Light light, float distance){
    float att = min(1.0, 1.0 / (light.function.x + light.function.y * distance + light.function.z * (distance * distance)));
    return att;
}

float calcFalloff(in Light light, vec3 light_dir) {
    // light_dir is normalized direction from surface point to light
    // We compare dot(-light_dir, normalize(light.dir)) because light.dir is the direction the spotlight points TOWARD.


    vec3 v = world_pos - light.pos;
    float x = acos(dot(normalize(v), normalize(light.dir)));
    float theta_in = light.angle - light.penumbra;
    float theta_out = light.angle;
    /*
    float falloff_calculation = (-2 * std::pow(( (x - theta_in) / (theta_out - theta_in)), 3) + 3 * std::pow(( (x - theta_in) / (theta_out - theta_in)), 2));
    float falloff = 0.0f;

    if (x <= theta_in){
        falloff = 1.0f;
    }
    else if (x >= theta_out){
        falloff = 0.0f;
    }
    else {
        falloff = 1.0f - falloff_calculation;
    }
    return falloff;
    */
    float falloff_calculation = (-2 * pow(( (x - theta_in) / (theta_out - theta_in)), 3) + 3 * pow(( (x - theta_in) / (theta_out - theta_in)), 2));
    float falloff = 0.0f;

    if (x <= theta_in){
        falloff = 1.0f;
    }
    else if (x >= theta_out){
        falloff = 0.0f;
    }
    else {
        falloff = 1.0f - falloff_calculation;
    }
    return falloff;

}

void main() {


    vec3 normal = normalize(world_norm);
    vec3 point = world_pos;

    // ambient term
    vec3 color = vec3(0);
    color += k_a * matAmbient;

    for (int i = 0; i < numLights ; i++) {
        Light light = lights[i];
        float distance = 1.0;
        float attenuation = 1.0;
        float falloff = 1.0;


        if (light.type == 0){
            attenuation = 1.0;
        } else {
            distance = length(world_pos - light.pos);
            attenuation = calcAttenuation(light, distance);
        }

        vec3 I_lambda_i = light.color;
        vec3 L;
        if (light.type == 1){
            L = normalize(light.pos - point);
        } else{
            L = normalize(-light.dir);
        }

        if (light.type == 2){
            falloff = calcFalloff(light, L); //check this
        }

        color += (falloff * attenuation * I_lambda_i) * (k_d * matDiffuse) * max( (dot(normal, L)), 0.0);
        vec3 Ri = reflect(-L, normal);
        vec3 V = normalize(world_cam - world_pos); //direction to camera
        float RiV;
        if (shininess != 0){
            RiV = pow(max(dot(Ri,V), 0.0), shininess);
        } else {
            RiV = max(dot(Ri,V), 0.0);
        }
        color += (falloff * attenuation * I_lambda_i) * (k_s * matSpecular * RiV);




        // if (L.type == 0) { // directional
        //     Li = normalize(-L.dir); // direction from surface to light
        //     attenuation = 1.0;
        // }
        // else {
        //     // point or spot
        //     vec3 toLight = L.pos - world_pos;
        //     float distance = length(toLight);
        //     Li = normalize(toLight);
        //     attenuation = calcAttenuation(L, distance);

        //     if (L.type == 2) { // spot
        //         spotFalloff = calcFalloff(L, Li);
        //         if (spotFalloff <= 0.0) continue; // outside cone
        //     }
        // }

        // diffuse
        // float NdotL = max(dot(N, Li), 0.0);
        // vec3 diffuse = matDiffuse * NdotL * k_d;

        // // specular (Phong using reflection vector)
        // vec3 R = reflect(-Li, N);
        // float RdotV = max(dot(R, V), 0.0);
        // vec3 spec = k_s * matSpecular * pow(RdotV, shininess);

        // vec3 lightContrib = (diffuse + spec) * L.color * attenuation * spotFalloff;
        // color += lightContrib;
    }

    fragColor = vec4(color, 1.0);


    // fragColor = vec4(1.0);
}


    // // Task 10: set your output color to white (i.e. vec4(1.0)). Make sure you get a white circle!
    // fragColor = vec4(1.0);

    // // Task 11: set your output color to the absolute value of your world-space normals,
    // //          to make sure your normals are correct.
    // fragColor = vec4(abs(world_norm), 1.0);
    // // Task 12: add ambient component to output color
    // fragColor = vec4(k_a);
    // // Task 13: add diffuse component to output color
    // vec3 n = normalize(world_norm);
    // vec3 L = normalize(vec3(light_pos) - world_pos);
    // fragColor += (k_d * clamp(dot(n, L), 0, 1));

    // // Task 14: add specular component to output color
    // vec3 R = reflect(-L, n);
    //     vec3 E = normalize(vec3(world_cam) - world_pos);
    //     float dot_prod = clamp(dot(R, E), 0.0, 1.0);

    //     fragColor += vec4(k_s * pow(dot_prod, shininess));


