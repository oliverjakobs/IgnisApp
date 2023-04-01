#version 330 core

out vec4 FragColor;

in vec3 WorldPos;

uniform vec3 lightPos;
uniform vec3 lightColor = vec3(1.0);
uniform vec3 objectColor = vec3(0.4, 0.4, 0.4);

void main()
{
    // get face normal
    vec3 xTangent = dFdx(WorldPos);
    vec3 yTangent = dFdy(WorldPos);
    vec3 normal = normalize(cross(xTangent, yTangent));

    // ambient
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * lightColor;

    // diffuse 
    vec3 lightDir = normalize(lightPos - WorldPos);
    vec3 diffuse = max(dot(lightDir, normal), 0.0) * lightColor;

    vec3 result = (ambient + diffuse) * objectColor;
    FragColor = vec4(result, 1.0);
}