# opfpy_cython.pyx
# Cython wrapper layer on top of the opfpy pybind11 extension.
# Provides statically-typed cdef classes that delegate to opfpy objects,
# allowing downstream Cython code to `cimport opfpy_cython` for zero-overhead
# attribute access and typed call sites.

# distutils: language = c++

import sys as _sys
import os as _os

# Ensure pythonlib/bin is on the path so opfpy can be imported
_bin_dir = _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), '..', 'bin')
if _bin_dir not in _sys.path:
    _sys.path.insert(0, _os.path.normpath(_bin_dir))

# On Windows/MSYS2, ensure GCC runtime DLLs are accessible
if _os.name == 'nt':
    _ucrt_bin = r'D:\msys64\ucrt64\bin'
    if _os.path.isdir(_ucrt_bin):
        _os.add_dll_directory(_ucrt_bin)

import opfpy as _opfpy


# ---------------------------------------------------------------------------
# Node wrapper
# ---------------------------------------------------------------------------

cdef class Node:
    """Cython wrapper around opfpy.Node (backed by C++ Node<float>)."""

    def __init__(self):
        self._node = _opfpy.Node()

    @staticmethod
    cdef Node _from_opfpy(object node):
        cdef Node n = Node.__new__(Node)
        n._node = node
        return n

    # --- Properties ---

    @property
    def pathval(self):
        return self._node.pathval

    @pathval.setter
    def pathval(self, float v):
        self._node.pathval = v

    @property
    def dens(self):
        return self._node.dens

    @dens.setter
    def dens(self, float v):
        self._node.dens = v

    @property
    def radius(self):
        return self._node.radius

    @radius.setter
    def radius(self, float v):
        self._node.radius = v

    @property
    def label(self):
        return self._node.label

    @label.setter
    def label(self, int v):
        self._node.label = v

    @property
    def root(self):
        return self._node.root

    @root.setter
    def root(self, int v):
        self._node.root = v

    @property
    def pred(self):
        return self._node.pred

    @pred.setter
    def pred(self, int v):
        self._node.pred = v

    @property
    def truelabel(self):
        return self._node.truelabel

    @truelabel.setter
    def truelabel(self, int v):
        self._node.truelabel = v

    @property
    def position(self):
        return self._node.position

    @position.setter
    def position(self, int v):
        self._node.position = v

    @property
    def status(self):
        return self._node.status

    @status.setter
    def status(self, int v):
        self._node.status = v

    @property
    def relevant(self):
        return self._node.relevant

    @relevant.setter
    def relevant(self, int v):
        self._node.relevant = v

    @property
    def nplatadj(self):
        return self._node.nplatadj

    @nplatadj.setter
    def nplatadj(self, int v):
        self._node.nplatadj = v

    @property
    def feat(self):
        return self._node.feat

    @feat.setter
    def feat(self, list v):
        self._node.feat = v

    @property
    def adj(self):
        return self._node.adj

    @adj.setter
    def adj(self, list v):
        self._node.adj = v

    def add_to_adj(self, int node_id):
        self._node.add_to_adj(node_id)

    def clear_adj(self):
        self._node.clear_adj()

    # Allow the raw opfpy.Node to be retrieved for passing to free functions
    @property
    def _raw(self):
        return self._node


# ---------------------------------------------------------------------------
# Subgraph wrapper
# ---------------------------------------------------------------------------

cdef class Subgraph:
    """Cython wrapper around opfpy.Subgraph (backed by C++ Subgraph<float>)."""

    def __init__(self, int nnodes=0):
        if nnodes > 0:
            self._sg = _opfpy.Subgraph(nnodes)
        else:
            self._sg = _opfpy.Subgraph()

    @staticmethod
    cdef Subgraph _from_opfpy(object sg):
        cdef Subgraph s = Subgraph.__new__(Subgraph)
        s._sg = sg
        return s

    # --- Properties ---

    @property
    def nfeats(self):
        return self._sg.nfeats

    @nfeats.setter
    def nfeats(self, int v):
        self._sg.nfeats = v

    @property
    def bestk(self):
        return self._sg.bestk

    @bestk.setter
    def bestk(self, int v):
        self._sg.bestk = v

    @property
    def nlabels(self):
        return self._sg.nlabels

    @nlabels.setter
    def nlabels(self, int v):
        self._sg.nlabels = v

    @property
    def df(self):
        return self._sg.df

    @df.setter
    def df(self, float v):
        self._sg.df = v

    @property
    def mindens(self):
        return self._sg.mindens

    @mindens.setter
    def mindens(self, float v):
        self._sg.mindens = v

    @property
    def maxdens(self):
        return self._sg.maxdens

    @maxdens.setter
    def maxdens(self, float v):
        self._sg.maxdens = v

    @property
    def K(self):
        return self._sg.K

    @K.setter
    def K(self, int v):
        self._sg.K = v

    @property
    def nnodes(self):
        return self._sg.nnodes

    # --- Node access ---

    def get_node(self, int idx) -> Node:
        return Node._from_opfpy(self._sg.get_node(idx))

    def add_node(self, node):
        """Add a Node (opfpy.Node or opfpy_cython.Node) to the subgraph."""
        if isinstance(node, Node):
            self._sg.add_node((<Node>node)._node)
        else:
            self._sg.add_node(node)

    def add_ordered_node(self, int idx):
        self._sg.add_ordered_node(idx)

    def clear_ordered_list_of_nodes(self):
        self._sg.clear_ordered_list_of_nodes()

    @property
    def ordered_list_of_nodes(self):
        return self._sg.ordered_list_of_nodes

    # --- Model I/O ---

    def write_model(self, str filename):
        self._sg.write_model(filename)

    @staticmethod
    def read_model(str filename) -> Subgraph:
        return Subgraph._from_opfpy(_opfpy.Subgraph.read_model(filename))

    @staticmethod
    def from_original_file(str filename) -> Subgraph:
        return Subgraph._from_opfpy(_opfpy.Subgraph.from_original_file(filename))

    # Allow the raw opfpy.Subgraph to be retrieved
    @property
    def _raw(self):
        return self._sg


