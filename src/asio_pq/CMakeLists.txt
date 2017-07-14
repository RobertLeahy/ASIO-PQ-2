add_library(asio_pq INTERFACE)
target_include_directories(asio_pq
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}/include
)
add_subdirectory(tests)
