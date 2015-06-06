#version 430

// Posición
coherent layout( std430, binding=0 ) buffer Pos {
   coherent vec4 position[ ]; 
};

// Velocidad
coherent layout( std430, binding=1 ) buffer Vel {
   coherent vec4 velocity[ ]; 
};

// Posición predicha
coherent layout( std430, binding=3 ) buffer PosPred {
   coherent vec4 position_pred[ ]; 
};

layout( local_size_x = 100, local_size_y = 1, local_size_z = 1 ) in;



float dt                 = 0.01;

// Recipiente
const vec3 LIM_INF = vec3(0);
const vec3 LIM_SUP = vec3(6);

void main() {
	uint i = gl_GlobalInvocationID.x;

    vec3 pos_pred = position_pred[i].xyz;
    
    // Integración numérica
    velocity[i].xyz = (pos_pred - position[i].xyz) / dt;

    pos_pred = clamp(pos_pred, LIM_INF, LIM_SUP);

    // Posición = Posición predicha
    position[i].xyz = pos_pred;
}
