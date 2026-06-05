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
                //TODO: Avoid copy by modifying createArcs2 to take a const Subgraph& and return the maxdists, then createArcs3 to take the non-const Subgraph& and build the adjacency based on the precomputed maxdists.
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
            const std::vector<float>& joint_log_probs = accumulator.getCentralJointProbabilities();
            
            if (static_cast<int>(joint_log_probs.size()) != n) {
                throw std::invalid_argument("Accumulator size must match subgraph node count");
            }

            if (n == 0) return;

            // Definição dos limites físicos rígidos e positivos do grafo OPF
            const float L_min = 1.0f;
            const float L_max = static_cast<float>(this->opf_MAXDENS);

            for (int i = 0; i < n; ++i) {
                // 1. joint_log_probs[i] possui a densidade conjunta acumulada em espaço logarítmico (negativo).
                // 2. Transforma para o espaço de probabilidade linear [0.0, 1.0] eliminando o sinal negativo.
                float joint_prob = std::exp(joint_log_probs[i]);
                
                // Proteção numérica contra eventuais overflows/underflows antes do mapeamento
                joint_prob = std::clamp(joint_prob, 0.0f, 1.0f);

                // 3. Mapeamento monotônico direto para o intervalo regulamentar do OPF [1.0, opf_MAXDENS].
                // Esta transformação NÃO muda de formato entre iterações, garantindo a consistência do EM.
                float dens = L_min + (L_max - L_min) * joint_prob;

                Node<T>& node = sg.getNode(i);
                node.setDens(dens);
                
                // O pathval do IFT opera baseado na capacidade física positiva injetada
                node.setPathval(dens - 1.0f);
            }

            // Alinha os metadados do subgrafo com os limites absolutos da escala do espaço
            sg.setMinDens(L_min);
            sg.setMaxDens(L_max);
        }

        /// OPF clustering no subgrafo original, com caminhos restritos pela 
        /// interseção implícita das vizinhanças de múltiplos kernels.
        void jointClusteringToKmax(Subgraph<T>& sg, const std::vector<KernelSubGraph<T>>& kernels) {
            if (kernels.empty()) return;

            const int n    = sg.getNumNodes();
            // Utiliza o kmax do primeiro kernel como limite de busca base
            const int kmax = kernels[0].getBestK();

            std::vector<float> pathval(n);
            using Elem = std::pair<float, int>;
            std::priority_queue<Elem, std::vector<Elem>> Q;  // max-heap

            // Inicialização no subgrafo original
            for (int p = 0; p < n; ++p) {
                pathval[p] = sg.getNode(p).getPathval();
                sg.getNode(p).setPred(NIL);
                sg.getNode(p).setRoot(p);
                Q.push({pathval[p], p});
            }

            int l = 0;
            sg.clearOrderedListOfNodes(); 
            
            while (!Q.empty()) {
                auto [pv, p] = Q.top(); Q.pop();
                sg.addOrderedNode(p);

                // Checa se é uma raiz/máximo local
                if (sg.getNode(p).getPred() == NIL) {
                    pathval[p] = sg.getNode(p).getDens();
                    sg.getNode(p).setLabel(l++);
                }
                sg.getNode(p).setPathval(pathval[p]);

                // 1. Pega a adjacência base a partir do primeiro kernel
                const auto& adjListBase = kernels[0].getKernelAdj(p);
                const int nadj = kmax + sg.getNode(p).getNplatadj();
                int k = 0;
                
                for (int q : adjListBase) {
                    if (k >= nadj) break;
                    if (q < 0 || q >= n) { ++k; continue; }

                    // =================================================================
                    // INTERSEÇÃO IMPLÍCITA ON-THE-FLY
                    // Verifica se 'q' é vizinho de 'p' em TODOS os outros kernels
                    // =================================================================
                    bool is_valid_in_all_kernels = true;
                    for (size_t ki = 1; ki < kernels.size(); ++ki) {
                        const auto& other_adj = kernels[ki].getKernelAdj(p);
                        
                        // Busca linear rápida (ideal para vetores pequenos típicos do kmax)
                        auto it = std::find(other_adj.begin(), other_adj.end(), q);
                        if (it == other_adj.end()) {
                            is_valid_in_all_kernels = false;
                            break; // Aborta cedo: 'q' não está na interseção
                        }
                    }

                    // Se 'q' sobreviveu ao filtro de interseção, avalia o caminho
                    if (is_valid_in_all_kernels) {
                        float tmp = std::min(pathval[p], sg.getNode(q).getDens());
                        if (tmp > pathval[q]) {
                            pathval[q] = tmp;
                            sg.getNode(q).setPred(p);
                            sg.getNode(q).setRoot(sg.getNode(p).getRoot());
                            sg.getNode(q).setLabel(sg.getNode(p).getLabel());
                            Q.push({pathval[q], q});
                        }
                    }
                    // Incrementa k independentemente de q ter passado na interseção ou não,
                    // pois k rastreia o limite de busca (kmax) do kernel base.
                    ++k; 
                }
            }
            sg.setNumLabels(l);
        }

        void clusterWithJointProbabilities(
            Subgraph<T>& sg,
            const KernelJointProbabilityAccumulator& accumulator,
            std::vector<opf::KernelSubGraph<T>>& kernels
        ) {
            applyJointProbabilitiesToSubgraph(sg, accumulator);
            jointClusteringToKmax(sg, kernels);
        }


        void intersectKernelAdjacencies(std::vector<opf::KernelSubGraph<T>>& kernels) {
            if (kernels.size() <= 1) {
                return; // Nenhuma interseção multi-kernel necessária
            }

            // Assume-se que todos os kernels possuem a mesma quantidade de nós (nnodes)
            int num_nodes = kernels[0].getNumNodes();

            // Iteramos por cada ID de nó (índice i de 0 até nnodes-1)
            for (int i = 0; i < num_nodes; ++i) {
                
                // 1. CORREÇÃO ABORDAGEM 3: Lê através do getKernelAdj para capturar possíveis overlays ativos
                // Coleta e ordena a adjacência atual real do nó 'i' no primeiro kernel
                std::vector<int> current_intersection = kernels[0].getKernelAdj(i); 
                std::sort(current_intersection.begin(), current_intersection.end());

                // 2. Intersecciona sucessivamente com a adjacência do nó 'i' dos demais kernels
                for (size_t k = 1; k < kernels.size(); ++k) {
                    // CORREÇÃO ABORDAGEM 3: Sempre ler via getKernelAdj do respectivo Kernel
                    std::vector<int> next_adj = kernels[k].getKernelAdj(i);
                    std::sort(next_adj.begin(), next_adj.end());

                    std::vector<int> temp_result;
                    temp_result.reserve(std::min(current_intersection.size(), next_adj.size()));

                    std::set_intersection(
                        current_intersection.begin(), current_intersection.end(),
                        next_adj.begin(), next_adj.end(),
                        std::back_inserter(temp_result)
                    );

                    current_intersection = std::move(temp_result);
                    
                    if (current_intersection.empty()) {
                        break;
                    }
                }

                // 3. CORREÇÃO ABORDAGEM 3: Alocação única e compartilhamento via ponteiro CoW centralizado.
                // Criamos um único ponteiro compartilhado para o resultado da interseção deste nó.
                auto shared_intersection_result = std::make_shared<const std::vector<int>>(std::move(current_intersection));

                // Injeta o mesmo ponteiro em todos os kernels em tempo O(1), sem cópias de memória!
                for (size_t k = 0; k < kernels.size(); ++k) {
                    kernels[k].setNodeSharedAdj(i, shared_intersection_result);
                }
            }
        }
    };

} // namespace opf

#endif // OPF_OPFPP_HPP