cd /home/ptato/cookies-emu
mkdir --parents Debug

gcc cookies.c -ISDL2-2.0.7/include -lSDL2 -o "Debug/cookies"

# 32-bit build
# cl %CommonCompilerFlags% fuck_win32.c /link -subsystem:windows,5.1 %CommonLinkerFlags%  opengl32.lib user32.lib gdi32.lib
