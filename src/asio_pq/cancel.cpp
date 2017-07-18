#include <asio_pq/cancel.hpp>

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

namespace asio_pq {

void cancel (connection & conn, boost::system::error_code & ec) noexcept {
	ec.clear();
	conn.cancel(ec);
}

void cancel (connection & conn) {
	boost::system::error_code ec;
	cancel(conn, ec);
	if (ec) throw boost::system::system_error(ec);
}

}
