#include <asio_pq/detail/socket.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>
#include <libpq-fe.h>
#include <cstring>

#ifdef _WIN32
#include <Winsock2.h>
#include <Windows.h>
#else
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace asio_pq {
namespace detail {

socket_type get_socket_type (PGconn * conn, boost::system::error_code & ec) noexcept {
	ec.clear();
	socket_type retr = socket_type::tcp_ip_v4;
	int handle = PQsocket(conn);
	if (handle == -1) {
		ec = make_error_code(boost::system::errc::not_a_socket);
		return retr;
	}
	struct sockaddr_storage addr;
	std::memset(&addr, 0, sizeof(addr));
	#ifdef _WIN32
	int
	#else
	socklen_t
	#endif
	size = sizeof(addr);
	if (getsockname(
		handle,
		reinterpret_cast<struct sockaddr *>(&addr),
		&size
	) == -1) {
		ec.assign(
			#ifdef _WIN32
			GetLastError()
			#else
			errno
			#endif
			, boost::system::system_category()
		);
		return retr;
	}
	switch (addr.ss_family) {
	case AF_INET:
		break;	//	Already correct
	case AF_INET6:
		retr = socket_type::tcp_ip_v6;
		break;
	#ifdef AF_UNIX
	case AF_UNIX:
	#endif
	#if defined(AF_LOCAL) && (!defined(AF_UNIX) || (AF_LOCAL != AF_UNIX))
	case AF_LOCAL:
	#endif
		#if defined(AF_LOCAL) || defined(AF_UNIX)
		retr = socket_type::unix_domain;
		break;
		#endif
	default:
		ec = make_error_code(boost::system::errc::not_supported);
		break;
	}
	return retr;
}

static void close_if (int handle, boost::system::error_code ec) noexcept {
	if (!ec) return;
	#ifdef _WIN32
	closesocket(handle);
	#else
	close(handle);
	#endif
}

boost::asio::ip::tcp::socket tcp_socket (
	boost::asio::io_service & ios,
	PGconn * conn,
	bool v4,
	boost::system::error_code & ec
) noexcept {
	ec.clear();
	boost::asio::ip::tcp::socket retr(ios);
	int handle = PQsocket(conn);
	#ifdef _WIN32
	WSAPROTOCOL_INFOW info;
	if (WSADuplicateSocketW(
		handle,
		GetProcessId(GetCurrentProcess()),
		&info
	) == SOCKET_ERROR) {
		ec.assign(GetLastError(), boost::system::system_category());
		return retr;
	}
	auto dup = WSASocketW(
		info.iAddressFamily,
		info.iSocketType,
		info.iProtocol,
		&info,
		0,
		WSA_FLAG_OVERLAPPED
	);
	if (dup == INVALID_SOCKET) {
		ec.assign(GetLastError(), boost::system::system_category());
		return retr;
	}
	#else
	auto dup = ::dup(handle);
	if (dup == -1) {
		ec.assign(errno, boost::system::system_category());
		return retr;
	}
	#endif
	retr.assign(
		v4 ? boost::asio::ip::tcp::v4() : boost::asio::ip::tcp::v6(),
		dup,
		ec
	);
	close_if(dup, ec);
	return retr;
}

#ifdef ASIO_PQ_HAS_LOCAL_SOCKETS
boost::asio::local::stream_protocol::socket local_socket (
	boost::asio::io_service & ios,
	PGconn * conn,
	boost::system::error_code & ec
) noexcept {
	ec.clear();
	boost::asio::local::stream_protocol::socket retr(ios);
	int handle = PQsocket(conn);
	auto dup = ::dup(handle);
	if (dup == -1) {
		ec.assign(errno, boost::system::system_category());
		return retr;
	}
	retr.assign(
		boost::asio::local::stream_protocol{},
		dup,
		ec
	);
	close_if(dup, ec);
	return retr;
}
#endif

}
}
