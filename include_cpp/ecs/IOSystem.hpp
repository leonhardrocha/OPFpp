#pragma once

#include <fstream>
#include <sstream>
#include <map>
#include "System.h"
#include "Components.hpp"

namespace ecs {

class IOSystem : public ISystem {
public:
    void update(EntityRegistry &registry, double dt) override {
        // This is a passive system; typically called explicitly via saveEntity/loadEntity
    }

    // Save a simple text representation of an entity's components to disk
    bool saveEntity(const EntityRegistry &registry, EntityId id, const std::string &filepath) {
        std::ofstream file(filepath);
        if (!file.is_open()) return false;

        // Write entity ID and type
        file << "entity_id: " << id << "\n";
        
        auto *path = registry.getComponent<CIOPath>(id);
        if (path) {
            file << "iopath: " << path->path << "\n";
        }

        auto *features = registry.getComponent<CFeatures>(id);
        if (features) {
            file << "features: ";
            for (size_t i = 0; i < features->values.size(); ++i) {
                if (i > 0) file << ",";
                file << features->values[i];
            }
            file << "\n";
        }

        auto *label = registry.getComponent<CLabel>(id);
        if (label) {
            file << "label: " << label->label << "\n";
        }

        auto *flags = registry.getComponent<CFlags>(id);
        if (flags) {
            file << "flags: ";
            file << (flags->isTraining ? "T" : "F");
            file << (flags->isEvaluation ? "T" : "F");
            file << (flags->isTesting ? "T" : "F");
            file << "\n";
        }

        auto *samples = registry.getComponent<CSamples>(id);
        if (samples) {
            file << "samples: ";
            for (size_t i = 0; i < samples->indices.size(); ++i) {
                if (i > 0) file << ",";
                file << samples->indices[i];
            }
            file << "\n";
        }

        auto *range = registry.getComponent<CSubgraphRange>(id);
        if (range) {
            file << "range: " << range->start << " " << range->end << "\n";
        }

        auto *split_params = registry.getComponent<CSplitParams>(id);
        if (split_params) {
            file << "split_params: " << split_params->train_percentage << " "
                 << split_params->eval_percentage << " "
                 << split_params->test_percentage << "\n";
        }

        file.close();
        return true;
    }

    // Load entity components from a text file (simple format)
    bool loadEntity(EntityRegistry &registry, EntityId id, const std::string &filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return false;

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            // Parse key: value format
            size_t colon = line.find(':');
            if (colon == std::string::npos) continue;

            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 2);  // skip ': '

            if (key == "iopath") {
                registry.addComponent<CIOPath>(id, CIOPath(value));
            } else if (key == "features") {
                std::vector<float> vals;
                std::stringstream ss(value);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    vals.push_back(std::stof(item));
                }
                registry.addComponent<CFeatures>(id, CFeatures(vals));
            } else if (key == "label") {
                int lbl = std::stoi(value);
                registry.addComponent<CLabel>(id, CLabel(lbl));
            } else if (key == "flags") {
                bool t = (value[0] == 'T');
                bool e = (value[1] == 'T');
                bool te = (value[2] == 'T');
                registry.addComponent<CFlags>(id, CFlags(t, e, te));
            } else if (key == "samples") {
                std::vector<int> indices;
                std::stringstream ss(value);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    indices.push_back(std::stoi(item));
                }
                registry.addComponent<CSamples>(id, CSamples(indices));
            } else if (key == "range") {
                std::stringstream ss(value);
                int start, end;
                ss >> start >> end;
                registry.addComponent<CSubgraphRange>(id, CSubgraphRange(start, end));
            } else if (key == "split_params") {
                std::stringstream ss(value);
                float train, eval, test;
                ss >> train >> eval >> test;
                registry.addComponent<CSplitParams>(id, CSplitParams(train, eval, test));
            }
        }

        file.close();
        return true;
    }

    // Save multiple entities to a single file
    bool saveEntities(const EntityRegistry &registry,
                      const std::vector<EntityId> &entity_ids,
                      const std::string &filepath) {
        std::ofstream file(filepath);
        if (!file.is_open()) return false;

        file << "# ECS Entity Collection\n";
        file << "entity_count: " << entity_ids.size() << "\n\n";

        for (EntityId id : entity_ids) {
            file << "[entity:" << id << "]\n";
            
            auto *path = registry.getComponent<CIOPath>(id);
            if (path) file << "iopath: " << path->path << "\n";

            auto *features = registry.getComponent<CFeatures>(id);
            if (features) {
                file << "features: ";
                for (size_t i = 0; i < features->values.size(); ++i) {
                    if (i > 0) file << ",";
                    file << features->values[i];
                }
                file << "\n";
            }

            auto *label = registry.getComponent<CLabel>(id);
            if (label) file << "label: " << label->label << "\n";

            auto *flags = registry.getComponent<CFlags>(id);
            if (flags) {
                file << "flags: " << (flags->isTraining ? "T" : "F")
                     << (flags->isEvaluation ? "T" : "F")
                     << (flags->isTesting ? "T" : "F") << "\n";
            }

            file << "\n";
        }

        file.close();
        return true;
    }
};

} // namespace ecs
