/**
 *	\file
 */

#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/system/error_code.hpp>
#include <libpq-fe.h>
#include <utility>

#if !defined(_WIN32) && defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
#define ASIO_PQ_HAS_LOCAL_SOCKETS
#endif

namespace asio_pq {
namespace detail {

enum class socket_type {
	tcp_ip_v4,
	tcp_ip_v6,
	unix_domain
};

socket_type get_socket_type (PGconn * conn, boost::system::error_code & ec) noexcept;

boost::asio::ip::tcp::socket tcp_socket (
	boost::asio::io_service & ios,
	PGconn * conn,
	bool v4,
	boost::system::error_code & ec
) noexcept;

#ifdef ASIO_PQ_HAS_LOCAL_SOCKETS
boost::asio::local::stream_protocol::socket local_socket (
	boost::asio::io_service & ios,
	PGconn * conn,
	boost::system::error_code & ec
) noexcept;
#endif

template <typename Handler>
void socket (boost::asio::io_service & ios, PGconn * conn, Handler h) {
	boost::system::error_code ec;
	if (PQstatus(conn) == CONNECTION_BAD) {
		h(ec, boost::asio::ip::tcp::socket(ios));
		return;
	}
	socket_type type = get_socket_type(conn, ec);
	if (!ec) {
		bool v4 = false;
		switch (type) {
		case socket_type::tcp_ip_v4:
			v4 = true;
		case socket_type::tcp_ip_v6:{
			auto tcp = detail::tcp_socket(ios, conn, v4, ec);
			h(ec, std::move(tcp));
			return;
		}
		case socket_type::unix_domain:
			#ifdef ASIO_PQ_HAS_LOCAL_SOCKETS
			{
				auto local = detail::local_socket(ios, conn, ec);
				h(ec, std::move(local));
				return;
			}
			#else
			ec = make_error_code(boost::system::errc::not_supported);
			#endif
			break;
		}
	}
	h(ec, boost::asio::ip::tcp::socket(ios));
}

}
}
