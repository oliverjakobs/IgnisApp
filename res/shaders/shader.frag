#version 330 core

out vec4 fColor;

in vec4 vColor;

void main()
{
    fColor = vColor;
}