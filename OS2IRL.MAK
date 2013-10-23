# os2irl.mak
#
# The actions included in this make file are:
#  Compile::C++ Compiler
#  Link::Linker
#  Bind::Resource Bind

.SUFFIXES: .C .obj .SQC

.all: \
    os2irl.exe

.C.obj:
    @echo " Compile::C++ Compiler "
    icc.exe /Gh /Ti /Ss /Gm /W3 /Gd /Fo"%|dpfF.obj" /C %s

os2irl.exe: \
    os2irl.obj
    @echo " Link::Linker "
    @echo " Bind::Resource Bind "
    icc.exe @<<
     /B" /de /noe /STACK:102400"
     /Feos2irl.exe
     os2irl.obj
<<

os2irl.obj: \
    os2irl.C

