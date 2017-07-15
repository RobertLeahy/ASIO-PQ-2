configure_file(config.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/config.hpp" ESCAPE_QUOTES)
add_executable(asio_pq_tests
	connect.cpp
	get_result.cpp
	main.cpp
)
target_include_directories(asio_pq_tests
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(asio_pq_tests
	asio_pq
	Catch
)
add_test(NAME asio_pq COMMAND asio_pq_tests)
