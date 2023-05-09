#version 330 core

const int MAX_JOINTS = 32;

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

uniform mat4 jointTransforms[MAX_JOINTS];

void main()
{
    vec4 totalPos = vec4(0.0);
    vec4 totalNormal = vec4(0.0);

    for (int i = 0; i < 4; ++i)
    {
        mat4 jointTransform = jointTransforms[aJoints[i]];
        totalPos += jointTransform * vec4(aPos, 1.0) * aWeights[i];

        totalNormal += jointTransform * vec4(aNormal, 0.0) * aWeights[i];
    }

    gl_Position = proj * view * model * totalPos;

    TexCoords = aTexCoords;
    Normal = mat3(transpose(inverse(model))) * totalNormal.xyz;
}