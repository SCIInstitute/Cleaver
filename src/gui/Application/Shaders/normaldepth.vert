  attribute vec3 position, normal;
    varying vec3 v_normal, v_position;
    uniform mat4 proj, view;

    void main(void) {
        //gl_Position = proj * view * vec4(position, 1.0);
        gl_Position = ftransform();
        v_normal = normal;
        v_position = position;
    }
