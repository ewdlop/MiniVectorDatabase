// batch_operations.cpp - Batch insert and update operations example
// Demonstrates efficient bulk operations for large datasets

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <random>
#include <iomanip>

// Include the VectorDatabase implementation
#include "../VectorDatabase.cpp"

// Utility function to generate test data
std::map<std::string, std::vector<float>> generateTestVectors(size_t count, size_t dimension, const std::string& prefix = "test") {
    std::map<std::string, std::vector<float>> vectors;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    
    for (size_t i = 0; i < count; ++i) {
        std::string id = prefix + "_" + std::to_string(i);
        std::vector<float> vector(dimension);
        
        for (size_t j = 0; j < dimension; ++j) {
            vector[j] = dis(gen);
        }
        
        vectors[id] = vector;
    }
    
    return vectors;
}

// Utility function to generate clustered test data
std::map<std::string, std::vector<float>> generateClusteredVectors(size_t clusters, size_t per_cluster, size_t dimension) {
    std::map<std::string, std::vector<float>> vectors;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> center_dis(-2.0f, 2.0f);
    std::normal_distribution<float> cluster_dis(0.0f, 0.2f);
    
    for (size_t c = 0; c < clusters; ++c) {
        // Generate cluster center
        std::vector<float> center(dimension);
        for (size_t d = 0; d < dimension; ++d) {
            center[d] = center_dis(gen);
        }
        
        // Generate vectors around this center
        for (size_t i = 0; i < per_cluster; ++i) {
            std::string id = "cluster" + std::to_string(c) + "_vec" + std::to_string(i);
            std::vector<float> vector(dimension);
            
            for (size_t d = 0; d < dimension; ++d) {
                vector[d] = center[d] + cluster_dis(gen);
            }
            
            vectors[id] = vector;
        }
    }
    
    return vectors;
}

// Function to measure execution time
template<typename Func>
double measureTime(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return static_cast<double>(duration.count()) / 1000.0; // Return milliseconds
}

