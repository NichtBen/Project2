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

//how tight agends sample infron of them
uniform float steeringangle;
//how far the variation away from target is
uniform float randomangle;
//passed seed for making random numbers
uniform uint randseed;
uint currentrandindex = 0;

const float PI = 3.14159265358979323846;

uniform float moveSpeed;

//current data in x texture
vec2 currentxy;
//current data in angle texture
float currentAngle;


//computationPos of thread on data texture
ivec2 computationPos;



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

float uintFloat(uint a)
{
 return a / 2147483647.0;
}

uint getRandomNumber()
{
    return hash(uint(randseed+ computationPos[0] * computationPos[1])+currentrandindex);
    currentrandindex++;
}


float clockwiseAngle(vec2 vector) {
    // Avoid division by zero
    if (vector.x == 0.0 && vector.y == 0.0) {
        return 0.0;  // undefined angle, return a default value
    }

    // Calculate the dot product with the reference vector (1, 0)
    float dotProduct = dot(normalize(vector), vec2(1.0, 0.0));

    // Use acos to find the angle in radians
    float angle = acos(dotProduct);

    // Check the sign of the y-component to determine the quadrant
    if (vector.y < 0.0) {
        angle = 2.0 * 3.14159265358979323846 - angle;  // Adjust for the lower half of the unit circle
    }

    return angle;
}




void updateAgend() {
    // Calculate the new position based on the original currentxy
    vec2 direction = vec2(cos(currentAngle), sin(currentAngle));
    vec2 newPos = currentxy + direction * moveSpeed ;

    float newAngle = currentAngle;
    if(newPos[0] < 0 || newPos[0] > worldWidth )
    {
        newPos = currentxy + direction * -1 * moveSpeed;
        direction[0] *= -1;
        newAngle = clockwiseAngle(direction);
    } 
        else if(newPos[1] < 0 || newPos[1] > worldHeight )
    {
            newPos = currentxy + direction * -1 * moveSpeed;
            direction[1] *= -1;
            newAngle = clockwiseAngle(direction);
    }
    
    //Sample 3 pixel infront, turn based on info, + little randomness
    
    int sensorReach = 3;

    vec2 infront = newPos + direction * sensorReach;

    mat2 rotationMatrix = mat2(cos(steeringangle), -sin(steeringangle), sin(steeringangle), cos(steeringangle));
    vec2 rotatedVector = rotationMatrix * direction*sensorReach;
    vec2 left = newPos + rotatedVector ;

    mat2 rotationMatrix2 = mat2(cos(-steeringangle), -sin(-steeringangle), sin(-steeringangle), cos(-steeringangle));
    vec2 rotatedVector2 = rotationMatrix2 * direction *sensorReach;
    vec2 right = newPos + rotatedVector2;

    vec4 leftColor = imageLoad(Result, ivec2(left));
    vec4 rightColor = imageLoad(Result, ivec2(right));
    vec4 infrontColor = imageLoad(Result, ivec2(infront));

    float rand =(0.5- (uintFloat(getRandomNumber()) )) * randomangle;
    

   if(leftColor[0] > rightColor[0] ){
        if(leftColor[0] > infrontColor[0])
        {
            newAngle = mod(currentAngle - steeringangle,2*PI);
        } else {
            
        }
   } 
   else if(rightColor[0] > infrontColor[0])
   {
            newAngle = mod(currentAngle + steeringangle,2*PI);
   }  
   else if (infrontColor[0] > rightColor[0]) 
   {
            
   } 
   else
   {
   }




    // Write nextpos to output
    imageStore(nextagenddata_angle, computationPos, vec4(newAngle,0.0f, 0.0f, 0.0f));
    imageStore(nextagenddata_xy, computationPos, vec4(newPos, 0, 0));
        

    // Update currentxy after writing to the output texture
    currentxy = newPos;
}
 
void main() {
if(moveSpeed == 0) return;
    computationPos = ivec2(gl_GlobalInvocationID.xy);


    
    
    //current data in xy texture
    vec4 temp = imageLoad(currentagenddata_xy, computationPos);
    currentxy = vec2(temp[0],temp[1]);

    //current color on result texture
    vec4 currentColor = imageLoad(Result, ivec2(currentxy));

    //current data in angle texture
    temp = imageLoad(currentagenddata_angle, computationPos);
    currentAngle = temp[0];




    updateAgend();



     ivec2 newPos = ivec2 (currentxy);
    

    // Set the next color to white
    vec4 nextColor =   currentColor +  vec4(1, 0, 0, 0)*0.5 ;

    nextColor[2] = (nextColor[0]/10);
    nextColor[1] = (nextColor[0]/100);
    nextColor[3] = (nextColor[0]/1000);



    
    // Write the next color to the image
    imageStore(Result, newPos, nextColor);
}
