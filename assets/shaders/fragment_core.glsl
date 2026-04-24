#version 330 core


uniform sampler2D planetTexture;
uniform sampler2D nightTexture;
uniform sampler2D specularMap;

uniform vec3 pColor;
uniform bool useTexture;
uniform bool isCloud;
uniform bool hasNightTexture;
uniform bool hasSpecularMap;

uniform vec3 lightPos; //position of light source
uniform vec3 viewPos; //position of camera
uniform bool isSun; //is current object being rendered the light source?
uniform bool isCorona; // glow toggle

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

void main(){
        //corona glow rendering
    if (isCorona) {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);
        
            // Intensity is 1.0 at the center, 0.0 at the grazing edges
        float intensity = max(dot(norm, viewDir), 0.0);
        
            // Cube the intensity so the glow fades smoothly but stays tight to the sun
        float alpha = pow(intensity, 3.0);
        
            // Multiply alpha by 0.6 so the center isn't 100% solid, allowing the Sun texture to peek through!
        FragColor = vec4(pColor, alpha * 0.6);
        return;
    }
        //normal rendering
    vec4 texData = texture(planetTexture, TexCoords);
    vec3 objectColor = texData.rgb;
    float alpha = texData.a;
    
    if (useTexture) {
        vec4 texData = texture(planetTexture, TexCoords);
        objectColor = texData.rgb;
        alpha = texData.a;
        
            //cloud hack
        if (isCloud) {
                // Use the redness/brightness of the image as the transparency!
                // Black background = 0.0 (Invisible). White clouds = 1.0 (Solid).
            alpha = texData.r; 
            objectColor = vec3(1.0); // Force the clouds to be pure white
        }
    }else{
        objectColor = pColor;
    }

    if(isSun){
        FragColor = vec4(objectColor * 5.0, alpha);
        return;
    }
    
        //ambient lighting
    float ambientStrength = 0.10;
    vec3 ambient = ambientStrength * vec3(1.0);

        //diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float nDotL = dot(norm, lightDir);
    float diff = max(nDotL, 0.0);
    vec3 diffuse = diff * vec3(1.0);

        //specular lighting
    float specularStrength = 0.2;
    if (hasSpecularMap && !isCloud) {
            // We only need the 'r' (red) channel because the image is black and white
        float specMask = texture(specularMap, TexCoords).r; 
        
            // Multiply our strength by the mask. 
            // Oceans (1.0) stay 0.2 shiny. Land (0.0) becomes 0.0 shiny!
        specularStrength *= specMask; 
    }
    if (isCloud) { specularStrength = 0.0; }
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = specularStrength * spec * vec3(1.0);
        // Note: The Moon shouldn't be very shiny, but Earth's oceans should! We keep it simple for now.

    vec3 result = (ambient + diffuse + specular) * objectColor;
    if (hasNightTexture && !isCloud) {
        vec3 nightColor = texture(nightTexture, TexCoords).rgb;
        
            // smoothstep creates a soft twilight gradient at the terminator line.
            // When nDotL is 0.1 (twilight), blend starts. When -0.2 (night), blend is 1.0.
        float nightBlend = smoothstep(0.1, -0.2, nDotL); 
        
            // Add the glowing city lights on top of the dark surface!
        result += nightColor * nightBlend;
    }
    FragColor = vec4(result, alpha);
}