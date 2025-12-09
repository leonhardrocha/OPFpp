import os
import sys
import numpy as np
import matplotlib.pyplot as plt

# Add the build directory to the path to find the pyopf module.
# This path is relative to this script's new location.
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'build'))
try:
    import pyopf
except ImportError:
    print("Error: Could not import the 'pyopf' module.")
    print("Please ensure you have built the project and that the path to the build directory is correct.")
    sys.exit(1)


def setup_and_cluster(data_file="unsupervised_data.txt", k_max=20):
    """
    Creates a dummy dataset if needed and runs the OPF clustering algorithm.
    Returns the clustered graph.
    """
    if not os.path.exists(data_file):
        print(f"Creating dummy data file: {data_file}")
        with open(data_file, "w") as f:
            f.write("100 0 2\n")  # 100 samples, 0 labels (unsupervised), 2 features
            for i in range(100):
                # Create two distinct blobs of data for clear visualization
                cx, cy = (20, 80) if i < 50 else (80, 20)
                # Add some random noise
                x = cx + np.random.randn() * 8
                y = cy + np.random.randn() * 8
                f.write(f"{i} 0 {x:.2f} {y:.2f}\n")

    print(f"Loading data from {data_file}...")
    graph = pyopf.Subgraph.read_from_text(data_file)

    print(f"\nPerforming clustering with k_max={k_max}...")
    # The graph object is modified in-place
    graph.cluster(k_max=k_max)

    num_clusters = graph.nlabels
    print(f"\nClustering complete. Found {num_clusters} clusters.")

    return graph


def visualize_clusters(graph: pyopf.Subgraph, output_file: str = None):
    """
    Creates a 2D scatter plot of the clustering results using matplotlib.
    Saves the plot to a file if a filename is provided.
    """
    if not graph or not graph.nodes:
        print("Graph is empty, nothing to visualize.")
        return

    if graph.nfeats < 2:
        print("Cannot create a 2D scatter plot with less than 2 features.")
        return

    features = np.array([node.feat for node in graph.nodes])
    labels = np.array([node.label for node in graph.nodes])

    unique_labels = np.unique(labels)
    colors = plt.cm.jet(np.linspace(0, 1, len(unique_labels)))

    plt.figure(figsize=(12, 9))

    for label, color in zip(unique_labels, colors):
        cluster_indices = (labels == label)
        cluster_points = features[cluster_indices]

        plt.scatter(cluster_points[:, 0], cluster_points[:, 1],
                    color=color, label=f'Cluster {label}',
                    alpha=0.8, edgecolors='k', s=50)

    plt.title('OPF Clustering Results', fontsize=16)
    plt.xlabel('Feature 1', fontsize=12)
    plt.ylabel('Feature 2', fontsize=12)
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.6)

    if output_file:
        plt.savefig(output_file)
        print(f"Plot saved to {output_file}")
    else:
        plt.show()


if __name__ == "__main__":
    if len(sys.argv) not in [1, 2, 3]:
        print("Usage: python visualize_cluster.py [input_data_file] [output_image_file]")
        sys.exit(1)

    input_file = sys.argv[1] if len(sys.argv) > 1 else "unsupervised_data.txt"
    output_file = sys.argv[2] if len(sys.argv) > 2 else None

    clustered_graph = setup_and_cluster(data_file=input_file)
    visualize_clusters(clustered_graph, output_file=output_file)