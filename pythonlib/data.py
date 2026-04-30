from typing import List, Optional
import struct

class Node:
    def __init__(self, index: int, label: int, features: List[float]):
        self.index = index
        self.label = label
        self.features = features
        # Additional fields can be added as needed

class Subgraph:
    def __init__(self, nodes: Optional[List[Node]] = None, nlabels: int = 0, nfeats: int = 0):
        self.nodes = nodes if nodes is not None else []
        self.nlabels = nlabels
        self.nfeats = nfeats

    @staticmethod
    def from_opf_file(filepath: str) -> 'Subgraph':
        import struct
        nodes = []
        with open(filepath, 'rb') as f:
            # Read header: 3 integers (samples, labels, features)
            header = f.read(12)
            if len(header) < 12:
                raise ValueError("File too short for OPF header")
            nsamples, nlabels, nfeats = struct.unpack('3i', header)
            for _ in range(nsamples):
                # Each sample: int (index), int (label), nfeats floats
                entry = f.read(8 + 4 * nfeats)
                if len(entry) < 8 + 4 * nfeats:
                    raise ValueError("File too short for OPF sample entry")
                index, label = struct.unpack('2i', entry[:8])
                features = list(struct.unpack(f'{nfeats}f', entry[8:]))
                nodes.append(Node(index, label, features))
            return Subgraph(nodes, nlabels, nfeats)

    def to_opf_file(self, filepath: str):
        import struct
        with open(filepath, 'wb') as f:
            # Write header
            f.write(struct.pack('3i', len(self.nodes), self.nlabels, self.nfeats))
            for node in self.nodes:
                f.write(struct.pack('2i', node.index, node.label))
                f.write(struct.pack(f'{self.nfeats}f', *node.features))
