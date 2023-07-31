# Use an official Ubuntu image
FROM ubuntu:latest

# Set the working directory in the container to /app
WORKDIR /app

# Copy the current directory contents into the container at /app
COPY . /app

# Install necessary packages
RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y build-essential g++ libboost-all-dev

# Compile the server
RUN g++ -std=c++11 udp_server.cpp -o udp_server

# Make port 9003 available to the world outside this container
EXPOSE 9003/udp

# Run udp_server when the container launches
CMD ["./udp_server"]
