#version 330 core
in vec3 COL;

out vec4 color;

void main()
{
    color = vec4(COL,1);
}
