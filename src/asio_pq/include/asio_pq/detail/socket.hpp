/**
 *	\file
 */

#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/system/error_code.hpp>
#include <libpq-fe.h>
#include <mpark/variant.hpp>
#include <utility>

#if !defined(_WIN32) && defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
#define ASIO_PQ_HAS_LOCAL_SOCKETS
#endif

namespace asio_pq {
namespace detail {

using socket_variant_type = mpark::variant<
	boost::asio::ip::tcp::socket
	#ifdef ASIO_PQ_HAS_LOCAL_SOCKETS
	, boost::asio::local::stream_protocol::socket
	#endif
>;

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

socket_variant_type socket (boost::asio::io_service & ios, PGconn * conn, boost::system::error_code & ec);

}
}
