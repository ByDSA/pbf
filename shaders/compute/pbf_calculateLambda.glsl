#version 430

// Posición
coherent layout( std430, binding=0 ) buffer Pos {
   coherent vec4 position[ ]; 
};

// Velocidad
coherent layout( std430, binding=1 ) buffer Vel {
   coherent vec4 velocity[ ]; 
};

// Velocidad
coherent layout( std430, binding=2 ) buffer Color {
   coherent vec4 color[ ]; 
};

// Posición predicha
coherent layout( std430, binding=3 ) buffer PosPred {
   coherent vec4 position_pred[ ]; 
};

// Vorticity
coherent layout( std430, binding=4 ) buffer Vorticity {
   coherent vec4 vorticity[ ]; 
};

// Lambda
coherent layout( std140, binding=5 ) buffer Lambda {
   coherent float lambda[ ]; 
};

// GridCells
coherent layout( std430, binding=7 ) buffer GridCells {
   coherent uint gridCells[ ]; 
};

// Neighbors (id)
coherent layout( std430, binding=8 ) buffer Neighbors {
   coherent uint neighbors[ ]; 
};

layout( local_size_x = 100, local_size_y = 1, local_size_z = 1 ) in;


const float PI = 3.14159265359f;
////////////////////////////////////////////////////////////////////////////////
//
// Parámetros SPH
//
////////////////////////////////////////////////////////////////////////////////
const int MAX_PART_PER_CELL = 20;                      // Máximo número de partículas por celda
const int CELL_NEIGHBORS     = 27;                     // Celdas vecinas (incluyendo propia)
const int MAX_PART_NEIGHBORS = CELL_NEIGHBORS*MAX_PART_PER_CELL; // Máximo número de vecinos por celda

float dt                 = 0.01;
const float density_rest = 998.29f;
const int ppedge    = 30;
const float kernel_part  = 20;
const float UMBRAL       = 1e-5f;

const float side         = 4;
const float volume       = side*side*side;
const int numPart        = ppedge*ppedge*ppedge;
const float mass         = density_rest * volume / numPart;

// Radio kernel
const float h  = pow( (3*volume*kernel_part) / ( 4 * PI * numPart ) , 1.0f/3);
const float h2 = h * h;
const float h3 = h2 * h;
const float h6 = h2 * h2 * h2;
const float h9 = h3 * h3 * h3;

// Recipiente
const vec3 LIM_INF = vec3(0);
const vec3 LIM_SUP = vec3(6);

// Cantidad de celdas
const vec3 CELL  = floor( (LIM_SUP - LIM_INF) / h ) + vec3(1);
const uint CELL_NUMBER = uint(CELL.x * CELL.y * CELL.z);

////////////////////////////////////////////////////////////////////////////////
//
// Parámetros PBF
//
////////////////////////////////////////////////////////////////////////////////
const float epsilon       = 500;

const vec3 K_GRAVITY = vec3( 0.0, -9.8, 0.0 );

uint get_id() {
    return uint(gl_GlobalInvocationID.x);
}


// Constantes
const float K_POLY6 = 315 / (64 * PI * h9);
const float K_GRADIENT_SPIKY =  -45 / (PI * h6);

float W_poly6(float r2) {
    float m = h2 - r2;
    return K_POLY6 * m * m * m;
}

vec3 gradientSpiky(vec3 dr) {
    float r = length(dr);

    if (r < UMBRAL)
        return vec3(0);

    float m = h - r;
    float mult = K_GRADIENT_SPIKY / r * m * m;
    return dr * mult;
}

// Restricción
// la densidad de cada partícula debe ser la densidad base
float constraint(const float density) {
	return density / density_rest - 1.0f;
}

void main() {
    uint i = get_id();
    uint m1 = (MAX_PART_NEIGHBORS+1)*i;
    const uint N = neighbors[m1];
    const uint OFFSET = m1+1;

    const vec3 pos_i = position_pred[i].xyz;

    float den = 0;
    float all_sum = 0;
    vec3 gradient_sum = vec3(0);

	for(uint k = 0; k < N; k++) {
        uint j = neighbors[OFFSET+k];

        const vec3 pos_j = position_pred[j].xyz;
        vec3 dr = pos_i - pos_j;
        float dr2 = dot(dr, dr);

        // Densidad
        den += mass * W_poly6(dr2);

        // Gradiente constraint
        vec3 grad = gradientSpiky(dr) / density_rest;
        all_sum += dot(grad, grad);

        gradient_sum += grad;     
    }

    all_sum += dot(gradient_sum, gradient_sum);
    
	lambda[i] = -constraint(den) / (all_sum + epsilon);

	lambda[i] = clamp(lambda[i], -0.0005, 0.0005);
}
