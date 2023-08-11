#version 430

// Global variables for lighting calculations
//layout(location = 1) uniform vec3 viewPos;

// Output for on-screen color
layout(location = 0) out vec4 outColor;

layout(location = 0) uniform mat4 mvp;
layout(location = 1) uniform vec3 viewPos;
layout(location = 2) uniform float refractionIndex;
layout(location = 3) uniform sampler2D backFaceNormal;
layout(location = 4) uniform sampler2D backFaceDepth;
layout(location = 5) uniform sampler2D EnviromentMap;
layout(location = 6) uniform sampler2D EnviromentDepth;
layout(location = 7) uniform mat4 project_inv;
layout(location = 8) uniform samplerCube envMap;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal
in vec2 fragtexCoord;

void main()
{
    vec3 V = fragPos-viewPos;

    vec4 screenPos = mvp * vec4(fragPos,1);
    screenPos.xyz = screenPos.xyz / screenPos.w;

    //They use the depth buffer and transform them back to NDC we simply store the length to viewpos in the alpha channel.
    //vec4 NDCPos = project_inv * vec4(screenPos.xyz,1);
    //NDCPos.xyz = NDCPos.xyz / NDCPos.w;

    vec3 T_1 = normalize(refract(normalize(V),normalize(fragNormal),refractionIndex/1));
    
    
    //vec4 NDCBackFace = project_inv * vec4(0,0,texture(backFaceDepth, screenPos.xy / 2 + 0.5).x,1);
    //NDCBackFace.xyz = NDCBackFace.xyz/NDCBackFace.w;
    
    float d_v =  length(fragPos-viewPos)-texture(backFaceNormal,screenPos.xy).w;
    //float d_v = NDCBackFace.z - NDCPos.z;

    //We should now calculate d_bar in the following way assuming d_normal would be precomputed:
    //Theta_i = dot(-V,fragNormal)
    //Theta_t = dot(T_1,-fragNormal)
    //d_bar = (theta_t/theta_i) * d_v + (1 - (theta_t/theta_i)) * d_n

    //However this is a lot of work and with low refractive indices and relatively convex geometry this works as well according to the paper
    float d_bar = d_v;

    //Calculate P_2
    vec3 P2 = fragPos + d_bar * T_1;
    vec4 P2_screen = mvp * vec4(P2,1);
    P2_screen.xyz = P2_screen.xyz / P2_screen.w;

    //Refract again to the outside.
    vec3 N_2 = normalize((texture(backFaceNormal, P2_screen.xy  / 2 + 0.5).xyz - 0.5) * 2); 
    vec3 T_2 =  normalize(refract(T_1,-N_2,1/refractionIndex));

    //Index the environment map with the new direction
    outColor = texture(envMap,T_2);
    //outColor = vec4(screenPos.xy,0, 1);
}