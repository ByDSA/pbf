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

void cellNeighbors(vec3 pos_ini, inout uint cells[CELL_NEIGHBORS]) {
    cells[0]  = grid_get( pos_ini + vec3(-h, -h, -h) );  // {x-1, y-1, z-1}
    cells[1]  = grid_get( pos_ini + vec3(-h, -h,  0) );  // {x-1, y-1, z}
    cells[2]  = grid_get( pos_ini + vec3(-h, -h, +h) );  // {x-1, y-1, z+1}

    cells[3]  = grid_get( pos_ini + vec3(-h,  0, -h) );  // {x-1, y, z-1}
    cells[4]  = grid_get( pos_ini + vec3(-h,  0,  0) );  // {x-1, y, z}
    cells[5]  = grid_get( pos_ini + vec3(-h,  0, +h) );  // {x-1, y, z+1}

    cells[6]  = grid_get( pos_ini + vec3(-h, +h, -h) );  // {x-1, y+1, z-1}
    cells[7]  = grid_get( pos_ini + vec3(-h, +h,  0) );  // {x-1, y+1, z}
    cells[8]  = grid_get( pos_ini + vec3(-h, +h, +h) );  // {x-1, y+1, z+1}

    cells[9]  = grid_get( pos_ini + vec3( 0, -h, -h) );  // {x, y-1, z-1}
    cells[10] = grid_get( pos_ini + vec3( 0, -h,  0) );  // {x, y-1, z}
    cells[11] = grid_get( pos_ini + vec3( 0, -h, +h) );  // {x, y-1, z+1}

    cells[12] = grid_get( pos_ini + vec3( 0,  0, -h) );  // {x, y, z-1}
    cells[13] = grid_get( pos_ini + vec3( 0,  0,  0) );  // {x, y, z}
    cells[14] = grid_get( pos_ini + vec3( 0,  0, +h) );  // {x, y, z+1}
    
    cells[15] = grid_get( pos_ini + vec3( 0, +h, -h) );  // {x, y+1, z-1}
    cells[16] = grid_get( pos_ini + vec3( 0, +h,  0) );  // {x, y+1, z}
    cells[17] = grid_get( pos_ini + vec3( 0, +h, +h) );  // {x, y+1, z+1}

    cells[18] = grid_get( pos_ini + vec3(+h, -h, -h) );  // {x+1, y-1, z-1}
    cells[19] = grid_get( pos_ini + vec3(+h, -h,  0) );  // {x+1, y-1, z}
    cells[20] = grid_get( pos_ini + vec3(+h, -h, +h) );  // {x+1, y-1, z+1}

    cells[21] = grid_get( pos_ini + vec3(+h,  0, -h) );  // {x+1, y, z-1}
    cells[22] = grid_get( pos_ini + vec3(+h,  0,  0) );  // {x+1, y, z}
    cells[23] = grid_get( pos_ini + vec3(+h,  0, +h) );;  // {x+1, y, z+1}

    cells[24] = grid_get( pos_ini + vec3(+h, +h, -h) );  // {x+1, y+1, z-1}
    cells[25] = grid_get( pos_ini + vec3(+h, +h,  0) );  // {x+1, y+1, z}
    cells[26] = grid_get( pos_ini + vec3(+h, +h, +h) );  // {x+1, y+1, z+1}
}

void main() {
	const uint i = gl_GlobalInvocationID.x;

    // Global Memory -> Registers
    const vec3 p1 = position_pred[i].xyz;

    // Celdas vecinas
    const uint cell_ini = grid_get(p1);
    uint cells[CELL_NEIGHBORS];
    cellNeighbors(p1, cells);
    
    uint gc; // Número de partículas en cierta celda
    uint n = 0; // Número de vecinos

    uint m1;
    const uint m2 = (MAX_PART_NEIGHBORS+1)*i;

    // Añadir vecinos
    for(uint c = 0; c < CELL_NEIGHBORS; ++c) {
        m1 = cells[c]*(MAX_PART_PER_CELL+1);
        gc = gridCells[m1]; // Número de partículas en celda 'cells[c]'

        if (gc == 0) // Si la celda está vacía, no hacer nada
            continue;

        
        // Añadir vecinos de cada celda
        // @pre: cells[c].size() > 0
        // @post: ?pj: dist(pi, pj) < h
        for(uint j = 1; j <= gc; j++) {
            uint pos2 = gridCells[m1 + j];
            
            float dist = length(p1 - position_pred[pos2].xyz);

            if (dist < h) {
                neighbors[m2 + (n+1)] = pos2;
                n++;
            }
        }
    }

    neighbors[m2] = n;
}
