# apps/CMakeLists.txt

# Find all application sources
file(GLOB APP_SOURCES "*.cpp")

# Create an executable for each source file
foreach(APP_SOURCE ${APP_SOURCES})
    # Get the filename without extension to use as the executable name
    get_filename_component(APP_NAME ${APP_SOURCE} NAME_WE)

    # Add the executable target
    add_executable(${APP_NAME} ${APP_SOURCE})

    # Link the executable against the opf library
    target_link_libraries(${APP_NAME} PRIVATE pyopf_static)
endforeach()
