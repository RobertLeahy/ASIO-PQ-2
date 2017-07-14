#include <asio_pq/handle.hpp>

#include <libpq-fe.h>
#include <utility>

namespace asio_pq {

void handle::destroy () noexcept {
	if (!conn_) return;
	PQfinish(conn_);
	conn_ = nullptr;
}

handle::handle () noexcept : conn_(nullptr) {	}

handle::handle (handle && other) noexcept : conn_(other.conn_) {
	other.conn_ = nullptr;
}

handle & handle::operator = (handle && rhs) noexcept {
	destroy();
	using std::swap;
	swap(conn_, rhs.conn_);
	return *this;
}

handle::handle (PGconn * conn) noexcept : conn_(conn) {	}

handle::~handle () noexcept {
	destroy();
}

PGconn * handle::release () noexcept {
	PGconn * retr = nullptr;
	using std::swap;
	swap(retr, conn_);
	return retr;
}

PGconn * handle::get() const noexcept {
	return conn_;
}

handle::operator PGconn * () const noexcept {
	return get();
}

handle::operator bool () const noexcept {
	return bool(get());
}

}
