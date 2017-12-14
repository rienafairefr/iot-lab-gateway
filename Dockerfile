# This file is a part of IoT-LAB gateway_code
# Copyright (C) 2015 INRIA (Contact: admin@iot-lab.info)
# Contributor(s) : see AUTHORS file
#
# This software is governed by the CeCILL license under French law
# and abiding by the rules of distribution of free software.  You can  use,
# modify and/ or redistribute the software under the terms of the CeCILL
# license as circulated by CEA, CNRS and INRIA at the following URL
# http://www.cecill.info.
#
# As a counterpart to the access to the source code and  rights to copy,
# modify and redistribute granted by the license, users are provided only
# with a limited warranty  and the software's author,  the holder of the
# economic rights,  and the successive licensors  have only  limited
# liability.
#
# The fact that you are presently reading this means that you have had
# knowledge of the CeCILL license and that you accept its terms.

FROM ubuntu:14.04 as openocd

#openocd
RUN apt-get update && \
    apt-get install -y \
        git \
        build-essential \
        libftdi-dev \
        libhidapi-dev \
        libusb-1.0-0-dev \
        autoconf \
        libpopt-dev \
        libtool \
        pkg-config && \
    apt-get clean

#openocd install (for M3 and SAMR21)
RUN git clone https://github.com/ntfreak/openocd && \
    cd openocd && \
    git checkout v0.9.0 && \
    ./bootstrap && \
    ./configure --prefix=/opt/openocd --enable-legacy-ft2232_libftdi --disable-ftdi2232 \
        --disable-ftd2xx --enable-cmsis-dap --enable-hidapi-libusb && \
    make && \
    sudo make install && \
    cd .. && rm -rf openocd

#openocd 0.10
RUN git clone https://github.com/ntfreak/openocd openocd10 && \
    cd openocd10 && \
    git checkout v0.10.0 && \
    ./bootstrap && \
    ./configure --prefix=/opt/openocd-0.10.0 --enable-cmsis-dap --enable-hidapi-libusb && \
    make && \
    sudo make install && \
    cd .. && rm -rf openocd10

FROM ubuntu:14.04 as liboml2

RUN apt-get update && \
    apt-get install -y \
        ruby \
        git \
        python \
        check \
        autoconf \
        automake \
        libtool \
        gnulib \
        libpopt-dev \
        libxml2 \
        libxml2-dev \
        libsqlite3-dev \
        pkg-config \
        libxml2-utils && \
    apt-get clean

RUN git clone https://github.com/mytestbed/oml.git && \
    cd oml && \
    git checkout tags/v2.11.0 && \
    ./autogen.sh && \
    ./configure --prefix=/opt/oml --disable-doc --disable-doxygen-doc --disable-doxygen-dot \
        --disable-android --disable-doxygen-html --disable-option-checking && \
    make && \
    make install && \
    cd .. && \
    rm -rf oml

FROM ubuntu:14.04 as iot-lab-ftdi-utils

RUN apt-get update && \
    apt-get install -y \
        git \
        build-essential \
        libftdi-dev \
        libhidapi-dev \
        libusb-1.0-0-dev \
        autoconf \
        libpopt-dev \
        libtool \
        pkg-config && \
    apt-get clean

#iot-lab-ftdi-utils install
RUN git clone https://github.com/iot-lab/iot-lab-ftdi-utils/  && \
    cd iot-lab-ftdi-utils && \
    make && \
    make install && \
    cd .. && \
    rm -rf iot-lab-ftdi-utils

# the actual image
FROM ubuntu:14.04
LABEL maintainer="Matthieu Berthomé <matthieu.berthome@inria.fr>"

#AVR (arduino like)
RUN apt-get update && \
    apt-get install -y \
        avrdude && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN apt-get update && \
    apt-get install -y \
        git \
        build-essential \
        python \
        socat \
        libxml2-dev \
        libftdi-dev \
        libhidapi-dev \
        libusb-1.0-0-dev \
        autoconf \
        libpopt-dev \
        libtool \
        pkg-config \
        ruby && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# cc2538 for firefly
RUN git clone https://github.com/JelmerT/cc2538-bsl && \
    cp cc2538-bsl/cc2538-bsl.py /usr/bin/. && \
    apt-get update && \
    apt-get install -y python-pip binutils && \
    pip install intelhex

# copy from the builders
COPY --from=openocd /opt/openocd /usr/local/
COPY --from=openocd /opt/openocd-0.10.0 /opt/openocd-0.10.0
COPY --from=iot-lab-ftdi-utils /usr/local/bin/ftdi* /usr/local/bin/
COPY --from=liboml2 /opt/oml/ /usr/local/

WORKDIR /iot-lab-gateway