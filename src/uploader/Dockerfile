FROM fedora:33
RUN yum install -y python3
RUN yum install -y python-requests
COPY ./secrets /secrets
COPY ./server.py /server.py
RUN ls -lh /secrets/
RUN source /secrets/imgur.sh
RUN echo "America/New_York" > /etc/timezone
RUN rm /etc/localtime && cd /etc && ln -s ../usr/share/zoneinfo/America/New_York localtime
EXPOSE 8000
CMD [ "bash", "-c", "source /secrets/imgur.sh && python3 /server.py" ]
