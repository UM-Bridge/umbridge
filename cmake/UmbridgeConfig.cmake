# Provide umbridge using this folder/repository
# If the variable UMBRIDGE_USE_SSL evaluates to true,
# UMBRIDGE_USE_SSL directly links to ssl and crypto
find_package(Threads REQUIRED)

add_library(Umbridge::Umbridge IMPORTED INTERFACE)

if(UMBRIDGE_USE_SSL)
  target_link_libraries(Umbridge::Umbridge INTERFACE Threads::Threads ssl crypto)
  target_include_directories(Umbridge::Umbridge INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../lib)
  target_compile_definitions(Umbridge::Umbridge INTERFACE CPPHTTPLIB_OPENSSL_SUPPORT)
else()
  target_link_libraries(Umbridge::Umbridge INTERFACE Threads::Threads)
  target_include_directories(Umbridge::Umbridge INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../lib)
endif()
