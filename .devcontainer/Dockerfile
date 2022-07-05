FROM gcc:12.1

WORKDIR /

RUN echo "Installing development tools ..."; \
    set -ex; \
    apt-get update && export DEBIAN_FRONTEND=noninteractive; \
    apt-get install -y --no-install-recommends \
        build-essential \
        vim \
        gdb \
        htop \
        cmake \
        python-dev \
        autotools-dev \
        libicu-dev \
        libbz2-dev \
        libgoogle-perftools-dev \
        clang-tidy \
        clang-format;

RUN echo "Installing mold linker ..."; \
    set -ex; \
    git clone https://github.com/rui314/mold.git; \
    cd mold; \
    git checkout v1.3.1; \
    make -j4; \
    make install; \
    rm -rf /mold;

RUN echo "Installing google test ..."; \
    set -ex; \
    git clone https://github.com/google/googletest.git; \
    cd googletest; \
    mkdir build; \
    cd build; \
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..; \
    make -j4 install; \
    rm -rf /googletest;

RUN echo "Installing google benchmark ..."; \
    set -ex; \
    git clone https://github.com/google/benchmark.git; \
    cd benchmark; \
    mkdir build; \
    cd build; \
    cmake -G "Unix Makefiles" -DBENCHMARK_ENABLE_GTEST_TESTS=OFF -DCMAKE_BUILD_TYPE=Release ..; \
    make -j4 install; \
    rm -rf /benchmark;

RUN echo "Installing boost:1.77 ..."; \
    set -ex; \
    wget -O boost_1_77_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.77.0/boost_1_77_0.tar.gz/download; \
    tar xzvf boost_1_77_0.tar.gz; \
    cd boost_1_77_0/; \
    ./bootstrap.sh --prefix=/usr/local; \
    ./b2 --with=all -j4 release install; \
    rm -rf /boost_1_77_0*;

RUN echo "Installing spdlog ..."; \
    set -ex; \
    git clone https://github.com/gabime/spdlog.git; \
    cd spdlog; \
    mkdir build; \
    cd build; \
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..; \
    make -j4 install; \
    rm -rf /spdlog;

RUN echo "Installing libzmq ..."; \
    set -ex; \
    git clone https://github.com/zeromq/libzmq.git; \
    cd libzmq; \
    mkdir build; \
    cd build; \
    cmake -G "Unix Makefiles" -DENABLE_DRAFTS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo ..; \
    make -j4 install; \
    rm -rf /libzmq;

RUN echo "Installing cppzmq ..."; \
    set -ex; \
    git clone https://github.com/zeromq/cppzmq.git; \
    cd cppzmq; \
    mkdir build; \
    cd build; \
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..; \
    make -j4 install; \
    rm -rf /cppzmq;

RUN echo "Installing quickfix ..."; \
    set -ex; \
    git clone https://github.com/quickfix/quickfix.git; \
    cd quickfix; \
    mkdir build; \
    cd build; \
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..; \
    make -j4 install; \
    rm -rf /quickfix;

RUN echo "Installing flatbuffers ..."; \
    set -ex; \
    git clone https://github.com/google/flatbuffers.git; \
    cd flatbuffers; \
    mkdir build; \
    cd build; \
    cmake -G "Unix Makefiles" -DFLATBUFFERS_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo ..; \
    make -j4 install; \
    rm -rf /flatbuffers;

RUN echo "Installing nlohmann/json ..."; \
    set -ex; \
    git clone https://github.com/nlohmann/json.git; \
    cd json; \
    mkdir build; \
    cd build; \
    cmake -G "Unix Makefiles" -DJSON_BuildTests=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo ..; \
    make -j4 install; \
    rm -rf /json;

RUN echo "Installing eventpp ..."; \
    set -ex; \
    git clone https://github.com/wqking/eventpp.git; \
    cp -r eventpp/include/eventpp /usr/local/include; \
    rm -rf /eventpp;


RUN echo "export LD_LIBRARY_PATH ...";
ENV LD_LIBRARY_PATH=/usr/local/lib;
