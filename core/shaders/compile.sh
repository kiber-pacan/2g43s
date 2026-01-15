#!/bin/fish
echo "Compiling shaders..."
set OUT_DIR "compiled"

# Fish использует другой синтаксис для циклов и поиска
for shader in (find . -type f \( -name "*.vert" -o -name "*.frag" -o -name "*.comp" \) -not -path "./$OUT_DIR/*")
    # Очищаем путь
    set clean_path (string replace "./" "" $shader)

    # Генерируем выходной путь
    set output_file "$OUT_DIR/"(string replace -r '\.\w+$' '.spv' $clean_path)
w
    # Создаем папку
    mkdir -p (dirname $output_file)

    echo "Compiling: $clean_path"
    glslc $clean_path --target-env=vulkan1.4 -O -o $output_file

    if test $status -ne 0
        echo "ERROR: Failed to compile $clean_path"
        exit 1
    end
end

echo "Done!"