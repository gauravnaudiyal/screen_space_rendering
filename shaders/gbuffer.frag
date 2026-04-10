#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Normal;

uniform vec3  albedo;
uniform float reflectivity;

void main() {
    gPosition        = FragPos;
    gNormal          = normalize(Normal);
    gAlbedoSpec.rgb  = albedo;
    gAlbedoSpec.a    = reflectivity;
}