FROM mcr.microsoft.com/dotnet/sdk:7.0 AS build-env

ENV HOME /home
RUN chown 1000:1000 /home
USER 1000

WORKDIR /app
ENTRYPOINT ["/app/docker/config/entrypoint.sh"]
