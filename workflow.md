# LibOPF Workflow

This document provides a summary of the workflow of each `opf_*.c` program in the `src` directory.

## `opf_accuracy`

**Purpose:** Computes the classification accuracy of a dataset that has been previously classified.

**Workflow:**

1.  **Read Data:** Reads a `Subgraph` from the specified dataset file.
2.  **Read Classification Results:** Reads the classification results from a `.out` file, updating the `label` field of each `SNode` in the `Subgraph`.
3.  **Compute Accuracy:** Calls the `opf_Accuracy` function to compare the predicted `label` with the `truelabel` for each node and calculate the percentage of correct classifications.
4.  **Output:** Prints the accuracy to the console and appends it to a `.acc` file.

## `opf_classify`

**Purpose:** Classifies a test dataset using a trained OPF classifier model.

**Workflow:**

1.  **Read Data:**
    *   Reads the test `Subgraph` from the specified test set file.
    *   Reads the trained classifier model from `classifier.opf` into another `Subgraph` structure.
2.  **Precomputed Distances (Optional):** If a precomputed distance file is provided, it reads the distance matrix.
3.  **Classify:** Calls the `opf_OPFClassifying` function, which iterates through each node in the test `Subgraph` and assigns it a label based on the trained classifier.
4.  **Output:**
    *   Writes the predicted labels for the test set to a `.out` file.
    *   Records the execution time in a `.time` file.

## `opf_cluster`

**Purpose:** Performs unsupervised clustering on a dataset.

**Workflow:**

1.  **Read Data:** Reads an unlabeled `Subgraph` from the specified dataset file.
2.  **Precomputed Distances (Optional):** If a precomputed distance file is provided, it reads the distance matrix.
3.  **Find Best `k`:** Calls `opf_BestkMinCut` to find the best number of neighbors (`k`) for the k-NN graph by minimizing the normalized cut.
4.  **Filter Clusters (Optional):** If specified, it filters the resulting clusters by height, area, or volume.
5.  **Cluster:** Calls `opf_OPFClustering` to perform the clustering, assigning a `label` to each `SNode`.
6.  **Label Propagation:**
    *   If the dataset has true labels, it propagates the true label of the root of each cluster to all nodes in that cluster.
    *   If the dataset is unlabeled, it copies the cluster labels to the `truelabel` field.
7.  **Output:**
    *   Saves the resulting model (including the clustered `Subgraph`) to `classifier.opf`.
    *   Writes the cluster labels to a `.out` file.

## `opf_distance`

**Purpose:** Precomputes a distance matrix for a given dataset and saves it to a file.

**Workflow:**

1.  **Read Data:** Reads a `Subgraph` from the specified dataset file.
2.  **Compute Distances:** Based on the user-selected distance metric (e.g., Euclidean, Chi-Square), it computes the distance between every pair of nodes in the `Subgraph`.
3.  **Normalization (Optional):** If requested, it normalizes the distances to the range [0, 1].
4.  **Output:** Writes the distance matrix to `distances.dat`.

## `opf_fold`

**Purpose:** Splits a dataset into `k` folds for cross-validation.

**Workflow:**

1.  **Read Data:** Reads a `Subgraph` from the specified dataset file.
2.  **Create Folds:** Calls the `opf_kFoldSubgraph` function to split the `Subgraph` into an array of `k` `Subgraph`s.
3.  **Normalization (Optional):** If requested, it normalizes the features of each fold.
4.  **Output:** Writes each fold to a separate file (e.g., `fold_1.dat`, `fold_2.dat`, etc.).

## `opf_info`

**Purpose:** Displays information about an OPF data file.

**Workflow:**

1.  **Open File:** Opens the specified OPF file in binary mode.
2.  **Read Header:** Reads the number of data points, number of labels, and number of features from the beginning of the file.
3.  **Output:** Prints the extracted information to the console.

## `opf_knn_classify`

**Purpose:** Classifies a test dataset using a trained k-NN classifier model. This is similar to `opf_classify`, but uses the `opf_OPFKNNClassify` function.

**Workflow:**

1.  **Read Data:**
    *   Reads the test `Subgraph` from the specified test set file.
    *   Reads the trained classifier model from `classifier.opf` into another `Subgraph` structure.
2.  **Precomputed Distances (Optional):** If a precomputed distance file is provided, it reads the distance matrix.
3.  **Classify:** Calls the `opf_OPFKNNClassify` function, which classifies the nodes of the test set by using the labels created by `opf_cluster` in the training set.
4.  **Output:**
    *   Writes the predicted labels for the test set to a `.out` file.
    *   Records the execution time in a `.time` file.

