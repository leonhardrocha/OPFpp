# python/CMakeLists.txt

# Create the Python module using pybind11's helper function
pybind11_add_module(pyopf
    opf_bindings.cpp
)

# Link the Python module against our core 'opf' library
target_link_libraries(pyopf PRIVATE pyopf_static)