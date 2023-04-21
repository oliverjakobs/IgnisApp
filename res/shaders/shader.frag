#version 330 core

in vec2 TexCoords;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D baseTexture;
uniform vec3 baseColor = vec3(0.4);

void main()
{
    FragColor = vec4(baseColor, 1.0) * texture(baseTexture, TexCoords);
}