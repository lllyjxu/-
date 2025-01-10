# 使用官方 Ubuntu 20.04 镜像
FROM ubuntu:20.04

# 设置环境变量，防止提示交互式安装
ENV DEBIAN_FRONTEND=noninteractive

# 更新APT并安装所需的依赖
RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y \
    gcc \
    make \
    curl \
    libmicrohttpd-dev \
    libjansson-dev \
    libcurl4-openssl-dev \
    && apt-get clean

# 设置工作目录
WORKDIR /app

# 将C语言源代码复制到容器中
COPY main.c /app/main.c

# 编译C程序
RUN gcc -o server main.c -lmicrohttpd -ljansson -lcurl -pthread

# 暴露服务端口
EXPOSE 9527

# 运行程序
CMD ["/app/server", "&&", "tail", "-f", "/dev/null"]

