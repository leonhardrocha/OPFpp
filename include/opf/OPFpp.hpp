#ifndef OPF_OPFPP_HPP
#define OPF_OPFPP_HPP

#include "KernelJointProbability.hpp"
#include "KernelSubGraph.hpp"
#include "OPF.hpp"
#include "StridedSubgraph.hpp"

namespace opf {

    struct KernelBestKResult {
        int kernel_id = -1;
        int bestk = 0;
        float mincut = std::numeric_limits<float>::max();
        float df_at_bestk = 0.0f;
    };

    template<typename T>
    class OPFpp : public OPF<T> {
    public:
        using OPF<T>::clustering;
        using OPF<T>::computePDF;
        using OPF<T>::createArcs;
        using OPF<T>::createArcs2;
        using OPF<T>::destroyArcs;
        using OPF<T>::normalizedCutToKmax;
        using OPF<T>::pdfToKmax;
        using OPF<T>::clusteringToKmax;

        void clustering(KernelSubGraph<T>& ksg) {
            std::vector<float> pathval(ksg.getNumNodes());
            using Elem = std::pair<float, int>;
            std::priority_queue<Elem, std::vector<Elem>> Q;

            for (int p = 0; p < ksg.getNumNodes(); ++p) {
                pathval[p] = ksg.getNode(p).getPathval();
                ksg.getNode(p).setPred(-1); // NIL
                ksg.getNode(p).setRoot(p);
                Q.push({pathval[p], p});
            }

            int l = 0;
            ksg.clearOrderedListOfNodes();
            while (!Q.empty()) {
                int p = Q.top().second;
                Q.pop();
                ksg.addOrderedNode(p);

                if (ksg.getNode(p).getPred() == -1) {
                    pathval[p] = ksg.getNode(p).getDens();
                    ksg.getNode(p).setLabel(l++);
                }

                ksg.getNode(p).setPathval(pathval[p]);

                for (int q : ksg.getKernelAdj(p)) {
                    if (q < 0 || q >= ksg.getNumNodes()) continue; // bounds check
                    float tmp = std::min(pathval[p], ksg.getNode(q).getDens());
                    if (tmp > pathval[q]) {
                        pathval[q] = tmp;
                        ksg.getNode(q).setPred(p);
                        ksg.getNode(q).setRoot(ksg.getNode(p).getRoot());
                        ksg.getNode(q).setLabel(ksg.getNode(p).getLabel());
                        Q.push({pathval[q], q});
                    }
                }
            }
            ksg.setNumLabels(l);
        }

        /// OPF clustering limited to the first bestk neighbors.
        /// Mirrors opf_OPFClusteringToKmax.
        void clusteringToKmax(KernelSubGraph<T>& ksg) {
            const int n    = ksg.getNumNodes();
            const int kmax = ksg.getBestK();

            std::vector<float> pathval(n);
            using Elem = std::pair<float, int>;
            std::priority_queue<Elem, std::vector<Elem>> Q;  // max-heap

            for (int p = 0; p < n; ++p) {
                pathval[p] = ksg.getNode(p).getPathval();
                ksg.getNode(p).setPred(NIL);
                ksg.getNode(p).setRoot(p);
                Q.push({pathval[p], p});
            }

            int l = 0;
            ksg.clearOrderedListOfNodes();
            while (!Q.empty()) {
                auto [pv, p] = Q.top(); Q.pop();
                ksg.addOrderedNode(p);

                if (ksg.getNode(p).getPred() == NIL) {
                    pathval[p] = ksg.getNode(p).getDens();
                    ksg.getNode(p).setLabel(l++);
                }
                ksg.getNode(p).setPathval(pathval[p]);

                const auto& adjList = ksg.getKernelAdj(p);
                const int nadj = kmax + ksg.getNode(p).getNplatadj();
                int k = 0;
                for (int q : adjList) {
                    if (k >= nadj) break;
                    if (q < 0 || q >= n) { ++k; continue; }
                    float tmp = std::min(pathval[p], ksg.getNode(q).getDens());
                    if (tmp > pathval[q]) {
                        pathval[q] = tmp;
                        ksg.getNode(q).setPred(p);
                        ksg.getNode(q).setRoot(ksg.getNode(p).getRoot());
                        ksg.getNode(q).setLabel(ksg.getNode(p).getLabel());
                        Q.push({pathval[q], q});
                    }
                    ++k;
                }
            }
            ksg.setNumLabels(l);
        }

