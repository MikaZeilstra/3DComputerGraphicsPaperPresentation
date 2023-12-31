#version 430

// Global variables for lighting calculations
//layout(location = 1) uniform vec3 viewPos;

// Output for on-screen color

layout(location = 0) out vec4 outColor;
layout(location = 0) uniform mat4 mvp;
layout(location = 1) uniform vec3 viewPos;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    // Output the normal as color

    outColor = vec4(normalize(fragNormal.xyz)/2 + 0.5, length(viewPos - fragPos));
}