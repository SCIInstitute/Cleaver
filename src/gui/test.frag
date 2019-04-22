varying vec4 gl_Color;
varying vec3 normal;

void main()
{
    vec3 up = normalize(vec3(0.0, 0.0, 1.0));

    vec3 light1 = normalize(vec3(0.5, 0.75, 0.5));
    vec3 light2 = normalize(vec3(0.2, 0.5, 0.75));

    float NdotL1 = abs(dot(light1, normal));
    float NdotL2 = abs(dot(light2, normal));
    float l = NdotL1 + NdotL2;
    float a = 0.2;
    float c = clamp(l + a, 0.0, 1.0);
    gl_FragColor = gl_Color;
}
