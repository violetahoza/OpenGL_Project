#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;
//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;

// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

uniform vec3 fogColor = vec3(0.5, 0.5, 0.5);
uniform float fogDensity;

uniform int isNight;

uniform sampler2D shadowMap;

//components
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float constant = 1.0f;
float linear = 0.0014f;
float quadratic = 0.000007f;

//point lighting
uniform vec3 pointLightPos;
uniform vec3 pointLightColor;
uniform vec3 pointLightPos1;
uniform vec3 pointLightColor1;
uniform vec3 pointLightPos2;
uniform vec3 pointLightColor2;
uniform vec3 pointLightPos3;
uniform vec3 pointLightColor3;

void computeDirLight()
{
    //compute eye space coordinates
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye;

    if(gl_FrontFacing){
        normalEye = normalize(normalMatrix * fNormal);
    }else{
        normalEye = -normalize(normalMatrix * fNormal);
    }

    //normalize light direction
    vec3 lightDirN = normalize(lightDir - fPosEye.xyz);

    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(- fPosEye.xyz);

    float dist = length(lightDir - fPosEye.xyz);
    float att = 1.0f / (constant + linear * dist + quadratic * (dist * dist));

    //compute ambient light
    ambient = att * ambientStrength * lightColor;

    //compute diffuse light
    diffuse = att * max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    

    //compute specular light
    //vec3 reflectDir = reflect(-lightDirN, normalEye);
    vec3 halfVector = normalize(lightDirN + viewDir);
    float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), 32);
    specular = att * specularStrength * specCoeff * lightColor;
}

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


float computeFog()
{
 vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
 float fragmentDistance = length(fPosEye.xyz);
 float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
 return clamp(fogFactor, 0.0f, 1.0f);
}

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

void main() 
{
    float shadow = 0.0f; 
    if(isNight == 0) {
    	computeDirLight();
        shadow = computeShadow();
    } 

    computePointLight(pointLightPos, pointLightColor);
    computePointLight(pointLightPos1, pointLightColor1);
    computePointLight(pointLightPos2, pointLightColor2);
    computePointLight(pointLightPos3, pointLightColor3);

    float fog = computeFog();    
    vec4 finalFogColor = vec4(fogColor, 1.0f);

    vec4 textureColor = abs(texture(diffuseTexture, fTexCoords));

    if(textureColor.a < 0.1) // discard fragments with a low alpha value
        discard;

    ambient *= texture(diffuseTexture, fTexCoords);
	diffuse *= texture(diffuseTexture, fTexCoords);

    //compute final vertex color
    //vec3 color = min((ambient.rgb + diffuse.rgb) * textureColor.rgb + specular * texture(specularTexture, fTexCoords).rgb, 1.0f);

    vec3 color = min((ambient + (1.0 - shadow) * diffuse) * texture(diffuseTexture, fTexCoords).rgb + (1.0 - shadow) * specular * texture(specularTexture, fTexCoords).rgb, 1.0f);

    //fColor = vec4(color, 1.0f);
    fColor = mix(finalFogColor, vec4(color, 1.0f), fog);
}
