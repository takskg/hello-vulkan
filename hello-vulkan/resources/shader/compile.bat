rem コンパイル
glslangValidator.exe ezshader.vert -V -S vert -o ezshader.vert.spv
glslangValidator.exe texshader.vert -V -S vert -o texshader.vert.spv
glslangValidator.exe texshaderUv.vert -V -S vert -o texshaderUv.vert.spv
glslangValidator.exe ezshader.frag -V -S frag -o ezshader.frag.spv
glslangValidator.exe texshader.frag -V -S frag -o texshader.frag.spv
glslangValidator.exe texshaderOpaque.frag -V -S frag -o texshaderOpaque.frag.spv
glslangValidator.exe texshaderAlpha.frag -V -S frag -o texshaderAlpha.frag.spv

rem リソースを出力先にコピー
copy /Y ezshader.vert.spv ..\..\..\resources\shader\ezshader.vert.spv
copy /Y texshader.vert.spv ..\..\..\resources\shader\texshader.vert.spv
copy /Y texshaderUv.vert.spv ..\..\..\resources\shader\texshaderUv.vert.spv
copy /Y ezshader.frag.spv ..\..\..\resources\shader\ezshader.frag.spv
copy /Y texshader.frag.spv ..\..\..\resources\shader\texshader.frag.spv
copy /Y texshaderOpaque.frag.spv ..\..\..\resources\shader\texshaderOpaque.frag.spv
copy /Y texshaderAlpha.frag.spv ..\..\..\resources\shader\texshaderAlpha.frag.spv