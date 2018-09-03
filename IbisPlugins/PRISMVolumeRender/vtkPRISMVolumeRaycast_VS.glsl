#version 150

in vec4 vertex;
uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

void main(void)
{
    gl_Position = projectionMatrix * modelViewMatrix * vertex;
}
