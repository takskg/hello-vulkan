rem �R���p�C��
glslangValidator.exe ezshader.vert -V -S vert -o ezshader.vert.spv
glslangValidator.exe ezshader.frag -V -S frag -o ezshader.frag.spv

rem ���\�[�X���o�͐�ɃR�s�[
copy /Y ezshader.vert.spv ..\..\..\resources\shader\ezshader.vert.spv
copy /Y ezshader.frag.spv ..\..\..\resources\shader\ezshader.frag.spv