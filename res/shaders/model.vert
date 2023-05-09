#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in uvec4 aJoints;
layout (location = 4) in vec4  aWeights;

out vec2 TexCoords;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    gl_Position = proj * view * model * vec4(aPos, 1.0);

    TexCoords = aTexCoords;
    Normal = mat3(transpose(inverse(model))) * aNormal;
}