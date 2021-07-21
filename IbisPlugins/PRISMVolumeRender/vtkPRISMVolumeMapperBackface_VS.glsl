#version 150

in vec3 in_vertex;
in vec3 in_color;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 volumeMatrix;

out vec3 color;

void main(void)
{
    gl_Position = projectionMatrix * modelViewMatrix * volumeMatrix * vec4(in_vertex,1.0);
    color = in_color;
}
