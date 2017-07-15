#include <asio_pq/result.hpp>

#include <libpq-fe.h>
#include <utility>

namespace asio_pq {

void result::destroy () noexcept {
	if (!handle_) return;
	PQclear(handle_);
	handle_ = nullptr;
}

result::result () noexcept : handle_(nullptr) {	}

result::result (result && other) noexcept : handle_(other.handle_) {
	other.handle_ = nullptr;
}

result & result::operator = (result && other) noexcept {
	destroy();
	using std::swap;
	swap(handle_, other.handle_);
	return *this;
}

result::result (PGresult * result) noexcept : handle_(result) {	}

result::~result () noexcept {
	destroy();
}

PGresult * result::release () noexcept {
	PGresult * retr = handle_;
	handle_ = nullptr;
	return retr;
}

PGresult * result::get () const noexcept {
	return handle_;
}

result::operator PGresult * () const noexcept {
	return get();
}

result::operator bool () const noexcept {
	return bool(get());
}

}
