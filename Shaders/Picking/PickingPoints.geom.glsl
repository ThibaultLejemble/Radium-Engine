#include "TransformStructs.glsl"

layout( points ) in;
layout(triangle_strip, max_vertices = 4) out;

in gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
}
gl_in[];

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};

layout( location = 0 ) in vec3 in_position[];
layout( location = 1 ) in vec3 in_normal[];
layout( location = 2 ) in vec3 in_eye[];

uniform Transform transform;
uniform float pointCloudSplatRadius;

layout( location = 0 ) out vec3 out_position;
layout( location = 1 ) out vec3 out_normal;
layout( location = 2 ) out vec3 out_eye;
layout( location = 3 ) flat out int out_eltID;
layout( location = 4 ) out vec3 out_eltCoords;

void main() {

    mat4 vp = transform.proj * transform.view;

    vec3 normal = in_normal[0]; // assume point is oriented

    // orthonormal basis {u, v, normal}
    vec3 u = vec3(1,0,0);
    if(abs(normal.x) > 1e-3) {
        u = normalize(vec3(-normal.z/normal.x, 0, 1));
    }
    vec3 v = normalize(cross(normal, u));

    // quad corners
    vec3 corner[4];
    corner[0] = in_position[0] - pointCloudSplatRadius*(u+v);
    corner[1] = corner[0] + pointCloudSplatRadius*u*2;
    corner[2] = corner[0] + pointCloudSplatRadius*v*2;
    corner[3] = corner[0] + pointCloudSplatRadius*(u+v)*2;

    for(uint idx = 0; idx<4; ++idx)
    {
        gl_Position   = vp * vec4(corner[idx], 1);
        out_position  = corner[idx];
        out_normal    = normal;
        out_eye       = in_eye[0];
        out_eltID     = gl_PrimitiveIDIn;
        out_eltCoords = vec3( 1, 0, 0 );
        EmitVertex();
    }
    EndPrimitive();
}

