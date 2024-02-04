#version 430

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 7, rgba32f) uniform image2D targetTexture;

uniform float smoothingFactor;

void main() {
    ivec2 computationPos = ivec2(gl_GlobalInvocationID.xy);
    vec4 mixedColor = vec4(0, 0, 0, 0);

    // Iterate over 3x3 grid
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            ivec2 neighborPos = computationPos + ivec2(i, j);
            vec4 neighborColor = imageLoad(targetTexture, ivec2(neighborPos));
            mixedColor += neighborColor;
        }
    }

    vec4 nextColor = mixedColor * (1.0 / 9.0) * (1.0 - smoothingFactor);

    // Write color to the image
    imageStore(targetTexture, computationPos, nextColor);
}
