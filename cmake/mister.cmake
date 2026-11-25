# CannonBall on the MiSTer hybrid core (ARM, the hybrid SDL2: software rendering, no OpenGL)
set(boost_dir /home/jess/ionfury/third/dl/boost)
set(sdl2_dir /home/jess/ionfury/EDuke32_MiSTer/build/sdl/mister/prefix/lib/cmake/SDL2)
set(platform_link_libs /home/jess/ionfury/EDuke32_MiSTer/build/sdl/mister/prefix/lib/libSDL2.a pthread dl m rt)
# the MiSTer's libstdc++ is older than the toolchain's: the C++ runtime goes in statically
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
