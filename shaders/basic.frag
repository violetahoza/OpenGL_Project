#version 410 core

// Input variables from the vertex shader
in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;
in vec4 fPosEye;
//in vec3 fragPos;

// Output color of the fragment
out vec4 fColor;

// Uniform variables for transformation matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
uniform mat3 lightDirMatrix;

// Uniform variables for lighting
uniform vec3 lightDir;
uniform vec3 lightColor;

// Uniform variables for point lighting
uniform vec3 pointLightPos;
uniform vec3 pointLightColor;
uniform vec3 pointLightPos1;
uniform vec3 pointLightColor1;
uniform vec3 pointLightPos2;
uniform vec3 pointLightColor2;
uniform vec3 pointLightPos3;
uniform vec3 pointLightColor3;

// Uniform variables for textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

// Uniform variables for fog
uniform vec3 fogColor = vec3(0.5, 0.5, 0.5);
uniform float fogDensity;

// Uniform variable to check if it is night
uniform int isNight;

// Uniform variable for shadow mapping
uniform sampler2D shadowMap;

// Components for lighting calculations
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.1f;

//alpha for transparency
uniform float alpha;

// Function to compute point light contribution
void computePointLight(vec3 pointLightPos, vec3 pointLightColor)
{
    float constant = 1.0f;
    float linear = 0.35f;
    float quadratic = 0.44f;

    vec3 normalEye = normalize(normalMatrix * fNormal);
    

    vec3 lightDir2 = normalize(pointLightPos - fPosition);

    vec3 halfVector = normalize(lightDir2 + lightDir2);
    float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), 32);

    float distance2 = length(pointLightPos - fPosition.xyz);
    float attenuation2 = 1.0 / (constant + linear * distance2 + quadratic * (distance2 * distance2));

    ambient += ambientStrength * pointLightColor * attenuation2;

    diffuse += max(dot(normalEye, lightDir2), 0.0f) * pointLightColor * attenuation2;
    specular += specularStrength * pow(max(dot(normalEye, reflect(-lightDir2, normalEye)), 0.0f), 32) * pointLightColor * attenuation2;
}

// Function to compute shadow contribution
float computeShadow()
{
	// perform perspective divide
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// Transform to [0,1] range
	normalizedCoords = normalizedCoords * 0.5 + 0.5;
	if (normalizedCoords.z > 1.0f)
		return 0.0f;
	// Get closest depth value from light's perspective
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
	// Get depth of current fragment from light's perspective
	float currentDepth = normalizedCoords.z;
	// Check whether current frag pos is in shadow
	float bias = 0.005f;
	float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;
	return shadow;
}

// Function to compute directional light contribution
void computeDirLight()
{
    //vec3 cameraPosEye = vec3(0.0f);
    //compute eye space coordinates
    //vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    //normalize light direction
    //vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));
    vec3 lightDirN = normalize(lightDir);  

    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(- fPosEye.xyz);

    //compute half vector
    vec3 halfVector = normalize(lightDirN + viewDir);

    //compute ambient light
    ambient = ambientStrength * lightColor;
    ambient *= texture(diffuseTexture, fTexCoords);

    //compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
    diffuse *= texture(diffuseTexture, fTexCoords);

    //compute specular light
    float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), 32);
    specular = specularStrength * specCoeff * lightColor;
    specular *= texture(specularTexture, fTexCoords);
}

// Function to compute fog contribution
float computeFog() {
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
	float fragDist  = length(fPosEye.xyz);
	float factor = exp(-pow(fragDist * fogDensity, 2));
	return clamp(factor, 0.0f, 1.0f);
}

void main() 
{
    float shadow = 0.0f;
    // Compute directional light if it is not night
    if(isNight == 0) {
       computeDirLight();
       shadow = computeShadow();
    } 

    // Compute point light contributions
    computePointLight(pointLightPos, pointLightColor);
    computePointLight(pointLightPos1, pointLightColor1);
    computePointLight(pointLightPos2, pointLightColor2);
    computePointLight(pointLightPos3, pointLightColor3);
    
    // Compute fog contributions
    float fog = computeFog();    

    vec4 finalFogColor = vec4(fogColor, 1.0f);

    vec4 textureColor = abs(texture(diffuseTexture, fTexCoords));

    if(textureColor.a < 0.1) // discard fragments with a low alpha value
        discard;

    // Compute final vertex color
    vec3 color = min((ambient + (1.0 - shadow) * diffuse) * texture(diffuseTexture, fTexCoords).rgb + (1.0 - shadow) * specular * texture(specularTexture, fTexCoords).rgb, 1.0f);
    fColor = mix(finalFogColor, vec4(color, 1.0f), fog);
}
