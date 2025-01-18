#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;


void main()
{
  gl_Position = lightSpaceMatrix * model * vec4(vPosition, 1.0f);
}

