#version 330 core

out vec4 frag_color;

struct AmbientLight {
    bool enabled;
    vec3 ambient;
};

struct PointLight {
    bool enabled;
    vec3 position;
    vec3 diffuse;
    vec3 specular;
    vec3 attenuation;
};

struct DirectionalLight {
    bool enabled;
    vec3 direction;
    vec3 diffuse;
    vec3 specular;
};
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Shadows {
    bool enabled;
    //float bias;
    mat4x4 lightNDC;
};

in vec3 world_position;
in vec3 world_normal;
in vec2 tex_coord;

uniform vec3 camera_position;

uniform Material material; // material properties of the object
uniform AmbientLight ambient_light;
uniform PointLight point_light; 
uniform DirectionalLight directional_light;
uniform Shadows shadow_info;
uniform sampler2D ambient_texture;
uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;
uniform sampler2D shadow_map;

vec3 CalcAmbientLight();
vec3 CalcPointLight(vec3 normal, vec3 view_dir);
vec3 CalcDirectionalLight(vec3 normal, vec3 view_dir);
float rand(vec2 coord);



void main() {
    vec3 normal = normalize(world_normal);
    vec3 view_dir = normalize(camera_position - world_position);

    frag_color = vec4(0.0);

    float isUnshaded = 1.;
    // Depth calculations
    if (shadow_info.enabled) {
        // NDC: [-1, 1] -> uv: [0, 1]
        vec4 mapUV = ((shadow_info.lightNDC * vec4(world_position, 1.)) + 1.) * 0.5;
        vec2 shadowUV = mapUV.xy;
        float fragmentDepth = mapUV.z;
        vec2 invMapSize = 1.f/textureSize(shadow_map, 0);
        float bias = 0.001f;
        float isShaded = 0.;
        // grid size = radius * 2 + 1
        int pcfGridRadius = 3;
        float pcfWeight = 1. / pow(pcfGridRadius * 2 + 1, 2.);
        for(float x = -pcfGridRadius; x < pcfGridRadius + 1; x++){
            for(float y = -pcfGridRadius; y < pcfGridRadius + 1; y++){
                vec2 uv = shadowUV + vec2(
                        x + rand(world_position.xy + y + world_position.z),
                        y + rand(world_position.xy + x + world_position.z)
                    )*invMapSize;
                float mapDepth = texture(shadow_map, uv).r;
                isShaded += float(mapDepth + bias < fragmentDepth);
            }
        }
        
        isShaded *= pcfWeight;
        
        isUnshaded = 1. - isShaded;
    }
    
    if (ambient_light.enabled) {
        frag_color += vec4(isUnshaded * CalcAmbientLight(), 1.);
    }
    
    if (point_light.enabled) {
        frag_color += vec4(isUnshaded * CalcPointLight(normal, view_dir), 1.);
    }

    if (directional_light.enabled) {
        frag_color += vec4(isUnshaded * CalcDirectionalLight(normal, view_dir), 1.);
    }
    
}

// adapted from https://thebookofshaders.com/10/
float rand(vec2 coord){
    return fract(sin(dot(coord.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123) * 2. - 1.;
}

vec3 GetAmbientColor() {
    return vec3(texture(ambient_texture, tex_coord));
    // return material.ambient;
}

vec3 GetDiffuseColor() {
    return vec3(texture(diffuse_texture, tex_coord));
    // return material.diffuse;
}

vec3 GetSpecularColor() {
    return vec3(texture(specular_texture, tex_coord));
    // return material.specular;
}

vec3 CalcAmbientLight() {
    return ambient_light.ambient * GetAmbientColor();
}

vec3 CalcPointLight(vec3 normal, vec3 view_dir) {
    PointLight light = point_light;
    vec3 light_dir = normalize(light.position - world_position);

    float diffuse_intensity = max(dot(normal, light_dir), 0.0);
    vec3 diffuse_color = diffuse_intensity * light.diffuse * GetDiffuseColor();

    vec3 reflect_dir = reflect(-light_dir, normal);
    float specular_intensity = pow(
        max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular_color = specular_intensity * 
        light.specular * GetSpecularColor();

    float distance = length(light.position - world_position);
    float attenuation = 1.0 / (light.attenuation.x + 
        light.attenuation.y * distance + 
        light.attenuation.z * (distance * distance));

    return attenuation * (diffuse_color + specular_color);
}

vec3 CalcDirectionalLight(vec3 normal, vec3 view_dir) {
    DirectionalLight light = directional_light;
    vec3 light_dir = normalize(-light.direction);
    float diffuse_intensity = max(dot(normal, light_dir), 0.0);
    vec3 diffuse_color = diffuse_intensity * light.diffuse * GetDiffuseColor();

    vec3 reflect_dir = reflect(-light_dir, normal);
    float specular_intensity = pow(
        max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular_color = specular_intensity * 
        light.specular * GetSpecularColor();

    vec3 final_color = diffuse_color + specular_color;
    return final_color;
}