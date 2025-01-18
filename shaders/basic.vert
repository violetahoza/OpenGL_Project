#version 410 core

// Input vertex attributes
layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

// Output variables to the fragment shader
out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;
out vec4 fragPosLightSpace;
//out vec3 fragPos;
out vec4 fPosEye;

// Uniform variables for transformation matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main() 
{
    // Calculate the final position of the vertex in clip space
	gl_Position = projection * view * model * vec4(vPosition, 1.0f);

	// Pass the vertex position, vertex normal and texture coordinates to the fragment shader
	fPosition = vPosition;
	fNormal = vNormal;
	fTexCoords = vTexCoords;

	// Calculate the position in light space for shadow mapping
	fragPosLightSpace = lightSpaceMatrix * model * vec4(vPosition, 1.0f);

	// Calculate the position in eye space for the lighting calculations
	fPosEye = view * model * vec4(vPosition, 1.0f);
	//fragPos = vec3(model* vec4(vPosition,1.0f));
}
