echo "Compiling shaders"

glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv

echo "Done!"
