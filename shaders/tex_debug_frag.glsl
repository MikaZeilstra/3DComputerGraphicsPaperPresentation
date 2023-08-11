#version 430

// Global variables for lighting calculations
//layout(location = 1) uniform vec3 viewPos;

// Output for on-screen color
layout(location = 0) out vec4 outColor;

layout(location = 1) uniform sampler2D cubeMap;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal
in vec2 fragtexCoord;

void main()
{

   outColor = texture(cubeMap,fragtexCoord);
}