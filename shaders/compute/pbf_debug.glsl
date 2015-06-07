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

// Celda
coherent layout( std430, binding=6 ) buffer Cell {
   coherent uint cell[ ]; 
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

uint get_id() {
    return uint(gl_GlobalInvocationID.x);
}

const int MAX_PART_PER_CELL = 20;                      // Máximo número de partículas por celda
const int CELL_NEIGHBORS     = 27;                     // Celdas vecinas (incluyendo propia)
const int MAX_PART_NEIGHBORS = CELL_NEIGHBORS*MAX_PART_PER_CELL; // Máximo número de vecinos por celda

void main() {
    uint i = get_id();
	uint c = cell[i];

    //color[i] = vec4(n, n, 1.0f, 1.0f);
	color[i] = vec4(abs(lambda[i]*1000), 0, 1.0f, 0.5f);
    

    //color[i] = vec4(c * 73856093 % 300 / 300.0f, c * 19349663 % 300 / 300.0f, c *83492791 % 300 / 300.0f, 1.0f);
}
