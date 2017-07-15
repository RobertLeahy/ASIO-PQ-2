#include <asio_pq/error.hpp>

#include <boost/system/error_code.hpp>
#include <string>

namespace asio_pq {

boost::system::error_code make_error_code (error e) noexcept {
	static const class : public boost::system::error_category {
	public:
		virtual const char * name () const noexcept override {
			return "PostgreSQL";
		}
		virtual std::string message (int condition) const override {
			error e = static_cast<error>(condition);
			switch (e) {
			case error::success:
				return "Success";
			case error::connection_bad:
				return "Connection bad";
			case error::polling_failed:
				return "Polling failed";
			default:
				break;
			}
			return "Unknown error";
		}
	} cat;
	return boost::system::error_code(static_cast<int>(e), cat);
}

}
