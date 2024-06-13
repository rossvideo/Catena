FROM ghcr.io/isaac-robert-ross/catena-env:latest

LABEL org.opencontainers.image.source https://github.com/rossvideo/Catena
LABEL org.opencontainers.image.description="Development environment for the Catena C++ SDK"
LABEL org.opencontainers.image.licenses=CC-BY-ND-4.0

RUN git clone https://github.com/google/googletest.git -b v1.14.0 && \
    cd googletest && \
    mkdir build && \
    cd build && \
    cmake .. -DBUILD_GMOCK=OFF && \
    make && \
    make install

# build without additional docs for now
# RUN apt-get install doxygen