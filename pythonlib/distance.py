from typing import List
import math

def euclidean_distance(a: List[float], b: List[float]) -> float:
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))

def chi_square_distance(a: List[float], b: List[float]) -> float:
    return sum(((x - y) ** 2) / (x + y) if (x + y) != 0 else 0 for x, y in zip(a, b))

def compute_distance_matrix(features: List[List[float]], metric: str = 'euclidean') -> List[List[float]]:
    n = len(features)
    dist = [[0.0] * n for _ in range(n)]
    for i in range(n):
        for j in range(i + 1, n):
            if metric == 'euclidean':
                d = euclidean_distance(features[i], features[j])
            elif metric == 'chi2':
                d = chi_square_distance(features[i], features[j])
            else:
                raise ValueError(f"Unknown metric: {metric}")
            dist[i][j] = dist[j][i] = d
    return dist
