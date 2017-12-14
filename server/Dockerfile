FROM iot-lab-gateway

COPY . /iot-lab-gateway/
RUN cd iot-lab-gateway && \
    python setup.py develop

#test with M3 config
RUN mkdir -p /var/local/config/ && \
    mkdir -p /iotlab/users/test && \
    chown www-data:www-data /iotlab/users/test

USER www-data

CMD ["/iot-lab-gateway/docker-gateway-rest-server"]