        // ---- Normalized cut -------------------------------------------------

        /// Compute the normalised cut value over the full adjacency graph.
        /// Mirrors opf_NormalizedCut from LibOPF.
        float normalizedCut(KernelSubGraph<T>& ksg) {
            const int n       = ksg.getNumNodes();
            const int nlabels = ksg.getNumLabels();
            std::vector<float> acumIC(nlabels, 0.0f);
            std::vector<float> acumEC(nlabels, 0.0f);

            for (int p = 0; p < n; ++p) {
                for (int q : ksg.getKernelAdj(p)) {
                    float dist = distance::euclDist<T>(*ksg.getNode(p).getFeat(), *ksg.getNode(q).getFeat());
                    if (dist > 0.0f) {
                        int lp = ksg.getNode(p).getLabel();
                        int lq = ksg.getNode(q).getLabel();
                        if (lp == lq)
                            acumIC[lp] += 1.0f / dist;
                        else {
                            acumEC[lp] += 1.0f / dist;
                            acumEC[lq] += 1.0f / dist;
                        }
                    }
                }
            }

            float ncut = 0.0f;
            for (int l = 0; l < nlabels; ++l) {
                float denom = acumIC[l] + acumEC[l];
                if (denom > 0.0f)
                    ncut += acumEC[l] / denom;
            }
            return ncut;
        }

        /// Normalised cut limited to the first bestk neighbors.
        /// Mirrors opf_NormalizedCutToKmax.
        float normalizedCutToKmax(KernelSubGraph<T>& ksg) {
            const int n      = ksg.getNumNodes();
            const int nlabels = ksg.getNumLabels();
            const int kmax   = ksg.getBestK();
            std::vector<float> acumIC(nlabels, 0.0f);
            std::vector<float> acumEC(nlabels, 0.0f);

            for (int p = 0; p < n; ++p) {
                const auto& adjList = ksg.getNode(p).getAdj();
                const int nadj = kmax + ksg.getNode(p).getNplatadj();
                int k = 0;
                for (int q : adjList) {
                    if (k >= nadj) break;
                    float dist = distance::euclDist<T>(*ksg.getNode(p).getFeat(), *ksg.getNode(q).getFeat());
                    if (dist > 0.0f) {
                        int lp = ksg.getNode(p).getLabel();
                        int lq = ksg.getNode(q).getLabel();
                        if (lp == lq)
                            acumIC[lp] += 1.0f / dist;
                        else {
                            acumEC[lp] += 1.0f / dist;
                            acumEC[lq] += 1.0f / dist;
                        }
                    }
                    ++k;
                }
            }

            float ncut = 0.0f;
            for (int l = 0; l < nlabels; ++l) {
                float denom = acumIC[l] + acumEC[l];
                if (denom > 0.0f) ncut += acumEC[l] / denom;
            }
            return ncut;
        }

        std::vector<KernelBestKResult> bestkMinCutPerKernel(
            const std::vector<KernelSubGraph<T>>& kernels,
            int kmin,
            int kmax
        ) {
            std::vector<KernelBestKResult> results;
            results.reserve(kernels.size());

            const int kmin_clamped = std::max(1, kmin);
            const int kmax_clamped = std::max(kmin_clamped, kmax);

            for (size_t idx = 0; idx < kernels.size(); ++idx) {
                Subgraph<T> sg_kernel = kernels[idx].toSubgraph();
                std::vector<float> maxdists = createArcs2(sg_kernel, kmax_clamped);

                int bestk = kmax_clamped;
                float mincut = std::numeric_limits<float>::max();

                for (int k = kmin_clamped; k <= kmax_clamped && mincut != 0.0f; ++k) {
                    sg_kernel.setDf(maxdists[k - 1]);
                    sg_kernel.setBestK(k);

                    pdfToKmax(sg_kernel);
                    clusteringToKmax(sg_kernel);
                    float nc = normalizedCutToKmax(sg_kernel);

                    if (nc < mincut) {
                        mincut = nc;
                        bestk = k;
                    }
                }

                destroyArcs(sg_kernel);
                sg_kernel.setBestK(bestk);
                createArcs(sg_kernel, bestk);
                computePDF(sg_kernel);

                KernelBestKResult out;
                out.kernel_id = static_cast<int>(idx);
                out.bestk = bestk;
                out.mincut = mincut;
                out.df_at_bestk = maxdists[bestk - 1];
                results.push_back(out);
            }

            return results;
        }

