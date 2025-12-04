echo "Compiling shaders"

glslc vertex.vert -o vertex.spv
glslc fragment.frag -o fragment.spv
glslc matrices.comp -o matrices.spv
glslc culling.comp -o culling.spv

echo "Done!"
