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

// Radio kernel
const float h  = pow( (3*volume*kernel_part) / ( 4 * PI * numPart ) , 1.0f/3);
const float h2 = h * h;
const float h3 = h2 * h;
const float h6 = h2 * h2 * h2;
const float h9 = h3 * h3 * h3;

////////////////////////////////////////////////////////////////////////////////
//
// Parámetros PBF
//
////////////////////////////////////////////////////////////////////////////////
const float K_VORTICITY   = 0.004;

// Constantes
const float K_GRADIENT_SPIKY =  -45 / (PI * h6);

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
    const uint NN = neighbors[m1];
    const uint OFFSET = m1+1;

    vec3 v_i   = velocity[i].xyz;
    const vec3 p_i = position[i].xyz;

    // Cálculo de η
    vec3 grad_sum = vec3(0, 0, 0);
	for(uint n = 0; n < NN; n++) {
        const uint j = neighbors[OFFSET+n];

        const vec3 p_j = position[j].xyz;
		vec3 dr = p_i - p_j;
        vec3 grad = gradientSpiky(dr);
        float omegaSq = length(vorticity[j].xyz);
		grad_sum += grad * omegaSq;
	}
	
    vec3 N;
	float grad_sum_mag = length(grad_sum);
	if (grad_sum_mag < UMBRAL) // Sólo se aplica vorticity 'si existe'
        return;
	
    N = grad_sum / grad_sum_mag; // Normalizar grad_sum
	
	// Aplicar fuerza vorticity
    vec3 x = cross (N, vorticity[i].xyz);
	v_i += x * dt * K_VORTICITY;
	
	// -> Global Memory
    velocity[i].xyz = v_i;
}
