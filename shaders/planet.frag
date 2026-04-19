#version 330 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 lightPos;      // Sun position in world space
uniform vec3 viewPos;       // Camera position in world space
uniform vec3 objectColor;   // Planet diffuse colour
uniform vec3 lightColor;    // Sunlight colour (typically white)

void main()
{
    // --- Ambient ---
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;

    // --- Diffuse ---
    vec3 norm      = normalize(Normal);
    vec3 lightDir  = normalize(lightPos - FragPos);
    float diff     = max(dot(norm, lightDir), 0.0);
    vec3 diffuse   = diff * lightColor;

    // --- Specular (Blinn-Phong) ---
    float specularStrength = 0.4;
    vec3 viewDir   = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec     = pow(max(dot(norm, halfwayDir), 0.0), 64.0);
    vec3 specular  = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor   = vec4(result, 1.0);
}
