FROM mcr.microsoft.com/dotnet/sdk:8.0 AS build-env

# Lets the entrypoint add the host user to /etc/passwd. The .NET runtime fails
# to create named mutexes for a user that has no entry there.
RUN chmod a+w /etc/passwd

ENV HOME=/app
WORKDIR /app
ENTRYPOINT ["/app/tools/shared/docker/installer/entrypoint.sh"]
