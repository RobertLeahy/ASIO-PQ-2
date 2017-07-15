/**
 *	\file
 */

#pragma once

#include <libpq-fe.h>

namespace asio_pq {

class result {
private:
	PGresult * handle_;
	void destroy () noexcept;
public:
	result (const result &) = delete;
	result & operator = (const result &) = delete;
	result () noexcept;
	result (result &&) noexcept;
	result & operator = (result &&) noexcept;
	explicit result (PGresult * result) noexcept;
	~result () noexcept;
	PGresult * release () noexcept;
	PGresult * get () const noexcept;
	operator PGresult * () const noexcept;
	explicit operator bool () const noexcept;
};

}
