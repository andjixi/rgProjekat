#version 330 core
out vec4 FragColor;

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;

    float shininess;
};

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform PointLight lampPointLight1;
uniform PointLight lampPointLight2;
uniform SpotLight lampSpotLight;
uniform DirLight dirLight;
uniform vec3 viewPosition;
uniform sampler2D texture1;
uniform Material material;

vec4 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    //blending
    vec4 texColor = vec4(texture(texture1, TexCoords));

    vec4 ambient = (light.ambient, 1.0) * texColor * vec4(vec3(texture(material.texture_diffuse1, TexCoords)), 1.0);
    vec4 diffuse = (light.diffuse, 1.0) * diff * texColor * vec4(vec3(texture(material.texture_diffuse1, TexCoords)), 1.0);
    vec4 specular = (light.specular, 1.0) * spec * texColor * vec4(vec3(texture(material.texture_diffuse1, TexCoords).xxx), 1.0);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec4 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec4 texColor = vec4(texture(texture1, TexCoords));

    vec4 ambient = (light.ambient, 1.0) * texColor * vec4(vec3(texture(material.texture_diffuse1, TexCoords)), 1.0);
    vec4 diffuse = (light.diffuse, 1.0) * diff * texColor * vec4(vec3(texture(material.texture_diffuse1, TexCoords)), 1.0);
    vec4 specular = (light.specular, 1.0) * spec * texColor * vec4(vec3(texture(material.texture_diffuse1, TexCoords)), 1.0);
    return (ambient + diffuse + specular);
}

vec4 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
     vec3 lightDir = normalize(light.position - fragPos);

     // diffuse shading
     float diff = max(dot(normal, lightDir), 0.0);

     // specular shading
     vec3 halfwayDir = normalize(lightDir + viewDir);
     float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

     //vec3 reflectDir = reflect(-lightDir, normal);
     //float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

     // attenuation
     float distance = length(light.position - fragPos);
     float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

     // spotlight intensity
     float theta = dot(lightDir, normalize(-light.direction));
     float epsilon = light.cutOff - light.outerCutOff;
     float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

     //blending
     vec4 texColor = vec4(texture(texture1, TexCoords));

     // combine results
     vec4 ambient = (light.ambient, 1.0) * texColor * vec4(vec3(texture(material.texture_diffuse1, TexCoords)), 1.0);
     vec4 diffuse = (light.diffuse, 1.0) * diff * texColor * vec4(vec3(texture(material.texture_diffuse1, TexCoords)), 1.0);
     vec4 specular = (light.specular, 1.0) * spec * texColor * vec4(vec3(texture(material.texture_diffuse1, TexCoords)), 1.0);
     ambient *= attenuation * intensity;
     diffuse *= attenuation * intensity;
     specular *= attenuation * intensity;
     return (ambient + diffuse + specular);
}

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);
    vec4 result =     CalcDirLight(dirLight, normal, viewDir)
                    + CalcPointLight(lampPointLight1, normal, FragPos, viewDir)
                    + CalcPointLight(lampPointLight2, normal, FragPos, viewDir)
                    + CalcSpotLight(lampSpotLight, normal, FragPos, viewDir);
    FragColor = result;
}