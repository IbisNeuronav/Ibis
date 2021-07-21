#version 150

uniform mat4 projectionMatrixInverse;
uniform ivec2 windowSize;

in vec3 color;

out vec4 fragColor;

void main()
{
    fragColor = vec4( color, 1.0 );
    vec4 ndcPos;
    ndcPos.xy = ( (gl_FragCoord.xy / vec2(windowSize) ) * 2.0) - 1.0;
    ndcPos.z = (2.0 * gl_FragCoord.z - gl_DepthRange.near - gl_DepthRange.far) / gl_DepthRange.diff;
    ndcPos.w = 1.0;
    vec4 clipPos = ndcPos / gl_FragCoord.w;
    vec4 eyeSpaceCoord = projectionMatrixInverse * clipPos;
    fragColor.a =  -eyeSpaceCoord.z;
}
