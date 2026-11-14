/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SslSocket_h__
#define SslSocket_h__

#include "Socket.h"
#include <boost/asio/ssl/stream.hpp>
#include <boost/system/error_code.hpp>

/**
    TLS stream for Socket<T, Stream>, from TrinityCore 6.x (src/server/shared/Networking/SslSocket.h).
    SslContext provides a static boost::asio::ssl::context& instance().
*/
template<class SslContext>
class SslSocket
{
public:
    explicit SslSocket(IoContextTcpSocket&& socket) : _socket(std::move(socket)), _sslSocket(_socket, SslContext::instance())
    {
        _sslSocket.set_verify_mode(boost::asio::ssl::verify_none);
    }

    // adapting the tcp socket api
    void close(boost::system::error_code& error)
    {
        _socket.close(error);
    }

    void shutdown(boost::asio::socket_base::shutdown_type what, boost::system::error_code& shutdownError)
    {
        _sslSocket.shutdown(shutdownError);
        _socket.shutdown(what, shutdownError);
    }

    template<typename MutableBufferSequence, typename ReadHandlerType>
    void async_read_some(MutableBufferSequence const& buffers, ReadHandlerType&& handler)
    {
        _sslSocket.async_read_some(buffers, std::forward<ReadHandlerType>(handler));
    }

    template<typename ConstBufferSequence, typename WriteHandlerType>
    void async_write_some(ConstBufferSequence const& buffers, WriteHandlerType&& handler)
    {
        _sslSocket.async_write_some(buffers, std::forward<WriteHandlerType>(handler));
    }

    template<typename ConstBufferSequence>
    std::size_t write_some(ConstBufferSequence const& buffers, boost::system::error_code& error)
    {
        return _sslSocket.write_some(buffers, error);
    }

    template<typename WaitHandlerType>
    void async_wait(boost::asio::socket_base::wait_type type, WaitHandlerType&& handler)
    {
        _socket.async_wait(type, std::forward<WaitHandlerType>(handler));
    }

    template<typename SettableSocketOption>
    void set_option(SettableSocketOption const& option, boost::system::error_code& error)
    {
        _socket.set_option(option, error);
    }

    IoContextTcpSocket::endpoint_type remote_endpoint() const
    {
        return _socket.remote_endpoint();
    }

    // ssl api
    template<typename HandshakeHandlerType>
    void async_handshake(boost::asio::ssl::stream_base::handshake_type type, HandshakeHandlerType&& handler)
    {
        _sslSocket.async_handshake(type, std::forward<HandshakeHandlerType>(handler));
    }

private:
    IoContextTcpSocket _socket;
    boost::asio::ssl::stream<IoContextTcpSocket&> _sslSocket;
};

#endif // SslSocket_h__
