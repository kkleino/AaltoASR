The Aalto ASR tools are build with the cmake build system.


## Building under Unix-like operating systems

### Required packages for Ubuntu (and other Debian-based distributions)

    build-essential
    cmake

    swig
    libsndfile1-dev
    libsdl1.2-dev
    liblapack-dev
    python2.7-dev
    gfortran

    libfftw3-dev (Optional. If not provided KissFFT is used)

### Required packages for Scientific Linux (and other Redhat-based distributions)

    yum groupinstall "Development Tools"
    cmake28 (EPEL)

    SDL-devel
    python-devel
    lapack-devel
    libsndfile-devel
    
    fftw-devel (optional. If not provided KissFFT is used)

### Build instructions

    git clone https://github.com/aalto-speech/AaltoASR.git
    cd AaltoASR
    mkdir build
    cd build 
    cmake ..
    make

Instead of make all, also only a subproject or executable can be build, e.g.:
   
    make aku

or 
    make playseg

After this all a 

    make install

will install the binaries and libraries on the correct places. This location can be changed with the option -DCMAKE_INSTALL_PREFIX=/path to the cmake command.

For a debug build, add -DCMAKE_BUILD_TYPE=Debug to the cmake command (very slow). For an optimized build add -DCMAKE_BUILD_TYPE=Release.

Normally, if available, the FFTW library is used, otherwise the KissFFT library is used. To force the KissFFT library, add -DKISS_FFT=1 to the cmake command.

### Creating an Eclipse project

If you would like to edit/debug the source code in Eclipse, you can follow the instructions given on http://www.vtk.org/Wiki/Eclipse_CDT4_Generator

In short

    git clone https://github.com/aalto-speech/AaltoASR.git
    cd AaltoASR
    cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug .

And import the resulting project in eclipse.


## Building under Windows and MinGW

* Install MinGW and MSYS environment.

Follow the instructions on this page: http://ingar.satgnu.net/devenv/mingw32/base.html

* Open MSYS shell and load the correct build environment.

For a 32-bit build, use

    source /local32/etc/profile.local

For a 64-bit build, use

    source /local64/etc/profile.local

* Install SDL.

    cd ${LOCALSOURCEDIR} && \
    wget -c http://www.libsdl.org/release/SDL-1.2.15.tar.gz && \
    cd ${LOCALBUILDDIR} && \
    tar xzf ${LOCALSOURCEDIR}/SDL-1.2.15.tar.gz && \
    cd SDL-1.2.15 && \
    ./configure --prefix=${LOCALDESTDIR} && \
    make && \
    make install

* Install libsndfile.

    cd ${LOCALSOURCEDIR} && \
    wget -c http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.25.tar.gz && \
    cd ${LOCALBUILDDIR} && \
    tar xzf ${LOCALSOURCEDIR}/libsndfile-1.0.25.tar.gz && \
    cd libsndfile-1.0.25 && \
    ./configure --prefix=${LOCALDESTDIR} && \
    make && \
    make install

* Install OpenBLAS.

    cd ${LOCALBUILDDIR} && \
    git clone https://github.com/xianyi/OpenBLAS.git && \
    cd OpenBLAS && \
    make && \
    make install PREFIX=${LOCALDESTDIR}

* Install AaltoASR.

    cd ${LOCALSOURCEDIR} && \
    git clone https://github.com/aalto-speech/AaltoASR.git && \
    cd ${LOCALBUILDDIR} && \
    mkdir AaltoASR && \
    cd AaltoASR && \
    cmake ${LOCALSOURCEDIR}/AaltoASR -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=${LOCALDESTDIR} -DDISABLE_SWIG=On && \
    make && \
    make install

OR

    cmake -G"Eclipse CDT4 - MinGW Makefiles" -DDISABLE_SWIG=On -DCMAKE_BUILD_TYPE=Release


## Cross-compiling for Windows under Unix-like operating systems

### Required packages for Ubuntu (and other Debian-based distributions)

    build-essential
    cmake
    gcc-mingw-w64-i686 (or gcc-mingw-w64-x86-64)
    gfortran-mingw-w64-i686 (or gfortran-mingw-w64-x86-64)

### Build instructions

When using the cross-compilation toolchain, cmake compiles the dependencies automatically using a cross-compiler.

    git clone https://github.com/aalto-speech/AaltoASR.git
    cd AaltoASR
    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=cmake/cross-mingw-i686/toolchain.cmake -DDISABLE_SWIG=On ..
    make