int main() {
    std::cout << "=== VectorDatabase Batch Operations Example ===" << std::endl;
    
    try {
        const size_t dimension = 128;
        VectorDatabase db(dimension);
        
        // 1. Basic batch insert
        std::cout << "\n1. Basic Batch Insert Operation:" << std::endl;
        std::cout << "   Generating 1000 random " << dimension << "D vectors..." << std::endl;
        
        auto batch1 = generateTestVectors(1000, dimension, "batch1");
        
        double insert_time = measureTime([&]() {
            if (!db.insert_batch(batch1)) {
                throw std::runtime_error("Batch insert failed");
            }
        });
        
        std::cout << "   ✓ Inserted " << batch1.size() << " vectors in " 
                  << std::fixed << std::setprecision(2) << insert_time << " ms" << std::endl;
        std::cout << "   ✓ Insert rate: " << std::fixed << std::setprecision(0) 
                  << (batch1.size() / insert_time * 1000) << " vectors/second" << std::endl;
        
        db.print_stats();
        
        // 2. Performance comparison: individual vs batch insert
        std::cout << "\n2. Performance Comparison (Individual vs Batch):" << std::endl;
        
        VectorDatabase db_individual(dimension);
        VectorDatabase db_batch(dimension);
        
        auto test_vectors = generateTestVectors(500, dimension, "perf_test");
        
        // Individual insertions
        double individual_time = measureTime([&]() {
            for (const auto& pair : test_vectors) {
                if (!db_individual.insert(pair.first, pair.second)) {
                    throw std::runtime_error("Individual insert failed");
                }
            }
        });
        
        // Batch insertion
        double batch_time = measureTime([&]() {
            if (!db_batch.insert_batch(test_vectors)) {
                throw std::runtime_error("Batch insert failed");
            }
        });
        
        std::cout << "   Individual insertions: " << std::fixed << std::setprecision(2) 
                  << individual_time << " ms (" << (test_vectors.size() / individual_time * 1000) << " ops/sec)" << std::endl;
        std::cout << "   Batch insertion:       " << std::fixed << std::setprecision(2) 
                  << batch_time << " ms (" << (test_vectors.size() / batch_time * 1000) << " ops/sec)" << std::endl;
        std::cout << "   Speedup: " << std::fixed << std::setprecision(1) 
                  << (individual_time / batch_time) << "x faster" << std::endl;
        
        // 3. Clustered data batch operations
        std::cout << "\n3. Clustered Data Batch Operations:" << std::endl;
        
        VectorDatabase clustered_db(dimension);
        
        std::cout << "   Generating clustered dataset (5 clusters, 200 vectors each)..." << std::endl;
        auto clustered_data = generateClusteredVectors(5, 200, dimension);
        
        double clustered_insert_time = measureTime([&]() {
            if (!clustered_db.insert_batch(clustered_data)) {
                throw std::runtime_error("Clustered batch insert failed");
            }
        });
        
        std::cout << "   ✓ Inserted " << clustered_data.size() << " clustered vectors in " 
                  << std::fixed << std::setprecision(2) << clustered_insert_time << " ms" << std::endl;
        
        clustered_db.print_stats();
        
        // 4. Large-scale batch operations
        std::cout << "\n4. Large-Scale Batch Operations:" << std::endl;
        
        VectorDatabase large_db(dimension);
        std::vector<size_t> batch_sizes = {1000, 5000, 10000, 25000};
        
        for (size_t batch_size : batch_sizes) {
            std::cout << "   Testing batch size: " << batch_size << " vectors..." << std::endl;
            
            auto large_batch = generateTestVectors(batch_size, dimension, "large_" + std::to_string(batch_size));
            
            double large_insert_time = measureTime([&]() {
                if (!large_db.insert_batch(large_batch)) {
                    throw std::runtime_error("Large batch insert failed");
                }
            });
            
            double rate = batch_size / large_insert_time * 1000;
            std::cout << "     ✓ " << batch_size << " vectors: " << std::fixed << std::setprecision(2) 
                      << large_insert_time << " ms (" << std::fixed << std::setprecision(0) 
                      << rate << " vectors/sec)" << std::endl;
        }
        
        std::cout << "   Final database size: " << large_db.size() << " vectors" << std::endl;
        
        // 5. Batch update simulation (remove and re-insert)
        std::cout << "\n5. Batch Update Simulation:" << std::endl;
        
        // Get some existing vector IDs
        auto all_ids = large_db.get_all_ids();
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(all_ids.begin(), all_ids.end(), gen);
        
        // Select 1000 vectors for "update"
        size_t update_count = std::min(static_cast<size_t>(1000), all_ids.size());
        std::vector<std::string> update_ids(all_ids.begin(), all_ids.begin() + update_count);
        
        std::cout << "   Simulating update of " << update_count << " vectors..." << std::endl;
        
        // Step 1: Remove old vectors
        double remove_time = measureTime([&]() {
            for (const auto& id : update_ids) {
                large_db.remove(id);
            }
        });
        
        // Step 2: Generate new versions
        std::map<std::string, std::vector<float>> updated_vectors;
        for (const auto& id : update_ids) {
            auto new_vector = VectorUtils::generateRandomVector(dimension);
            updated_vectors[id + "_updated"] = new_vector;
        }
        
        // Step 3: Insert updated vectors
        double update_insert_time = measureTime([&]() {
            if (!large_db.insert_batch(updated_vectors)) {
                throw std::runtime_error("Update batch insert failed");
            }
        });
        
        double total_update_time = remove_time + update_insert_time;
        
        std::cout << "     Remove time: " << std::fixed << std::setprecision(2) << remove_time << " ms" << std::endl;
        std::cout << "     Insert time: " << std::fixed << std::setprecision(2) << update_insert_time << " ms" << std::endl;
        std::cout << "     Total time:  " << std::fixed << std::setprecision(2) << total_update_time << " ms" << std::endl;
        std::cout << "     Update rate: " << std::fixed << std::setprecision(0) 
                  << (update_count / total_update_time * 1000) << " updates/sec" << std::endl;
        
        // 6. Memory usage analysis
        std::cout << "\n6. Memory Usage Analysis:" << std::endl;
        
        size_t total_vectors = large_db.size();
        size_t estimated_memory = total_vectors * dimension * sizeof(float);
        
        std::cout << "   Total vectors: " << total_vectors << std::endl;
        std::cout << "   Vector dimension: " << dimension << std::endl;
        std::cout << "   Estimated memory: " << std::fixed << std::setprecision(2) 
                  << (estimated_memory / (1024.0 * 1024.0)) << " MB" << std::endl;
        std::cout << "   Memory per vector: " << (estimated_memory / total_vectors) << " bytes" << std::endl;
        
        // 7. Batch search performance
        std::cout << "\n7. Batch Search Performance:" << std::endl;
        
        // Generate multiple query vectors
        std::vector<std::vector<float>> queries;
        for (int i = 0; i < 100; ++i) {
            queries.push_back(VectorUtils::generateRandomVector(dimension));
        }
        
        std::cout << "   Performing " << queries.size() << " searches on " << total_vectors << " vectors..." << std::endl;
        
        double search_time = measureTime([&]() {
            for (const auto& query : queries) {
                auto results = large_db.search(query, 10);
            }
        });
        
        double avg_search_time = search_time / queries.size();
        double searches_per_second = queries.size() / search_time * 1000;
        
        std::cout << "     Total search time: " << std::fixed << std::setprecision(2) << search_time << " ms" << std::endl;
        std::cout << "     Average per search: " << std::fixed << std::setprecision(3) << avg_search_time << " ms" << std::endl;
        std::cout << "     Searches per second: " << std::fixed << std::setprecision(0) << searches_per_second << std::endl;
        
        // 8. Final statistics
        std::cout << "\n8. Final Database Statistics:" << std::endl;
        large_db.print_stats();
        
        std::cout << "\n=== Batch Operations Example Completed Successfully! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 