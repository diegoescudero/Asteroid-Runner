varying vec3 vLightVector;
varying vec3 vNormal;
varying vec3 vEye;

uniform vec3 uColor;

void main()
{
    vec4 finishedColor = (gl_FrontLightModelProduct.sceneColor * gl_FrontMaterial.ambient) + 
    (gl_LightSource[0].ambient * gl_FrontMaterial.ambient);
    
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vLightVector);
    
    float diffuse = dot(N, L);
    
    if (diffuse > 0.0) {
        finishedColor += vec4(uColor, 0.0) * diffuse;
        
        vec3 E = normalize(vEye);
        vec3 R = reflect(-L, N);
        
        //float specular = pow(max(dot(R, E), 0.0), 500.0);
        //finishedColor += vec4(uColor, 0.0) * specular;
    }
    
    gl_FragColor = finishedColor;
}