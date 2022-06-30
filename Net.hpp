/**
 * Net.hpp
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#ifndef HELIFE_NET_NAMESPACE
    #define HELIFE_NET_NAMESPACE hnet
#endif

#if ! defined(_WIN64) && ! defined(_WIN32) && ! defined(__linux__) && ! defined(__APPLE__)
    #error "Unsupported platform"
#endif

#if defined(_WIN64) || defined(_WIN32)
    #include <windows.h>
    #include <winsock2.h>
    #include <Ws2tcpip.h>
#elif defined(__linux__) || defined(__APPLE__)
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <string.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

#ifndef HELIFE_NET_GET_LAST_ERROR
    #if defined(__linux__) || defined(__APPLE__)
        #define HELIFE_NET_GET_LAST_ERROR(buf, buflen) ::strerror_r(errno, buf, buflen)
    #elif defined(_WIN64) || defined(_WIN32)
        #define HELIFE_NET_GET_LAST_ERROR(buf, buflen) ::strerror_s(errno, buf, buflen)
    #endif
#endif

#ifndef HELIFE_NET_ERROR_MAX_SIZE
    #define HELIFE_NET_ERROR_MAX_SIZE 8192
#endif

#ifndef HELIFE_NET_THROW_LAST_ERROR
    #define HELIFE_NET_THROW_LAST_ERROR(__hnet_error_type, __hnet_prefix) \
        do { \
            char *__hnet_error_buffer = new char[HELIFE_NET_ERROR_MAX_SIZE+1]; \
            if (__hnet_error_buffer == nullptr) \
                throw __hnet_error_type("Error while allocating space to display the error"); \
            HELIFE_NET_GET_LAST_ERROR(__hnet_error_buffer, HELIFE_NET_ERROR_MAX_SIZE); \
            const std::string __hnet_result = std::string(__hnet_prefix) + __hnet_error_buffer; \
            delete[] __hnet_error_buffer; \
            throw __hnet_error_type(__hnet_result); \
        } while (0)
#endif

#ifndef HELIFE_NET_SOCKET
    #if defined(_WIN64) || defined(_WIN32)
        #define HELIFE_NET_SOCKET SOCKET
    #elif defined(__linux__) || defined(__APPLE__)
        #define HELIFE_NET_SOCKET int
    #endif
#endif

#ifndef HELIFE_NET_INVALID_SOCKET
    #if defined(_WIN64) || defined(_WIN32)
        #define HELIFE_NET_INVALID_SOCKET INVALID_SOCKET
    #elif defined(__linux__) || defined(__APPLE__)
        #define HELIFE_NET_INVALID_SOCKET (-1)
    #endif
#endif

#ifndef HELIFE_NET_SOCKET_ERROR
    #if defined(_WIN64) || defined(_WIN32)
        #define HELIFE_NET_SOCKET_ERROR SOCKET_ERROR
    #elif defined(__linux__) || defined(__APPLE__)
        #define HELIFE_NET_SOCKET_ERROR (-1)
    #endif
#endif

#ifndef HELIFE_NET_ERRNO
    #if defined(__linux__) || defined(__APPLE__)
        #define HELIFE_NET_ERRNO errno
    #elif defined(_WIN64) || defined(_WIN32)
        #define HELIFE_NET_ERRNO WSAGetLastError()
    #endif
#endif

#ifndef HELIFE_NET_WINDOWS_SOCKET_VERSION_WSA_STARTUP
    #if defined(_WIN64) || defined(_WIN32)
        #define HELIFE_NET_WINDOWS_SOCKET_VERSION_WSA_STARTUP MAKEWORK(2, 2)
    #endif
#endif

#ifndef HELIFE_NET_SOCKADDR
    #if defined(__linux__) || defined(__APPLE__)
        #define HELIFE_NET_SOCKADDR struct sockaddr
    #elif defined(_WIN64) || defined(_WIN32)
        #define HELIFE_NET_SOCKADDR SOCKADDR
    #endif
#endif

#include <exception>
#include <string>
#include <vector>

namespace HELIFE_NET_NAMESPACE {

    enum SocketDomain {
        LocalCommunication = AF_LOCAL,
        Ipv4 = AF_INET,
        Ipv6 = AF_INET6,
        NetLink = AF_NETLINK,
        Packet = AF_PACKET
    };

    enum SocketType {
        Stream = SOCK_STREAM,
        Datagrams = SOCK_DGRAM,
        Raw = SOCK_RAW
    };

    class Error : public std::exception {
    private:
        const std::string _msg;
    public:
        Error(const std::string& msg)
            : _msg(msg)
        {}

        const char *what() const noexcept override
        { return _msg.c_str(); }
    };

    class SocketError : public Error {
    public:
        SocketError(const std::string& msg)
            : Error(msg)
        {}
    };

    class SetupError : public Error {
    public:
        SetupError(const std::string& msg)
            : Error(msg)
        {}
    };

    enum SocketProtocol {
        Default = 0,
    };

    class Socket {
    private:
        HELIFE_NET_SOCKET _sock;

    public:
        Socket(const SocketDomain& domain, const SocketType& type,
               const SocketProtocol& protocol=SocketProtocol::Default) {
            _sock = ::socket(domain, type, protocol);
            if (_sock == HELIFE_NET_INVALID_SOCKET) {
                HELIFE_NET_THROW_LAST_ERROR(SocketError, "Could not intialize socket: ");
            }
        }

        operator HELIFE_NET_SOCKET() { return _sock; }

        ~Socket()
        {
#if defined(_WIN32) || defined(_WIN64)
            closesocket(_sock);
#elif defined(__linux__) || defined(__APPLE__)
            close(_sock);
#endif
        }
    };

    static inline void setupNetworking()
    {
#if defined(_WIN32) || defined(_WIN64)
        WSADATA wsaData = { 0 };
        iResult = WSAStartup(HELIFE_NET_WINDOWS_SOCKET_VERSION_WSA_STARTUP, &wsaData);
        if (iResult != 0) {
            std::string fmt = "Could not start properly WSAStartup (";
            fmt += std::to_string(iResult) + " ): ";
            HELIFE_NET_THROW_LAST_ERROR(SetupError, fmt);
        }
#endif
    }

    static inline void cleanupNetworking()
    {
#if defined(_WIN32) || defined(_WIN64)
        WSACleanup();
#endif
    }

    class Client {
    private:
        HELIFE_NET_SOCKET _sock;

    public:
        Client() = default;
        const HELIFE_NET_SOCKET& getSocket() const { return _sock; }
    };

    class Tcp {
    private:
        Socket _server_sock;
        struct sockaddr_in saServer;
        std::vector<Client> _clients;

    public:
        class InvalidIp : public Error {
        public:
            InvalidIp(const std::string& msg) : Error(msg) {}
        };

        class BindError : public Error {
        public:
            BindError(const std::string& msg) : Error(msg) {}
        };

        class ListenError : public Error {
        public:
            ListenError(const std::string& msg) : Error(msg) {}
        };

        Tcp(const std::string& hostip="0.0.0.0", const uint16_t& port=8080, const uint32_t& maxBacklog=5)
            : _server_sock(SocketDomain::Ipv4, SocketType::Stream, SocketProtocol::Default)
        {
            saServer.sin_family = SocketDomain::Ipv4;
            saServer.sin_port = ::htons(port);

            if ((saServer.sin_addr.s_addr = ::inet_addr(hostip.c_str())) == 0)
                throw InvalidIp("TCP (ipv4): Invalid ip given");
            else if (::bind(_server_sock, (HELIFE_NET_SOCKADDR *)&saServer, sizeof(saServer)) == HELIFE_NET_SOCKET_ERROR)
                HELIFE_NET_THROW_LAST_ERROR(BindError, "Could not bind Server Socket TCP (ipv4): ");
            else if (::listen(_server_sock, maxBacklog) == HELIFE_NET_SOCKET_ERROR)
                HELIFE_NET_THROW_LAST_ERROR(ListenError, "Could not bind listen on the Socket TCP (ipv4): ");
        }
    };

    class Udp {
    };
};