## `opf_learn`

**Purpose:** Improves a trained classifier by learning from errors on an evaluation set.

**Workflow:**

1.  **Read Data:**
    *   Reads a training `Subgraph` from the training set file.
    *   Reads an evaluation `Subgraph` from the evaluation set file.
2.  **Precomputed Distances (Optional):** If a precomputed distance file is provided, it reads the distance matrix.
3.  **Learn:** Calls the `opf_OPFLearning` function, which identifies misclassified samples in the evaluation set and adjusts the training set to improve the classifier.
4.  **Output:**
    *   Saves the improved classifier model to `classifier.opf`.
    *   Records the execution time in a `.time` file.

## `opf_merge`

**Purpose:** Merges multiple datasets into a single dataset.

**Workflow:**

1.  **Read Data:** Reads multiple `Subgraph`s from the specified input files.
2.  **Merge:** Repeatedly calls the `opf_MergeSubgraph` function to merge the `Subgraph`s one by one.
3.  **Output:** Writes the final merged `Subgraph` to `merged.dat`.

## `opf_normalize`

**Purpose:** Normalizes the feature vectors of a dataset.

**Workflow:**

1.  **Read Data:** Reads a `Subgraph` from the specified dataset file.
2.  **Normalize:** Calls the `opf_NormalizeFeatures` function to scale the feature vectors, typically to the range [0, 1].
3.  **Output:** Writes the `Subgraph` with normalized features to a new file.

## `opf_pruning`

**Purpose:** Prunes a trained classifier to reduce its size while maintaining a desired accuracy.

**Workflow:**

1.  **Read Data:**
    *   Reads a training `Subgraph` from the training set file.
    *   Reads an evaluation `Subgraph` from the evaluation set file.
2.  **Precomputed Distances (Optional):** If a precomputed distance file is provided, it reads the distance matrix.
3.  **Prune:** Calls the `opf_OPFPruning` function, which removes non-essential nodes from the training set to reduce the model size, while trying to stay above the desired accuracy threshold.
4.  **Output:**
    *   Saves the pruned classifier model to `classifier.opf`.
    *   Records the pruning rate and execution time to files.

## `opf_semi`

**Purpose:** Trains a semi-supervised OPF classifier using both labeled and unlabeled data.

**Workflow:**

1.  **Read Data:**
    *   Reads a labeled `Subgraph` from the labeled training set file.
    *   Reads an unlabeled `Subgraph` from the unlabeled training set file.
    *   Optionally reads an evaluation `Subgraph`.
2.  **Precomputed Distances (Optional):** If a precomputed distance file is provided, it reads the distance matrix.
3.  **Learn:** Calls `opf_OPFSemiLearning` to create a combined `Subgraph` from the labeled and unlabeled data, and then calls `opf_OPFTraining` to train the classifier.
4.  **Output:**
    *   Saves the trained semi-supervised model to `classifier.opf`.
    *   Writes the labels to a `.out` file.
    *   Records the execution time in a `.time` file.

## `opf_split`

**Purpose:** Splits a single dataset into training, evaluation, and test sets.

**Workflow:**

1.  **Read Data:** Reads a `Subgraph` from the specified dataset file.
2.  **Normalization (Optional):** If requested, it normalizes the features of the entire dataset.
3.  **Split:** Calls the `opf_SplitSubgraph` function to divide the main `Subgraph` into smaller `Subgraph`s for training, evaluation, and testing based on the specified percentages.
4.  **Output:** Writes the training, evaluation (if any), and testing `Subgraph`s to `training.dat`, `evaluating.dat`, and `testing.dat` respectively.

## `opf_train`

**Purpose:** Trains an OPF classifier from a training dataset.

**Workflow:**

1.  **Read Data:** Reads a training `Subgraph` from the specified training set file.
2.  **Precomputed Distances (Optional):** If a precomputed distance file is provided, it reads the distance matrix.
3.  **Train:** Calls the `opf_OPFTraining` function, which builds the Optimum-Path Forest on the `Subgraph`. This involves finding the best paths from prototype nodes and assigning labels accordingly.
4.  **Output:**
    *   Saves the trained model to `classifier.opf`.
    *   Writes the labels assigned during training to a `.out` file.
    *   Records the training time in a `.time` file.
