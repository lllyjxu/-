# Dockerfile
FROM ubuntu:20.04

# Install dependencies, including curl
RUN apt-get update && apt-get upgrade -y && apt-get install -y \
    gcc \
    make \
    curl \
    libmicrohttpd-dev \
    libjansson-dev \
    libcurl4-openssl-dev \
    && apt-get clean

# Set the working directory
WORKDIR /app

# Copy source code
COPY main.c /app/main.c

# Compile the application
RUN gcc -o server main.c -lmicrohttpd -ljansson -lcurl -pthread

# Expose the application port
EXPOSE 9527

# Command to start the application
CMD ["/app/server"]
