#version 430  

// Posición
layout( std430, binding=0 ) buffer Pos {
   vec4 position[ ]; 
};

// Velocidad
layout( std430, binding=1 ) buffer Vel {
   vec4 velocity[ ]; 
};

// Velocidad
layout( std430, binding=2 ) buffer Color {
   vec4 color[ ]; 
};

// Posición predicha
layout( std430, binding=3 ) buffer PosPred {
   vec4 position_pred[ ]; 
};

// Celda
layout( std430, binding=6 ) buffer Cell {
   uint cell[ ]; 
};

// GridCells
layout( std430, binding=7 ) buffer GridCells {
   uint gridCells[ ]; 
};

// Tarea por hacer: definir el tamaño del grupo de trabajo local.
layout( local_size_x = 100, local_size_y = 1, local_size_z = 1 ) in;


const int THREADS_PER_BLOCK = 50;




////////////////////////////////////////////////////////////////////////////////
//
// Parámetros SPH
//
////////////////////////////////////////////////////////////////////////////////
const int MAX_PART_PER_CELL = 20;                      // Máximo número de partículas por celda

float dt                 = 0.01;
const float density_rest = 998.29f;
const int ppedge         = 30;
const float kernel_part  = 20;
const float side         = 4;
const float volume       = side*side*side;
const float inc          = side / ppedge;
const int numPart        = ppedge*ppedge*ppedge;
const float mass         = density_rest * volume / numPart;

// Radio kernel
const float PI = 3.14159265359f;
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

uint get_id() {
    return uint(gl_GlobalInvocationID.x);
}

uint grid_get(const vec3 pos) {
    vec3 c = floor((pos - LIM_INF) / h);

    uint cell_id = uint((c.x*CELL.y + c.y)*CELL.z + c.z) % CELL_NUMBER;

    return cell_id;
}

vec3 externalForces() {
	const vec3 K_GRAVITY = vec3( 0.0, -9.8, 0.0 );

    return K_GRAVITY;
}

void applyPredict() {
    uint i = get_id();

    // Global Memory -> Register
    vec3 p_i = position[i].xyz;
    vec3 v_i = velocity[i].xyz;

    // Predecir Velocidad
    v_i += externalForces() * dt;

    // Predecir Position
    p_i += v_i * dt;  

    // Register -> Global Memory
    velocity[i].xyz = v_i; 
    position_pred[i].xyz = p_i;
}

void updateParticleCell() {
    uint i = get_id();
    cell[i] = grid_get(position_pred[i].xyz);
}

void clearGrid() {
    for(uint c = 0; c < CELL_NUMBER; c++) {
		const uint m1 = c*(MAX_PART_PER_CELL + 1);
        gridCells[m1] = 0;
	}
    /*uint i = get_id();
    gridCells[(MAX_PART_NEIGHBORS+1)*i] = 0;*/
}

void main() {
    applyPredict();
	
    clearGrid(); // Pone el contador de partículas de cada celda del grid a 0. gridCell
	
    updateParticleCell(); // Actualiza el índice de celda de las partículas. cell    
}
