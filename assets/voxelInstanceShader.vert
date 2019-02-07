#version  330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;
layout (location = 2) in mat4 instanceMatrix;

/* uniform mat4 model; */
uniform mat4 view;
uniform mat4 projection;

/* out vec2 TexCoord; */
out vec4 COLOR;

void main()
{
    gl_Position = projection * view * instanceMatrix * vec4(position, 1.0f);
    COLOR = color; 
}
