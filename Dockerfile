# FROM resin/rpi-raspbian:wheezy-2015-01-15
FROM debian:wheezy

RUN apt-get update && apt-get install -y \
    git \
#    libxml2-dev \
#    python \
    build-essential \
    make \
    cmake \
    gcc \
#    python-dev \
    locales \
#    python-pip
    libusb-dev \
    libssl-dev \
    mosquitto

RUN dpkg-reconfigure locales && \
    locale-gen C.UTF-8 && \
    /usr/sbin/update-locale LANG=C.UTF-8

ENV LC_ALL C.UTF-8


# Update package lists
# RUN apt-get update

# Install libusb.
# RUN apt-get install -y libusb-dev

# Install git client
# RUN apt-get install -y git

# Get and build Paho client
WORKDIR /tmp/paho
RUN git clone http://git.eclipse.org/gitroot/paho/org.eclipse.paho.mqtt.c.git /tmp/paho && make && make install

# Get and build skyetek-mqtt
WORKDIR /tmp/skyetek-mqtt/build
RUN git clone https://github.com/MikeStankavich/skyetek-mqtt /tmp/skyetek-mqtt && RUN cmake .. && make && cp /tmp/skyetek-mqtt/build/skyetek_mqtt /usr/local/sbin/


# what about that skyetek usb permission annoyance?
CMD ["service mosquitto start", "skyetek-mqtt"]