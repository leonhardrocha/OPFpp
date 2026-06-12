#ifndef OPF_OPFPP_HPP
#define OPF_OPFPP_HPP

#include "KernelJointProbability.hpp"
#include "KernelSubGraph.hpp"
#include "OPF.hpp"
#include "StridedSubgraph.hpp"

namespace opf
{

    struct KernelBestKResult
    {
        int kernel_id = -1;
        int bestk = 0;
        float mincut = std::numeric_limits<float>::max();
        float df_at_bestk = 0.0f;
    };

    template <typename T>
    class OPFpp : public OPF<T>
    {
    public:
        using OPF<T>::clustering;
        using OPF<T>::computePDF;
        using OPF<T>::createArcs;
        using OPF<T>::createArcs2;
        using OPF<T>::destroyArcs;
        using OPF<T>::normalizedCutToKmax;
        using OPF<T>::pdfToKmax;
        using OPF<T>::clusteringToKmax;

        void clustering(KernelSubGraph<T> &ksg)
        {
            std::vector<float> pathval(ksg.getNumNodes());
            using Elem = std::pair<float, int>;
            std::priority_queue<Elem, std::vector<Elem>> Q;

            for (int p = 0; p < ksg.getNumNodes(); ++p)
            {
                pathval[p] = ksg.getNode(p).getPathval();
                ksg.getNode(p).setPred(-1); // NIL
                ksg.getNode(p).setRoot(p);
                Q.push({pathval[p], p});
            }

            int l = 0;
            ksg.clearOrderedListOfNodes();
            while (!Q.empty())
            {
                int p = Q.top().second;
                Q.pop();
                ksg.addOrderedNode(p);

                if (ksg.getNode(p).getPred() == -1)
                {
                    pathval[p] = ksg.getNode(p).getDens();
                    ksg.getNode(p).setLabel(l++);
                }

                ksg.getNode(p).setPathval(pathval[p]);

                for (int q : ksg.getKernelAdj(p))
                {
                    if (q < 0 || q >= ksg.getNumNodes())
                        continue; // bounds check
                    float tmp = std::min(pathval[p], ksg.getNode(q).getDens());
                    if (tmp > pathval[q])
                    {
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
        void clusteringToKmax(KernelSubGraph<T> &ksg)
        {
            const int n = ksg.getNumNodes();
            const int kmax = ksg.getBestK();

            std::vector<float> pathval(n);
            using Elem = std::pair<float, int>;
            std::priority_queue<Elem, std::vector<Elem>> Q; // max-heap

            for (int p = 0; p < n; ++p)
            {
                pathval[p] = ksg.getNode(p).getPathval();
                ksg.getNode(p).setPred(NIL);
                ksg.getNode(p).setRoot(p);
                Q.push({pathval[p], p});
            }

            int l = 0;
            ksg.clearOrderedListOfNodes();
            while (!Q.empty())
            {
                auto [pv, p] = Q.top();
                Q.pop();
                ksg.addOrderedNode(p);

                if (ksg.getNode(p).getPred() == NIL)
                {
                    pathval[p] = ksg.getNode(p).getDens();
                    ksg.getNode(p).setLabel(l++);
                }
                ksg.getNode(p).setPathval(pathval[p]);

                const auto &adjList = ksg.getKernelAdj(p);
                const int nadj = kmax + ksg.getNode(p).getNplatadj();
                int k = 0;
                for (int q : adjList)
                {
                    if (k >= nadj)
                        break;
                    if (q < 0 || q >= n)
                    {
                        ++k;
                        continue;
                    }
                    float tmp = std::min(pathval[p], ksg.getNode(q).getDens());
                    if (tmp > pathval[q])
                    {
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
        float normalizedCut(KernelSubGraph<T> &ksg)
        {
            const int n = ksg.getNumNodes();
            const int nlabels = ksg.getNumLabels();
            std::vector<float> acumIC(nlabels, 0.0f);
            std::vector<float> acumEC(nlabels, 0.0f);

            for (int p = 0; p < n; ++p)
            {
                for (int q : ksg.getKernelAdj(p))
                {
                    float dist = distance::euclDist<T>(*ksg.getNode(p).getFeat(), *ksg.getNode(q).getFeat());
                    if (dist > 0.0f)
                    {
                        int lp = ksg.getNode(p).getLabel();
                        int lq = ksg.getNode(q).getLabel();
                        if (lp == lq)
                            acumIC[lp] += 1.0f / dist;
                        else
                        {
                            acumEC[lp] += 1.0f / dist;
                            acumEC[lq] += 1.0f / dist;
                        }
                    }
                }
            }

            float ncut = 0.0f;
            for (int l = 0; l < nlabels; ++l)
            {
                float denom = acumIC[l] + acumEC[l];
                if (denom > 0.0f)
                    ncut += acumEC[l] / denom;
            }
            return ncut;
        }

        std::vector<float> computeKernelDists(KernelSubGraph<T> &ksg, int kmax)
        {
            const int n = ksg.getNumNodes();
            std::vector<float> maxdists(kmax, 0.0f); // maxdists[k-1] = max df at k

            for (int i = 0; i < n; ++i)
            {
                // ksg.getNode(i).clearAdj();
                //ksg.getNode(i).setNplatadj(0);

                std::vector<float> d(kmax + 1, std::numeric_limits<float>::max());
                std::vector<int> nn(kmax + 1, -1);

                for (int j = 0; j < n; ++j)
                {
                    if (j == i)
                        continue;
                    d[kmax] = distance::euclDist<T>(*ksg.getNode(i).getFeat(), *ksg.getNode(j).getFeat());
                    nn[kmax] = j;
                    int pos = kmax;
                    while (pos > 0 && d[pos] < d[pos - 1])
                    {
                        std::swap(d[pos], d[pos - 1]);
                        std::swap(nn[pos], nn[pos - 1]);
                        --pos;
                    }
                }

                for (int l = 0; l < kmax; ++l)
                {
                    if (d[l] < std::numeric_limits<float>::max())
                    {
                        if (d[l] > maxdists[l])
                            maxdists[l] = d[l];
                            
                        //ksg.setNodeSharedAdj(i, nn[l]);
                        // ksg.getNode(i).addToAdj(nn[l]);
                    }
                }
            }

            // maxdists[k-1] should be the max over all 1..k neighbors, not just k-th
            for (int k = 1; k < kmax; ++k)
                if (maxdists[k] < maxdists[k - 1])
                    maxdists[k] = maxdists[k - 1];

            return maxdists;
        }

        /// Normalised cut limited to the first bestk neighbors.
        /// Mirrors opf_NormalizedCutToKmax.
        float normalizedCutToKmax(KernelSubGraph<T> &ksg)
        {
            const int n = ksg.getNumNodes();
            const int nlabels = ksg.getNumLabels();
            const int kmax = ksg.getBestK();
            std::vector<float> acumIC(nlabels, 0.0f);
            std::vector<float> acumEC(nlabels, 0.0f);

            for (int p = 0; p < n; ++p)
            {
                const auto &adjList = ksg.getNode(p).getAdj();
                const int nadj = kmax + ksg.getNode(p).getNplatadj();
                int k = 0;
                for (int q : adjList)
                {
                    if (k >= nadj)
                        break;
                    float dist = distance::euclDist<T>(*ksg.getNode(p).getFeat(), *ksg.getNode(q).getFeat());
                    if (dist > 0.0f)
                    {
                        int lp = ksg.getNode(p).getLabel();
                        int lq = ksg.getNode(q).getLabel();
                        if (lp == lq)
                            acumIC[lp] += 1.0f / dist;
                        else
                        {
                            acumEC[lp] += 1.0f / dist;
                            acumEC[lq] += 1.0f / dist;
                        }
                    }
                    ++k;
                }
            }

            float ncut = 0.0f;
            for (int l = 0; l < nlabels; ++l)
            {
                float denom = acumIC[l] + acumEC[l];
                if (denom > 0.0f)
                    ncut += acumEC[l] / denom;
            }
            return ncut;
        }
    
        void computeDensityFromAdjacency(KernelSubGraph<T> &ksg, int adjacency_limit)
        {
            const int n = ksg.getNumNodes();
            const float K = 2.0f * ksg.getDf() / 9.0f;
            ksg.setK(K);

            std::vector<float> value(n);
            float mindens = std::numeric_limits<float>::max();
            float maxdens = std::numeric_limits<float>::lowest();

            for (int i = 0; i < n; ++i)
            {
                float sum = 0.0f;
                int nelems = 1;
                const auto &adjList = ksg.getNode(i).getAdj();
                int k = 0;
                for (int q : adjList)
                {
                    if (adjacency_limit > 0 && k >= adjacency_limit)
                        break;
                    float dist = distance::euclDist<T>(*ksg.getNode(i).getFeat(), *ksg.getNode(q).getFeat());
                    sum += std::exp(-dist / K);
                    ++nelems;
                    ++k;
                }
                value[i] = sum / static_cast<float>(nelems);
                if (value[i] < mindens)
                    mindens = value[i];
                if (value[i] > maxdens)
                    maxdens = value[i];
            }

            ksg.setMinDens(mindens);
            ksg.setMaxDens(maxdens);

            if (mindens == maxdens)
            {
                for (int i = 0; i < n; ++i)
                {
                    ksg.getNode(i).setDens(OPF<T>::opf_MAXDENS);
                    ksg.getNode(i).setPathval(OPF<T>::opf_MAXDENS - 1.0f);
                }
                return;
            }

            for (int i = 0; i < n; ++i)
            {
                float norm = (value[i] - mindens) / (maxdens - mindens);
                norm = std::clamp(norm, 0.0f, 1.0f);

                float dens = (OPF<T>::opf_MAXDENS - 1.0f) * norm + 1.0f;
                ksg.getNode(i).setDens(dens);
                ksg.getNode(i).setPathval(dens - 1.0f);
            }
        }

         /// Clear all adjacency lists (mirrors opf_DestroyArcs).
        void destroyKernelArcs(KernelSubGraph<T>& sg) {
            for (int i = 0; i < sg.getNumNodes(); ++i) {
                sg.getNode(i).clearAdj();
                sg.getNode(i).setNplatadj(0);
            }
        }

        std::vector<KernelBestKResult> bestkMinCutPerKernel(
            const std::vector<KernelSubGraph<T>> &kernels,
            int kmin,
            int kmax)
        {
            std::vector<KernelBestKResult> results;
            results.reserve(kernels.size());

            const int kmin_clamped = std::max(1, kmin);
            const int kmax_clamped = std::max(kmin_clamped, kmax);

            for (size_t idx = 0; idx < kernels.size(); ++idx)
            {
                // TODO: Avoid copy by modifying createArcs3 to take a const Subgraph& and return the maxdists, then createArcs3 to take the non-const Subgraph& and build the adjacency based on the precomputed maxdists.
                auto sg_kernel = kernels[idx]; //.toSubgraph();
                std::vector<float> maxdists = computeKernelDists(sg_kernel, kmax_clamped);

                int bestk = kmax_clamped;
                float mincut = std::numeric_limits<float>::max();

                for (int k = kmin_clamped; k <= kmax_clamped && mincut != 0.0f; ++k)
                {
                    sg_kernel.setDf(maxdists[k - 1]);
                    sg_kernel.setBestK(k);

                    computeDensityFromAdjacency(sg_kernel, kmax_clamped);
                    clusteringToKmax(sg_kernel);
                    float nc = normalizedCutToKmax(sg_kernel);

                    if (nc < mincut)
                    {
                        mincut = nc;
                        bestk = k;
                    }
                }

                destroyKernelArcs(sg_kernel);
                sg_kernel.setBestK(bestk);
                sg_kernel.setDf(maxdists[bestk - 1]);
                computeDensityFromAdjacency(sg_kernel, bestk);

                KernelBestKResult out;
                out.kernel_id = static_cast<int>(idx);
                out.bestk = bestk;
                out.mincut = mincut;
                out.df_at_bestk = maxdists[bestk - 1];
                results.push_back(out);
            }

            return results;
        }

        void clusteringWithRandomLabels(Subgraph<T> &sg, int num_samples, int num_labels)
        {
            if (num_labels <= 0 || num_samples <= 0)
            {
                throw std::invalid_argument("O numero de amostras e labels deve ser maior que 0.");
            }
            if (num_labels > num_samples)
            {
                throw std::invalid_argument("O numero de labels nao pode ser maior que o numero de amostras.");
            }
            if (num_samples > sg.getNumNodes())
            {
                throw std::invalid_argument("O numero de amostras nao pode exceder o total de nos do grafo.");
            }

            const int total_nodes = sg.getNumNodes();

            // 1. Inicializa as estruturas de controle com valores vazios/padrão
            // Usamos -1.0f para indicar que o nó ainda não foi conquistado por nenhuma árvore
            std::vector<float> pathval(total_nodes, -1.0f);
            using Elem = std::pair<float, int>;
            std::priority_queue<Elem, std::vector<Elem>> Q;

            for (int p = 0; p < total_nodes; ++p)
            {
                sg.getNode(p).setPred(-1); // NIL
                sg.getNode(p).setRoot(-1);
                sg.getNode(p).setLabel(-1);
            }

            // 2. Sorteia uniformemente 'num_samples' nós únicos do grafo
            std::vector<int> all_indices(total_nodes);
            std::iota(all_indices.begin(), all_indices.end(), 0); // Preenche de 0 a total_nodes-1

            std::random_device rd;
            std::mt19937 gen(rd());
            std::shuffle(all_indices.begin(), all_indices.end(), gen);

            // 3. Geração e distribuição dos labels fixos mapeados para as amostras
            std::vector<int> sample_labels(num_samples);

            // Garante que TODOS os 'num_labels' sejam usados pelo menos uma vez (evita labels órfãos)
            for (int i = 0; i < num_labels; ++i)
            {
                sample_labels[i] = i;
            }
            // Preenche o restante das vagas das amostras com escolhas puramente aleatórias
            std::uniform_int_distribution<int> dis(0, num_labels - 1);
            for (int i = num_labels; i < num_samples; ++i)
            {
                sample_labels[i] = dis(gen);
            }
            // Embaralha o vetor de labels para que a garantia inicial se misture nas amostras sorteadas
            std::shuffle(sample_labels.begin(), sample_labels.end(), gen);

            for (int i = 0; i < num_labels; ++i)
            {
                int p = all_indices[i];     // Índice do nó sorteado do grafo
                int lbl = sample_labels[i]; // Label sorteado atribuído a ele

                pathval[p] = sg.getNode(p).getPathval();
                sg.getNode(p).setPred(-1); // NIL
                sg.getNode(p).setRoot(p);
                Q.push({pathval[p], p});
            }

            int l = 0;
            sg.clearOrderedListOfNodes();
            while (!Q.empty())
            {
                int p = Q.top().second;
                Q.pop();
                sg.addOrderedNode(p);

                if (sg.getNode(p).getPred() == -1)
                {
                    pathval[p] = sg.getNode(p).getDens();
                    sg.getNode(p).setLabel(l++);
                }

                sg.getNode(p).setPathval(pathval[p]);

                for (int q : sg.getNode(p).getAdj())
                {
                    if (q < 0 || q >= sg.getNumNodes())
                        continue; // bounds check
                    float tmp = std::min(pathval[p], sg.getNode(q).getDens());
                    if (tmp > pathval[q])
                    {
                        pathval[q] = tmp;
                        sg.getNode(q).setPred(p);
                        sg.getNode(q).setRoot(sg.getNode(p).getRoot());
                        sg.getNode(q).setLabel(sg.getNode(p).getLabel());
                        Q.push({pathval[q], q});
                    }
                }
            }
            sg.setNumLabels(l);
        }

        std::vector<KernelBestKResult> bestkMinCutPerStrideKernel(
            const StridedSubgraph<T> &strided,
            int kmin,
            int kmax)
        {
            std::vector<KernelBestKResult> results;
            results.reserve(static_cast<size_t>(strided.getNumKernels()));

            const int kmin_clamped = std::max(1, kmin);
            const int kmax_clamped = std::max(kmin_clamped, kmax);

            for (const auto &slice : strided.layout().slices())
            {
                Subgraph<T> sg_kernel = strided.makeKernelCopy(slice.kernel_id);
                std::vector<float> maxdists = createArcs2(sg_kernel, kmax_clamped);

                int bestk = kmax_clamped;
                float mincut = std::numeric_limits<float>::max();

                for (int k = kmin_clamped; k <= kmax_clamped && mincut != 0.0f; ++k)
                {
                    sg_kernel.setDf(maxdists[k - 1]);
                    sg_kernel.setBestK(k);

                    pdfToKmax(sg_kernel);
                    clusteringToKmax(sg_kernel);
                    float nc = normalizedCutToKmax(sg_kernel);

                    if (nc < mincut)
                    {
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
            const std::vector<KernelSubGraph<T>> &kernels,
            KernelJointProbabilityAccumulator &accumulator,
            float epsilon = 1e-12f) const
        {
            for (size_t idx = 0; idx < kernels.size(); ++idx)
            {
                std::vector<float> probs = kernels[idx].computeLogDensityProbabilities(epsilon);
                accumulator.updateKernelProbabilities(static_cast<int>(idx), probs);
            }
        }

        void applyJointProbabilitiesToSubgraph(
            Subgraph<T> &sg,
            const KernelJointProbabilityAccumulator &accumulator) const
        {
            const int n = sg.getNumNodes();
            const std::vector<float> &joint_log_probs = accumulator.getCentralJointProbabilities();

            if (static_cast<int>(joint_log_probs.size()) != n)
            {
                throw std::invalid_argument("Accumulator size must match subgraph node count");
            }

            if (n == 0)
                return;

            // Definição dos limites físicos rígidos e positivos do grafo OPF
            const float L_min = 1.0f;    // sg.getMinDens(); // Pode ser 0.0f ou um valor mínimo positivo específico do domínio
            const float L_max = 1000.0f; // sg.getMaxDens(); // Deve ser um valor positivo finito que representa a densidade máxima esperada no grafo

            for (int i = 0; i < n; ++i)
            {
                // 1. joint_log_probs[i] possui a densidade conjunta acumulada em espaço logarítmico (negativo).
                // 2. Transforma para o espaço de probabilidade linear [0.0, 1.0] eliminando o sinal negativo.
                float joint_prob = std::exp(joint_log_probs[i]);

                // Proteção numérica contra eventuais overflows/underflows antes do mapeamento
                joint_prob = std::clamp(joint_prob, 0.0f, 1.0f);

                // 3. Mapeamento monotônico direto para o intervalo regulamentar do OPF [1.0, opf_MAXDENS].
                // Esta transformação NÃO muda de formato entre iterações, garantindo a consistência do EM.
                float dens = L_min + (L_max - L_min) * joint_prob;

                Node<T> &node = sg.getNode(i);
                node.setDens(dens);
            }

            // Alinha os metadados do subgrafo com os limites absolutos da escala do espaço
            sg.setMinDens(L_min);
            sg.setMaxDens(L_max);
        }

        /// OPF clustering no subgrafo original, com caminhos restritos pela
        /// interseção implícita das vizinhanças de múltiplos kernels.
        void jointKernelsClustering(Subgraph<T> &sg, const std::vector<KernelSubGraph<T>> &kernels, const float cost_offset = 0.0f)
        {
            if (kernels.empty())
                return;

            const int n = sg.getNumNodes();

            std::vector<float> pathval(n);
            using Elem = std::pair<float, int>;
            std::priority_queue<Elem, std::vector<Elem>> Q; // max-heap

            // Initially, all nodes are unconquered (pathval = -inf) and will only be conquered if at least one kernel allows the connection via its adjacency.
            // The initial seeds for the clustering will be determined by the original densities of the subgraph, while the path constraints will be governed by the intersection of the kernels' adjacencies.
            for (int p = 0; p < n; ++p)
            {
                if (sg.getNode(p).getPred() == NIL)
                {
                    pathval[p] = sg.getNode(p).getDens();
                }
                else
                {
                    pathval[p] = sg.getNode(p).getDens() - cost_offset;
                }
                Q.push({pathval[p], p});
            }
            int l = 0;
            sg.clearOrderedListOfNodes();

            while (!Q.empty())
            {
                auto [pv, p] = Q.top();
                Q.pop();
                sg.addOrderedNode(p);

                // Verify if 'p' is a new seed (not yet conquered by any tree).
                // If so, assign a new label and update its pathval to its own density.
                // This ensures that the initial seeds for the clustering are determined by the original densities of the subgraph,
                // while still allowing the path constraints to be governed by the intersection of the kernels' adjacencies.
                if (sg.getNode(p).getPred() == NIL)
                {
                    pathval[p] = sg.getNode(p).getDens();
                    sg.getNode(p).setLabel(l++);
                    sg.getNode(p).setTruelabel(sg.getNode(p).getLabel()); // keep the original label as truelabel for potential later use
                }
                sg.getNode(p).setPathval(pathval[p]);

                // get the base adjacency list from the original subgraph, which represents the most permissive connectivity constraints before applying the kernel intersections
                const auto &adjListBase = sg.getNode(p).getAdj();
                for (int q : adjListBase)
                {
                    if (q < 0 || q >= n)
                    {
                        continue;
                    }
                    // extract the maximum density value for node q across all kernels, which represents the most permissive path constraint from any kernel's perspective
                    float dens_q = kernels[0].getNode(q).getDens();
                    for (size_t ki = 1; ki < kernels.size(); ++ki)
                    {
                        float tmp = kernels[ki].getNode(q).getDens();
                        dens_q = std::max(dens_q, tmp);
                    }
                    float tmp = std::min(pathval[p], dens_q);
                    if (tmp > pathval[q])
                    {
                        pathval[q] = tmp;
                        sg.getNode(q).setPred(p);
                        sg.getNode(q).setRoot(sg.getNode(p).getRoot());
                        sg.getNode(q).setLabel(sg.getNode(p).getLabel());
                        Q.push({pathval[q], q});
                    }
                }
            }
            sg.setNumLabels(l);
        }

        void intersectKernelAdjacencies(std::vector<opf::KernelSubGraph<T>> &kernels)
        {
            if (kernels.size() <= 1)
            {
                return; // Nenhuma interseção multi-kernel necessária
            }

            // Assume-se que todos os kernels possuem a mesma quantidade de nós (nnodes)
            int num_nodes = kernels[0].getNumNodes();

            // Iteramos por cada ID de nó (índice i de 0 até nnodes-1)
            for (int i = 0; i < num_nodes; ++i)
            {

                // 1. CORREÇÃO ABORDAGEM 3: Lê através do getKernelAdj para capturar possíveis overlays ativos
                // Coleta e ordena a adjacência atual real do nó 'i' no primeiro kernel
                std::vector<int> current_intersection = kernels[0].getKernelAdj(i);
                std::sort(current_intersection.begin(), current_intersection.end());

                // 2. Intersecciona sucessivamente com a adjacência do nó 'i' dos demais kernels
                for (size_t k = 1; k < kernels.size(); ++k)
                {
                    // CORREÇÃO ABORDAGEM 3: Sempre ler via getKernelAdj do respectivo Kernel
                    std::vector<int> next_adj = kernels[k].getKernelAdj(i);
                    std::sort(next_adj.begin(), next_adj.end());

                    std::vector<int> temp_result;
                    temp_result.reserve(std::min(current_intersection.size(), next_adj.size()));

                    std::set_intersection(
                        current_intersection.begin(), current_intersection.end(),
                        next_adj.begin(), next_adj.end(),
                        std::back_inserter(temp_result));

                    current_intersection = std::move(temp_result);

                    if (current_intersection.empty())
                    {
                        break;
                    }
                }

                // 3. CORREÇÃO ABORDAGEM 3: Alocação única e compartilhamento via ponteiro CoW centralizado.
                // Criamos um único ponteiro compartilhado para o resultado da interseção deste nó.
                auto shared_intersection_result = std::make_shared<const std::vector<int>>(std::move(current_intersection));

                // Injeta o mesmo ponteiro em todos os kernels em tempo O(1), sem cópias de memória!
                for (size_t k = 0; k < kernels.size(); ++k)
                {
                    kernels[k].setNodeSharedAdj(i, shared_intersection_result);
                }
            }
        }
    };

} // namespace opf

#endif // OPF_OPFPP_HPP