#version 430

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 1, rgba32f) uniform image2D Start;
layout(binding = 0, rgba32f) uniform image2D Result;

uniform float time;  // Time variable for animation

vec3 x = vec3(1,0,0);
vec3 y = vec3(0,1,0);


void main() {
    ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);

    int neighbours = 0;
    float addedAngles =0;

    vec4 currentColor = imageLoad(Start, storePos);

    //iterate over 3x3 grit 
    for(int i = -1; i <= 1; i++){
        for(int j = -1; j <= 1 && (i != 0 || j != 0);j++){
            ivec2 neighborPos = storePos + ivec2(i, j);
            vec4 neighbourColor = imageLoad(Start, neighborPos);
            

            if(neighbourColor[0] == 1){
            neighbours++;
            }
        }    
    }
    
    vec4 nextColor = vec4(0,0,0,1);

    //if dead cell
    if(currentColor[0] < 1){
        if(neighbours == 3){
            //birth new cell
            nextColor = vec4(1,0,0,1);
        }
    }else {
        if(neighbours == 2 || neighbours == 3){
            nextColor = vec4(1,0,0,1);
        }
    }
    // Calculate color based on time      -old for now
    //vec3 color = vec3(sin(time), cos(time), 0.5);

    
    // Write color to the image
    imageStore(Result, storePos, nextColor);
}
