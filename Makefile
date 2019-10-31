WIN_CC = i686-w64-mingw32.static-g++
MSYS2_CC = x86_64-w64-mingw32-g++
LINUX_CC = g++
MACOS_CC = clang

WIN_FLAGS = -O3 -mwindows -mconsole -lgdiplus -lws2_32 -lole32 -lpsapi -lprotobuf -static-libstdc++ -std=c++11
LINUX_FLAGS = -O3 -lX11 -lXext -lXtst -lXi -lpthread -lturbojpeg -lprotobuf -std=c++11
MACOS_FLAGS = -O3 -I/usr/local/include -L/usr/local/lib/ -lprotobuf -lc++ -std=c++11 -framework Foundation -framework Carbon
PROFILING_FLAG = -DPROFILING

CPP = src/main.cpp src/socket.cpp src/profiling.cpp src/keys.cpp
HPP = src/socket.hpp src/profiling.hpp src/keys.hpp

PB_CC = src/messages.pb.cc
PB_H = src/messages.pb.h

WIN_CPP = ${CPP} ${PB_CC} src/win/win.cpp src/win/screen.cpp src/win/inputs.cpp
WIN_HPP = ${HPP} ${PC_H} src/platform.hpp

LINUX_CPP = ${CPP} ${PB_CC} src/linux.cpp
LINUX_HPP = ${HPP} ${PC_H} src/platform.hpp

MACOS_CPP = ${CPP} ${PB_CC} src/macos.cpp
MACOS_HPP = ${HPP} ${PC_H} src/platform.hpp

PROTO = messages.proto

WIN_OUTPUT = bin/main.exe
OUTPUT = bin/main

windows: init protoc_win ${WIN_CPP} ${WIN_HPP}
	${WIN_CC} -o ${WIN_OUTPUT} ${WIN_CPP} ${WIN_FLAGS}

windows_profiling: init protoc_win ${WIN_CPP} ${WIN_HPP}
	${WIN_CC} -o ${WIN_OUTPUT} ${WIN_CPP} ${WIN_FLAGS} ${PROFILING_FLAG}

msys2: init protoc ${WIN_CPP} ${WIN_HPP}
	${MSYS2_CC} -o ${WIN_OUTPUT} ${WIN_CPP} ${WIN_FLAGS}
	cp /mingw64/bin/libprotobuf.dll /mingw64/bin/libgcc_s_seh-1.dll /mingw64/bin/libwinpthread-1.dll /mingw64/bin/libstdc++-6.dll /mingw64/bin/zlib1.dll ./bin
	
msys2_profiling: init protoc ${WIN_CPP} ${WIN_HPP}
	${MSYS2_CC} -o ${WIN_OUTPUT} ${WIN_CPP} ${WIN_FLAGS} ${PROFILING_FLAG}
	cp /mingw64/bin/libprotobuf.dll /mingw64/bin/libgcc_s_seh-1.dll /mingw64/bin/libwinpthread-1.dll /mingw64/bin/libstdc++-6.dll /mingw64/bin/zlib1.dll ./bin
	
linux: init protoc ${LINUX_CPP} ${LINUX_HPP}
	${LINUX_CC} -o ${OUTPUT} ${LINUX_CPP} ${LINUX_FLAGS}

linux_profiling: init protoc ${LINUX_CPP} ${LINUX_HPP}
	${LINUX_CC} -o ${OUTPUT} ${LINUX_CPP} ${LINUX_FLAGS} ${PROFILING_FLAG}

mac: init protoc ${MACOS_CPP} ${MACOS_HPP}
	${MACOS_CC} -o ${OUTPUT} ${MACOS_CPP} ${MACOS_FLAGS}

mac_profiling: init protoc ${MACOS_CPP} ${MACOS_HPP}
	${MACOS_CC} -o ${OUTPUT} ${MACOS_CPP} ${MACOS_FLAGS} ${PROFILING_FLAG}

init:
	mkdir -p bin

# When cross compiling for Windows, use the protoc binary built by MXE,
# which should be located at MXE_PROTOC (see the install instructions in README)
protoc_win: ${PROTO}
	${MXE_PROTOC} --cpp_out=src --python_out=examples ${PROTO}

# On Linux/MacOS/MSYS2 we can use the system protoc since it should match the libprotobuf version
protoc: ${PROTO}
	protoc --cpp_out=src --python_out=examples ${PROTO}