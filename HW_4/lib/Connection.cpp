//
// Created by Andrew Kireev on 30.03.2020.
//


#include <exception>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>

#include "Connection.h"
#include "exception.h"


namespace server {

    Connection::Connection(std::string ip, uint16_t port) : dst_addr_(std::move(ip)), dst_port_(port){
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd_ < 0){
            close();
            throw TcpException("socket creating error");
        }
        connect(dst_addr_, port);
    }

    Connection::Connection(int sockfd, const sockaddr_in& sock): sockfd_(sockfd),
                                                                 dst_port_(sock.sin_port), is_open_(true){
        sockfd = -1;
    }

    Connection::Connection(int fd) : sockfd_(fd), is_open_(true){}


    Connection::~Connection() noexcept{
        try {
            close();
        } catch (std::runtime_error& er){
            std::cerr << "closing fail" << er.what() << std::endl;
        }
    }

    size_t Connection::write(const void *data, std::size_t len) {
        ssize_t bytes = ::write(sockfd_, data, len);

        if(bytes < 0){
            throw TcpException("could not write anything");
        }
        return bytes;
    }

    void Connection::writeExact(const void *data, size_t len) {
        size_t wr = 0, last_it = 0;
        while (wr != len) {
            wr += write(static_cast<const char*>(data) + wr, len - wr);
            if (wr == last_it) {
                throw TcpException("received not enough to write");
            }
            last_it = wr;
        }
    }

    size_t Connection::read(void *data, std::size_t len) {
        if (!is_open_){
            throw TcpException("closed descriptor");
        }

        ssize_t bytes = ::read(sockfd_, data, len);

        if (bytes == 0) {
            is_open_ = false;
        } else if (bytes < 0) {
            throw TcpException("could not read anything");
        }
        return bytes;
    }

    void Connection::readExact(void *data, size_t len) {
        if (!is_open_){
            throw TcpException("closed descriptor");
        }

        std::size_t read = 0, last_it = 0;
        size_t num;
        while (read != len) {
            num = ::read(sockfd_, static_cast<char *>(data) + read, len - read);
            if(num < 0 && read == 0){
                throw TcpException("could not read anything");
            }
            read += num;
            if (read == last_it) {
                throw TcpException("received not enough to read");
            }
            last_it = read;
        }
    }

    void Connection::close(){
        if(sockfd_ != -1 && !is_open_) {
            if (::close(sockfd_) < 0) {
                throw TcpException("close failed");
            }
        }
        is_open_ = false;
    }

    bool Connection::is_opened() const {
        return is_open_;
    }

    void Connection::set_timeout(int time){
        timeval timeout{time, .tv_usec = 0};
        if (setsockopt(sockfd_,
                       SOL_SOCKET,
                       SO_RCVTIMEO,
                       &timeout,
                       sizeof(timeout)) < 0){
            throw TcpException("timeout failed");
        }

    }

    void Connection::send_size(const size_t& data)
    {
        writeExact(&data, sizeof(size_t));
    }

    void Connection::recv_size(size_t& data)
    {
        readExact(&data, sizeof(size_t));
    }

    void Connection::connect(const std::string& addr, uint16_t port) {
        struct sockaddr_in addr_{};
        dst_addr_ = addr;

        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        addr_.sin_addr.s_addr = ::inet_addr(addr.c_str());
        if(::connect(sockfd_, (struct sockaddr *)&addr_, sizeof(addr_)) < 0){
            close();
            throw TcpException("connecting failed");
        }
        is_open_ = true;
    }

}