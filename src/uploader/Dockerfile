FROM fedora:33
RUN yum install -y python3
RUN yum install -y python-requests
COPY ./secrets /secrets
COPY ./server.py /server.py
RUN ls -lh /secrets/
RUN source /secrets/imgur.sh
EXPOSE 8000
CMD [ "bash", "-c", "source /secrets/imgur.sh && python3 /server.py" ]