# ---------------------------------------------------------------------------
# OPF wrapper
# ---------------------------------------------------------------------------

cdef class OPF:
    """Cython wrapper around opfpy.OPF (backed by C++ OPF<float>)."""

    def __init__(self):
        self._opf = _opfpy.OPF()

    def train(self, subgraph):
        if isinstance(subgraph, Subgraph):
            self._opf.train((<Subgraph>subgraph)._sg)
        else:
            self._opf.train(subgraph)

    def classify(self, train_subgraph, test_subgraph):
        if isinstance(train_subgraph, Subgraph):
            train_subgraph = (<Subgraph>train_subgraph)._sg
        if isinstance(test_subgraph, Subgraph):
            test_subgraph = (<Subgraph>test_subgraph)._sg
        self._opf.classify(train_subgraph, test_subgraph)

    def learn(self, train_subgraph, eval_subgraph, int n_iterations=10):
        if isinstance(train_subgraph, Subgraph):
            train_subgraph = (<Subgraph>train_subgraph)._sg
        if isinstance(eval_subgraph, Subgraph):
            eval_subgraph = (<Subgraph>eval_subgraph)._sg
        self._opf.learn(train_subgraph, eval_subgraph, n_iterations)

    def accuracy(self, subgraph):
        if isinstance(subgraph, Subgraph):
            subgraph = (<Subgraph>subgraph)._sg
        return self._opf.accuracy(subgraph)

    def create_arcs(self, subgraph, int knn):
        """Build k-NN adjacency and radius in native C++ (opf_CreateArcs)."""
        if isinstance(subgraph, Subgraph):
            subgraph = (<Subgraph>subgraph)._sg
        self._opf.create_arcs(subgraph, knn)

    def destroy_arcs(self, subgraph):
        if isinstance(subgraph, Subgraph):
            subgraph = (<Subgraph>subgraph)._sg
        self._opf.destroy_arcs(subgraph)

    def compute_pdf(self, subgraph):
        if isinstance(subgraph, Subgraph):
            subgraph = (<Subgraph>subgraph)._sg
        self._opf.compute_pdf(subgraph)

    def bestk_min_cut(self, subgraph, int kmin, int kmax):
        """Run native best-k normalized-cut search (opf_BestkMinCut)."""
        if isinstance(subgraph, Subgraph):
            subgraph = (<Subgraph>subgraph)._sg
        self._opf.bestk_min_cut(subgraph, kmin, kmax)

    def cluster(self, subgraph):
        if isinstance(subgraph, Subgraph):
            subgraph = (<Subgraph>subgraph)._sg
        self._opf.cluster(subgraph)

    def knn_classify(self, train_subgraph, test_subgraph):
        if isinstance(train_subgraph, Subgraph):
            train_subgraph = (<Subgraph>train_subgraph)._sg
        if isinstance(test_subgraph, Subgraph):
            test_subgraph = (<Subgraph>test_subgraph)._sg
        self._opf.knn_classify(train_subgraph, test_subgraph)

    def semi_supervised(self, labeled_subgraph, unlabeled_subgraph, eval_subgraph=None):
        if isinstance(labeled_subgraph, Subgraph):
            labeled_subgraph = (<Subgraph>labeled_subgraph)._sg
        if isinstance(unlabeled_subgraph, Subgraph):
            unlabeled_subgraph = (<Subgraph>unlabeled_subgraph)._sg
        if isinstance(eval_subgraph, Subgraph):
            eval_subgraph = (<Subgraph>eval_subgraph)._sg
        result = self._opf.semi_supervised(labeled_subgraph, unlabeled_subgraph, eval_subgraph)
        return Subgraph._from_opfpy(result)

    def normalize(self, subgraph):
        if isinstance(subgraph, Subgraph):
            subgraph = (<Subgraph>subgraph)._sg
        self._opf.normalize(subgraph)

    def pruning(self, train_subgraph, eval_subgraph, float desired_accuracy):
        if isinstance(train_subgraph, Subgraph):
            train_subgraph = (<Subgraph>train_subgraph)._sg
        if isinstance(eval_subgraph, Subgraph):
            eval_subgraph = (<Subgraph>eval_subgraph)._sg
        return self._opf.pruning(train_subgraph, eval_subgraph, desired_accuracy)

    @property
    def _raw(self):
        return self._opf


# ---------------------------------------------------------------------------
# Free functions
# ---------------------------------------------------------------------------

def read_subgraph(str filename) -> Subgraph:
    """Read a Subgraph from OPF training binary format."""
    return Subgraph._from_opfpy(_opfpy.read_subgraph(filename))


def write_subgraph(str filename, subgraph):
    """Write a Subgraph to OPF training binary format."""
    if isinstance(subgraph, Subgraph):
        _opfpy.write_subgraph(filename, (<Subgraph>subgraph)._sg)
    else:
        _opfpy.write_subgraph(filename, subgraph)


def hello() -> str:
    return _opfpy.hello()
