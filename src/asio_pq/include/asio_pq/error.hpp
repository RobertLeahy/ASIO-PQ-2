/**
 *	\file
 */

#pragma once

#include <boost/system/error_code.hpp>
#include <type_traits>

namespace asio_pq {

/**
 *	Encapsulates various errors returned by
 *	the libpq API.
 */
enum class error {
	success = 0,
	connection_bad,
	polling_failed
};

boost::system::error_code make_error_code (error e) noexcept;

}

namespace boost {
namespace system {

template <>
struct is_error_code_enum<asio_pq::error> : public std::true_type {	};

}
}
