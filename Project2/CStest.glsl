#version 430

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, rgba32f) uniform image2D Result;

uniform float time;  // Time variable for animation

void main() {
    ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);

    // Calculate color based on time
    vec3 color = vec3(sin(time), cos(time), 0.5);

    // Write color to the image
    imageStore(Result, storePos, vec4(color, 1.0));
}
