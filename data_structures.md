# LibOPF Data Structures

This document provides a detailed explanation of the main data structures used in the LibOPF project.

## Core Data Structures

These are the main data structures used to represent graphs and nodes.

### `SNode`

Represents a node in the graph.

| Field | Type | Description |
|---|---|---|
| `pathval` | `float` | The cost of the path from the root to this node. |
| `dens` | `float` | The density of the node, used in unsupervised classification. |
| `radius` | `float` | Maximum distance among the k-nearest neighbors in the training set. |
| `label` | `int` | The label assigned to the node by the classifier. |
| `root` | `int` | The root node of the path this node belongs to. |
| `pred` | `int` | The predecessor node in the path. |
| `truelabel` | `int` | The true label of the node, if known. |
| `position` | `int` | The index of the node in the feature space. |
| `feat` | `float *` | A pointer to the feature vector of the node. |
| `status` | `char` | A flag indicating if the node is a prototype (1) or not (0). |
| `relevant` | `char` | A flag indicating if the node is relevant (1) or not (0). |
| `nplatadj`| `int`| The number of adjacent nodes on plateaus. |
| `adj` | `Set *` | An adjacency list for k-NN graphs. |

### `Subgraph`

Represents a graph, which can be a training set, a test set, or a complete dataset.

| Field | Type | Description |
|---|---|---|
| `node` | `SNode *` | An array of nodes belonging to the subgraph. |
| `nnodes` | `int` | The number of nodes in the subgraph. |
| `nfeats` | `int` | The number of features in the feature vectors of the nodes. |
| `bestk` | `int` | The number of adjacent nodes for k-NN graphs. |
| `nlabels` | `int` | The number of labels (classes) in the subgraph. |
| `df` | `float` | The radius in the feature space for density computation. |
| `mindens` | `float` | The minimum density value in the subgraph. |
| `maxdens` | `float` | The maximum density value in the subgraph. |
| `K` | `float` | A constant for the opf_PDF computation. |
| `ordered_list_of_nodes` | `int *` | A list of nodes ordered by increasing cost, used for speeding up supervised classification. |

## Utility Data Structures

These are helper data structures used for various algorithms.

### `GQueue`

A priority queue implemented using a circular queue and doubly linked lists.

| Field | Type | Description |
|---|---|---|
| `C` | `GCircularQueue` | The circular queue part of the priority queue. |
| `L` | `GDoublyLinkedLists` | The doubly linked lists part of the priority queue. |

### `GCircularQueue`

A circular queue used within the `GQueue`.

| Field | Type | Description |
|---|---|---|
| `first` | `int *` | An array with the first elements of each doubly-linked list. |
| `last` | `int *` | An array with the last elements of each doubly-linked list. |
| `nbuckets` | `int` | The number of buckets in the circular queue. |
| `minvalue` | `int` | The minimum value of a node in the queue. |
| `maxvalue` | `int` | The maximum value of a node in the queue. |
| `tiebreak` | `char` | The tie-breaking policy (FIFO or LIFO). |
| `removal_policy` | `char` | The removal policy (MINVALUE or MAXVALUE). |

### `GDoublyLinkedLists`

Doubly linked lists used within the `GQueue`.

| Field | Type | Description |
|---|---|---|
| `elem` | `GQNode *` | An array of all possible doubly-linked lists. |
| `nelems` | `int` | The total number of elements. |
| `value` | `int *` | The value of the nodes in the graph. |

### `GQNode`

A node in a doubly linked list used within `GQueue`.

| Field | Type | Description |
|---|---|---|
| `next` | `int` | The index of the next node. |
| `prev` | `int` | The index of the previous node. |
| `color` | `char` | The color of the node (WHITE, GRAY, or BLACK) for tracking visited status. |

### `RealHeap`

A binary heap for floating-point costs.

| Field | Type | Description |
|---|---|---|
| `cost` | `float *` | An array of costs associated with the elements. |
| `color` | `char *` | An array of colors for tracking the status of elements (e.g., visited). |
| `pixel` | `int *` | An array of pixel indices. |
| `pos` | `int *` | An array of positions of the elements in the heap. |
| `last` | `int` | The index of the last element in the heap. |
| `n` | `int` | The total number of elements in the heap. |
| `removal_policy` | `char` | The removal policy (MINVALUE or MAXVALUE). |

### `Set`

A simple linked list-based set for storing integers.

| Field | Type | Description |
|---|---|---|
| `elem` | `int` | The integer element stored in the set node. |
| `next` | `struct _set *` | A pointer to the next element in the set. |

### `SgCTree`

A component tree built from a `Subgraph`.

| Field | Type | Description |
|---|---|---|
| `node` | `SgCTNode *` | An array of nodes in the component tree. |
| `cmap` | `int *` | A component map. |
| `root` | `int` | The root node of the tree. |
| `numnodes` | `int` | The number of nodes in the tree. |

### `SgCTNode`

A node in a `SgCTree`.

| Field | Type | Description |
|---|---|---|
| `level` | `int` | The gray level of the node. |
| `comp` | `int` | The representative pixel of this node. |
| `dad` | `int` | The parent node in the tree. |
| `son` | `int *` | An array of child nodes. |
| `numsons` | `int` | The number of child nodes. |
| `size` | `int` | The number of pixels in the node. |
