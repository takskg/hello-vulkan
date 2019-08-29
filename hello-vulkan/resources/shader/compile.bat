rem コンパイル
glslangValidator.exe ezshader.vert -V -S vert -o ezshader.vert.spv
glslangValidator.exe ezshader.frag -V -S frag -o ezshader.frag.spv

rem リソースを出力先にコピー
copy /Y ezshader.vert.spv ..\..\..\resources\shader\ezshader.vert.spv
copy /Y ezshader.frag.spv ..\..\..\resources\shader\ezshader.frag.spv