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