echo "Compiling shaders"

glslc vertex.vert -o vertex.spv
glslc fragment.frag -o fragment.spv
glslc matrices.comp -o matrices.spv
#glslc occlusion.comp -o occlusion.spv

echo "Done!"
