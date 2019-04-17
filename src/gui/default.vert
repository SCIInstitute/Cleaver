attribute vec4 gl_Color;

varying vec4 gl_FrontColor; // writable on the vertex shader
varying vec3 normal;

void main()
{
    /* first transform the normal into eye space and normalize the result */
    normal = normalize(gl_Normal);
    gl_FrontColor = gl_Color;
    gl_Position = ftransform();
}