        std::vector<KernelBestKResult> bestkMinCutPerStrideKernel(
            const StridedSubgraph<T>& strided,
            int kmin,
            int kmax
        ) {
            std::vector<KernelBestKResult> results;
            results.reserve(static_cast<size_t>(strided.getNumKernels()));

            const int kmin_clamped = std::max(1, kmin);
            const int kmax_clamped = std::max(kmin_clamped, kmax);

            for (const auto& slice : strided.layout().slices()) {
                Subgraph<T> sg_kernel = strided.makeKernelCopy(slice.kernel_id);
                std::vector<float> maxdists = createArcs2(sg_kernel, kmax_clamped);

                int bestk = kmax_clamped;
                float mincut = std::numeric_limits<float>::max();

                for (int k = kmin_clamped; k <= kmax_clamped && mincut != 0.0f; ++k) {
                    sg_kernel.setDf(maxdists[k - 1]);
                    sg_kernel.setBestK(k);

                    pdfToKmax(sg_kernel);
                    clusteringToKmax(sg_kernel);
                    float nc = normalizedCutToKmax(sg_kernel);

                    if (nc < mincut) {
                        mincut = nc;
                        bestk = k;
                    }
                }

                destroyArcs(sg_kernel);
                sg_kernel.setBestK(bestk);
                createArcs(sg_kernel, bestk);
                computePDF(sg_kernel);

                KernelBestKResult out;
                out.kernel_id = slice.kernel_id;
                out.bestk = bestk;
                out.mincut = mincut;
                out.df_at_bestk = maxdists[bestk - 1];
                results.push_back(out);
            }

            return results;
        }

        void updateJointProbabilitiesFromKernels(
            const std::vector<KernelSubGraph<T>>& kernels,
            KernelJointProbabilityAccumulator& accumulator,
            float epsilon = 1e-12f
        ) const {
            for (size_t idx = 0; idx < kernels.size(); ++idx) {
                std::vector<float> probs = kernels[idx].computeLogDensityProbabilities(epsilon);
                accumulator.updateKernelProbabilities(static_cast<int>(idx), probs);
            }
        }

        void applyJointProbabilitiesToSubgraph(
            Subgraph<T>& sg,
            const KernelJointProbabilityAccumulator& accumulator
        ) const {
            const int n = sg.getNumNodes();
            const std::vector<float>& probs = accumulator.getCentralJointProbabilities();
            if (static_cast<int>(probs.size()) != n) {
                throw std::invalid_argument("Accumulator size must match subgraph node count");
            }

            if (n == 0) return;

            float minprob = std::numeric_limits<float>::max();
            float maxprob = std::numeric_limits<float>::lowest();
            for (float p : probs) {
                if (p < minprob) minprob = p;
                if (p > maxprob) maxprob = p;
            }

            sg.setMinDens(minprob);
            sg.setMaxDens(maxprob);

            if (minprob == maxprob) {
                for (int i = 0; i < n; ++i) {
                    Node<T>& node = sg.getNode(i);
                    node.setDens(this->opf_MAXDENS);
                    node.setPathval(this->opf_MAXDENS - 1.0f);
                }
                return;
            }

            for (int i = 0; i < n; ++i) {
                float norm = (probs[i] - minprob) / (maxprob - minprob);
                norm = std::clamp(norm, 0.0f, 1.0f);
                float dens = (this->opf_MAXDENS - 1.0f) * norm + 1.0f;
                Node<T>& node = sg.getNode(i);
                node.setDens(dens);
                node.setPathval(dens - 1.0f);
            }
        }

        void clusterWithJointProbabilities(
            Subgraph<T>& sg,
            const KernelJointProbabilityAccumulator& accumulator
        ) {
            applyJointProbabilitiesToSubgraph(sg, accumulator);
            clustering(sg);
        }
    };

} // namespace opf

#endif // OPF_OPFPP_HPP