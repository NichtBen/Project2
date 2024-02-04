#version 430


// Workgroup size
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Texture bindings
layout(binding = 0, rgba32f) uniform image2D currentagenddata_xy;
layout(binding = 2, rgba32f) uniform image2D currentagenddata_angle;
layout(binding = 3, rgba32f) uniform image2D nextagenddata_xy;
layout(binding = 5, rgba32f) uniform image2D nextagenddata_angle;
layout(binding = 6, rgba32f) uniform image2D Start;
layout(binding = 7, rgba32f) uniform image2D Result;

// Time variable for animation
uniform float deltaTime;



// Computation variables
uniform uint worldWidth;
uniform uint worldHeight;

const float PI = 3.14159265358979323846;

float moveSpeed = 1.0f;

//current data in x texture
vec2 currentxy;
//current data in angle texture
float currentAngle;


//computationPos of thread on data texture
ivec2 computationPos;


float clockwiseAngle(vec2 vector) {
    // The vector (1, 0) is the positive x-axis
    vec2 xAxis = vec2(1.0, 0.0);

    // Calculate the dot product between the given vector and the positive x-axis
    float dotProduct = dot(normalize(vector), xAxis);

    // Use arccosine to find the angle in radians
    float angle = acos(dotProduct);

    // If the y-component of the vector is negative, it's in the clockwise direction
    if (vector.y < 0.0) {
        angle = 2.0 * 3.14159265358979323846 - angle;
    }

    return angle;
}



// Simple hash function
uint hash(uint state) {
    state ^= 2747636419u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    return state;
}

// Float hash function
float hashIntIoFloat(int state) {
    return hash(state) / 2147483647.0;
}


void updateAgend() {
    // Calculate the new position based on the original currentxy
    uint random = hash(uint(currentxy[0] * currentxy[1] * currentAngle));

    vec2 direction = vec2(cos(currentAngle), sin(currentAngle));
    vec2 newPos = currentxy + direction * moveSpeed;
    float newAngle=0;
    if(newPos[0] < 0 || newPos[0] > worldWidth   )
    {
        direction[0] *= -1;
        newPos = currentxy;
        newAngle = clockwiseAngle(direction);

    } else if(newPos[1] < 0 )//|| newPos[1] > height )
    {
        direction[1] *= -1;
        newPos = currentxy;
        newAngle = clockwiseAngle(direction);
    }

    if(worldWidth == 0)
    newPos[0] += -1;

    // Write nextpos to output
    imageStore(nextagenddata_angle, computationPos, vec4(newAngle,0.0f, 0.0f, 0.0f));
    imageStore(nextagenddata_xy, computationPos, vec4(newPos, 0, 0));
        

    // Update currentxy after writing to the output texture
    currentxy = newPos;
}
 
void main() {
    
    computationPos = ivec2(gl_GlobalInvocationID.xy);
    
    //current data in xy texture
    vec4 temp = imageLoad(currentagenddata_xy, computationPos);
    currentxy = vec2(temp[0],temp[1]);
    //current data in angle texture
    temp = imageLoad(currentagenddata_angle, computationPos);
    currentAngle = temp[0];
    

    updateAgend();



     ivec2 newPos = ivec2 (currentxy);
    

    // Set the next color to white
    vec4 nextColor = vec4(1, 0, 0, 1);
    
    // Write the next color to the image
    imageStore(Result, newPos, nextColor);
}
