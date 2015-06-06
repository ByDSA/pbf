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

// Presión
coherent layout( std430, binding=5 ) buffer Pressure {
   coherent float pressure[ ]; 
};

// Lambda
coherent layout( std140, binding=6 ) buffer Lambda {
   coherent float lambda[ ]; 
};

// GridCells
coherent layout( std430, binding=8 ) buffer GridCells {
   coherent uint gridCells[ ]; 
};

// Neighbors (id)
coherent layout( std430, binding=9 ) buffer Neighbors {
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
const float CRF          = 0;

const float side         = 4;
const float volume       = side*side*side;
const float inc          = side / ppedge;
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
const int ITERATIONS = 4;
const float epsilon       = 500;
const float K_PRESSURE    = 0.001;
const float K_VISCOSITY   = 0.01;
const float K_VORTICITY   = 0.004;
const int   K_N           = 4;
const float K_Q           = 0.3*h;

const vec3 K_GRAVITY = vec3( 0.0, -9.8, 0.0 );

uint grid_get(const vec3 pos) {
    vec3 c = floor((pos - LIM_INF) / h);

    uint cell_id = uint((c.x*CELL.y + c.y)*CELL.z + c.z) % CELL_NUMBER;

    return cell_id;
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
        return vec3(0, 0, 0);

    float m = h - r;
    float mult = K_GRADIENT_SPIKY / r * m * m;
    return dr * mult;
}

void main() {
    uint i = gl_GlobalInvocationID.x;

    uint m1 = (MAX_PART_NEIGHBORS+1)*i;
    const uint N = neighbors[m1];
    const uint OFFSET = m1+1;

    vec3 v_i   = velocity[i].xyz;
    const vec3 p_i = position[i].xyz;

    // Cálculo de v^new (viscosidad) y omega (vorticity)
	vec3 v = vec3(0);
	vec3 omega = vec3(0);
    for(uint n = 0; n < N; n++) {
        const uint j = neighbors[OFFSET+n];

        const vec3 v_j = velocity[j].xyz;
        vec3 v_ij = v_j - v_i;
        const vec3 p_j = position[j].xyz;

		vec3 dr = p_i - p_j;
        float dr2 = dot(dr, dr);
        float tmp = W_poly6(dr2);

        v += v_ij * tmp;

        vec3 grad = gradientSpiky(dr);
        omega += cross(v_ij, grad);
    }

    v_i += v * K_VISCOSITY;

    vorticity[i].xyz = omega;
}
