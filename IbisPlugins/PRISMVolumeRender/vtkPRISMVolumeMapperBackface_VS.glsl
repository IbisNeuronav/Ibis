#version 150

in vec4 vertex;
in vec3 in_color;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

out vec3 color;

void main(void)
{
    gl_Position = projectionMatrix * modelViewMatrix * vertex;
    color = in_color;
}
