# opfpy_cython.pxd
# Cython-facing declarations for the opfpy_cython extension module.
# Other Cython modules can `cimport` from this file to get statically-typed
# access to the Node and Subgraph wrappers without going through Python dicts.

cdef class Node:
    cdef object _node  # opfpy.Node instance
    @staticmethod
    cdef Node _from_opfpy(object node)

cdef class Subgraph:
    cdef object _sg  # opfpy.Subgraph instance
    @staticmethod
    cdef Subgraph _from_opfpy(object sg)
