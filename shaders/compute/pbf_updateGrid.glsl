#version 430

const int MAX_PART_PER_CELL = 20;

// Celda
layout( std430, binding=6 ) buffer Cell {
   uint cell[ ]; 
};

// GridCells
layout( std430, binding=7 ) buffer GridCells {
   uint gridCells[ ]; 
};

layout( local_size_x = 100, local_size_y = 1, local_size_z = 1 ) in;

void main() {
	// Inserta los índices de las partículas en el grid. También cuenta cuántas partículas hay. gridCell

	uint i = gl_GlobalInvocationID.x;

    const uint m1 = cell[i]*(MAX_PART_PER_CELL + 1);
    
    // Contar y distribuir
    uint old = atomicAdd(gridCells[m1], 1);

    if (old >= MAX_PART_PER_CELL)
        gridCells[m1] = MAX_PART_PER_CELL;
    else
        gridCells[m1 + (old + 1)] = i; // En el grid tenemos posiciones del vector de partículas, NO ids de partículas
}
