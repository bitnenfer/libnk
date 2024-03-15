@echo off

dxc sprite_vs.hlsl -T vs_6_0 -E main -Fo sprite_vs.cso
dxc sprite_ps.hlsl -T ps_6_0 -E main -Fo sprite_ps.cso

python bin2c.py sprite_vs.cso sprite_ps.cso -o sprite_shaders.h

xcopy /y sprite_shaders.h ..\..\src\backend\windows\sprite_shaders.h

if exist *.cso (
	del *.cso /f /q
)

if exist *.h (
	del *.h /f /q
)
