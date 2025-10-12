echo "Compiling shaders"

glslc vertex.vert -o vert.spv
glslc fragment.frag -o frag.spv
glslc compute.comp -o comp.spv

echo "Done!"
