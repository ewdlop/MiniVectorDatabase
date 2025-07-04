// VectorDatabase.cpp : High-performance vector database implementation in C++
//

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <thread>
#include <mutex>
#include <fstream>
#include <memory>
#include <queue>
#include <functional>
#include <random>
#include <chrono>
#include <iomanip>

// Forward declarations
enum class DistanceMetric {
    EUCLIDEAN,
    COSINE,
    MANHATTAN,
    DOT_PRODUCT
};

enum class IndexType {
    LINEAR,
    KD_TREE,
    HASH_TABLE
};

struct VectorDatabaseConfig {
    DistanceMetric distance_metric = DistanceMetric::EUCLIDEAN;
    IndexType index_type = IndexType::LINEAR;
    size_t max_vectors = 100000;
    size_t thread_count = std::thread::hardware_concurrency();
    
    VectorDatabaseConfig() = default;
};

struct SearchResult {
    std::string id;
    float distance;
    std::vector<float> vector;
    
    SearchResult(const std::string& _id, float _distance, const std::vector<float>& _vector)
        : id(_id), distance(_distance), vector(_vector) {}
        
    // Comparator for priority queue (min-heap for distances)
    bool operator>(const SearchResult& other) const {
        return distance > other.distance;
    }
};

// Utility class for high-dimensional vector operations
class VectorUtils {
public:
    // Generate random vector with specified dimension
    static std::vector<float> generateRandomVector(size_t dimension, float min_val = -1.0f, float max_val = 1.0f) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(min_val, max_val);
        
        std::vector<float> vector(dimension);
        for (size_t i = 0; i < dimension; ++i) {
            vector[i] = dis(gen);
        }
        return vector;
    }
    
    // Generate random normalized vector (unit vector)
    static std::vector<float> generateRandomUnitVector(size_t dimension) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::normal_distribution<float> dis(0.0f, 1.0f);
        
        std::vector<float> vector(dimension);
        float norm = 0.0f;
        
        // Generate random vector from normal distribution
        for (size_t i = 0; i < dimension; ++i) {
            vector[i] = dis(gen);
            norm += vector[i] * vector[i];
        }
        
        // Normalize to unit length
        norm = std::sqrt(norm);
        if (norm > 0.0f) {
            for (size_t i = 0; i < dimension; ++i) {
                vector[i] /= norm;
            }
        }
        
        return vector;
    }
    
    // Generate vectors based on Gaussian distribution around a center
    static std::vector<float> generateGaussianVector(const std::vector<float>& center, float std_dev = 0.1f) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::normal_distribution<float> dis(0.0f, std_dev);
        
        std::vector<float> vector(center.size());
        for (size_t i = 0; i < center.size(); ++i) {
            vector[i] = center[i] + dis(gen);
        }
        
        return vector;
    }
    
    // Calculate vector magnitude/norm
    static float calculateMagnitude(const std::vector<float>& vector) {
        float sum = 0.0f;
        for (float val : vector) {
            sum += val * val;
        }
        return std::sqrt(sum);
    }
    
    // Normalize vector to unit length
    static std::vector<float> normalize(const std::vector<float>& vector) {
        float magnitude = calculateMagnitude(vector);
        if (magnitude == 0.0f) return vector;
        
        std::vector<float> normalized(vector.size());
        for (size_t i = 0; i < vector.size(); ++i) {
            normalized[i] = vector[i] / magnitude;
        }
        return normalized;
    }
    
    // Add two vectors
    static std::vector<float> add(const std::vector<float>& a, const std::vector<float>& b) {
        if (a.size() != b.size()) {
            throw std::invalid_argument("Vector dimensions must match for addition");
        }
        
        std::vector<float> result(a.size());
        for (size_t i = 0; i < a.size(); ++i) {
            result[i] = a[i] + b[i];
        }
        return result;
    }
    
    // Scalar multiplication
    static std::vector<float> multiply(const std::vector<float>& vector, float scalar) {
        std::vector<float> result(vector.size());
        for (size_t i = 0; i < vector.size(); ++i) {
            result[i] = vector[i] * scalar;
        }
        return result;
    }
    
    // Print vector in a compact format (useful for high-dimensional vectors)
    static void printVector(const std::vector<float>& vector, size_t max_elements = 5) {
        std::cout << "[";
        if (vector.size() <= max_elements * 2) {
            // Print all elements if vector is small
            for (size_t i = 0; i < vector.size(); ++i) {
                std::cout << std::fixed << std::setprecision(3) << vector[i];
                if (i < vector.size() - 1) std::cout << ", ";
            }
        } else {
            // Print first few and last few elements for large vectors
            for (size_t i = 0; i < max_elements; ++i) {
                std::cout << std::fixed << std::setprecision(3) << vector[i] << ", ";
            }
            std::cout << "..., ";
            for (size_t i = vector.size() - max_elements; i < vector.size(); ++i) {
                std::cout << std::fixed << std::setprecision(3) << vector[i];
                if (i < vector.size() - 1) std::cout << ", ";
            }
        }
        std::cout << "] (dim=" << vector.size() << ")";
    }
};

class VectorDatabase {
private:
    size_t dimension_;
    VectorDatabaseConfig config_;
    std::unordered_map<std::string, std::vector<float>> vectors_;
    mutable std::mutex database_mutex_;
    
    // Distance calculation functions
    float calculateEuclideanDistance(const std::vector<float>& a, const std::vector<float>& b) const {
        if (a.size() != b.size()) return std::numeric_limits<float>::max();
        
        float sum = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            float diff = a[i] - b[i];
            sum += diff * diff;
        }
        return std::sqrt(sum);
    }
    
    float calculateCosineDistance(const std::vector<float>& a, const std::vector<float>& b) const {
        if (a.size() != b.size()) return std::numeric_limits<float>::max();
        
        float dot_product = 0.0f;
        float norm_a = 0.0f;
        float norm_b = 0.0f;
        
        for (size_t i = 0; i < a.size(); ++i) {
            dot_product += a[i] * b[i];
            norm_a += a[i] * a[i];
            norm_b += b[i] * b[i];
        }
        
        if (norm_a == 0.0f || norm_b == 0.0f) return 1.0f;
        
        float cosine_similarity = dot_product / (std::sqrt(norm_a) * std::sqrt(norm_b));
        return 1.0f - cosine_similarity;  // Convert similarity to distance
    }
    
    float calculateManhattanDistance(const std::vector<float>& a, const std::vector<float>& b) const {
        if (a.size() != b.size()) return std::numeric_limits<float>::max();
        
        float sum = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            sum += std::abs(a[i] - b[i]);
        }
        return sum;
    }
    
    float calculateDotProductDistance(const std::vector<float>& a, const std::vector<float>& b) const {
        if (a.size() != b.size()) return std::numeric_limits<float>::max();
        
        float dot_product = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            dot_product += a[i] * b[i];
        }
        return -dot_product;  // Negative because we want higher dot products to be "closer"
    }
    
    float calculateDistance(const std::vector<float>& a, const std::vector<float>& b) const {
        switch (config_.distance_metric) {
            case DistanceMetric::EUCLIDEAN:
                return calculateEuclideanDistance(a, b);
            case DistanceMetric::COSINE:
                return calculateCosineDistance(a, b);
            case DistanceMetric::MANHATTAN:
                return calculateManhattanDistance(a, b);
            case DistanceMetric::DOT_PRODUCT:
                return calculateDotProductDistance(a, b);
            default:
                return calculateEuclideanDistance(a, b);
        }
    }
    
    bool validateVector(const std::vector<float>& vector) const {
        return vector.size() == dimension_;
    }

public:
    // Constructors
    explicit VectorDatabase(size_t dimension) 
        : dimension_(dimension), config_() {
        if (dimension == 0) {
            throw std::invalid_argument("Vector dimension must be greater than 0");
        }
        std::cout << "Created VectorDatabase for " << dimension << "-dimensional vectors" << std::endl;
    }
    
    VectorDatabase(size_t dimension, const VectorDatabaseConfig& config)
        : dimension_(dimension), config_(config) {
        if (dimension == 0) {
            throw std::invalid_argument("Vector dimension must be greater than 0");
        }
        std::cout << "Created VectorDatabase for " << dimension << "-dimensional vectors with custom config" << std::endl;
    }
    
    // Utility functions for high-dimensional vectors
    std::vector<float> generateRandomVector(float min_val = -1.0f, float max_val = 1.0f) const {
        return VectorUtils::generateRandomVector(dimension_, min_val, max_val);
    }
    
    std::vector<float> generateRandomUnitVector() const {
        return VectorUtils::generateRandomUnitVector(dimension_);
    }
    
    std::vector<float> generateGaussianVector(const std::vector<float>& center, float std_dev = 0.1f) const {
        if (center.size() != dimension_) {
            throw std::invalid_argument("Center vector dimension mismatch");
        }
        return VectorUtils::generateGaussianVector(center, std_dev);
    }
    
    // Generate and insert random vectors for testing
    bool insertRandomVectors(size_t count, const std::string& prefix = "rand") {
        std::map<std::string, std::vector<float>> batch;
        
        for (size_t i = 0; i < count; ++i) {
            std::string id = prefix + "_" + std::to_string(i);
            batch[id] = generateRandomVector();
        }
        
        return insert_batch(batch);
    }
    
    // Generate and insert clustered vectors around centers
    bool insertClusteredVectors(const std::vector<std::vector<float>>& centers, 
                               size_t vectors_per_cluster, 
                               float std_dev = 0.1f,
                               const std::string& prefix = "cluster") {
        std::map<std::string, std::vector<float>> batch;
        
        for (size_t c = 0; c < centers.size(); ++c) {
            if (centers[c].size() != dimension_) {
                std::cerr << "Error: Center " << c << " dimension mismatch" << std::endl;
                return false;
            }
            
            for (size_t i = 0; i < vectors_per_cluster; ++i) {
                std::string id = prefix + "_" + std::to_string(c) + "_" + std::to_string(i);
                batch[id] = generateGaussianVector(centers[c], std_dev);
            }
        }
        
        return insert_batch(batch);
    }
    
    // Insert operations
    bool insert(const std::string& id, const std::vector<float>& vector) {
        if (!validateVector(vector)) {
            std::cerr << "Error: Vector dimension mismatch. Expected " << dimension_ 
                      << ", got " << vector.size() << std::endl;
            return false;
        }
        
        if (id.empty()) {
            std::cerr << "Error: Vector ID cannot be empty" << std::endl;
            return false;
        }
        
        std::lock_guard<std::mutex> lock(database_mutex_);
        
        if (vectors_.size() >= config_.max_vectors) {
            std::cerr << "Error: Maximum vector capacity reached (" << config_.max_vectors << ")" << std::endl;
            return false;
        }
        
        vectors_[id] = vector;
        return true;
    }
    
    bool insert_batch(const std::map<std::string, std::vector<float>>& vectors) {
        std::lock_guard<std::mutex> lock(database_mutex_);
        
        // Validate all vectors first
        for (const auto& pair : vectors) {
            if (!validateVector(pair.second) || pair.first.empty()) {
                std::cerr << "Error: Invalid vector in batch for ID: " << pair.first << std::endl;
                return false;
            }
        }
        
        if (vectors_.size() + vectors.size() > config_.max_vectors) {
            std::cerr << "Error: Batch insert would exceed maximum capacity" << std::endl;
            return false;
        }
        
        // Insert all vectors
        for (const auto& pair : vectors) {
            vectors_[pair.first] = pair.second;
        }
        
        return true;
    }
    
    // Search operations
    std::vector<SearchResult> search(const std::vector<float>& query, size_t k) const {
        if (!validateVector(query)) {
            std::cerr << "Error: Query vector dimension mismatch" << std::endl;
            return {};
        }
        
        std::lock_guard<std::mutex> lock(database_mutex_);
        
        if (vectors_.empty()) {
            return {};
        }
        
        // Use priority queue to maintain top-k results
        std::priority_queue<SearchResult, std::vector<SearchResult>, std::greater<SearchResult>> pq;
        
        for (const auto& pair : vectors_) {
            float distance = calculateDistance(query, pair.second);
            
            if (pq.size() < k) {
                pq.emplace(pair.first, distance, pair.second);
            } else if (distance < pq.top().distance) {
                pq.pop();
                pq.emplace(pair.first, distance, pair.second);
            }
        }
        
        // Convert to vector and sort by distance
        std::vector<SearchResult> results;
        results.reserve(pq.size());
        
        while (!pq.empty()) {
            results.push_back(pq.top());
            pq.pop();
        }
        
        std::sort(results.begin(), results.end(), 
                  [](const SearchResult& a, const SearchResult& b) {
                      return a.distance < b.distance;
                  });
        
        return results;
    }
    
    std::vector<SearchResult> search_radius(const std::vector<float>& query, float radius) const {
        if (!validateVector(query)) {
            std::cerr << "Error: Query vector dimension mismatch" << std::endl;
            return {};
        }
        
        std::lock_guard<std::mutex> lock(database_mutex_);
        
        std::vector<SearchResult> results;
        
        for (const auto& pair : vectors_) {
            float distance = calculateDistance(query, pair.second);
            if (distance <= radius) {
                results.emplace_back(pair.first, distance, pair.second);
            }
        }
        
        // Sort by distance
        std::sort(results.begin(), results.end(),
                  [](const SearchResult& a, const SearchResult& b) {
                      return a.distance < b.distance;
                  });
        
        return results;
    }
    
    // Database operations
    bool save(const std::string& filepath) const {
        std::lock_guard<std::mutex> lock(database_mutex_);
        
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file for writing: " << filepath << std::endl;
            return false;
        }
        
        // Write header
        file.write(reinterpret_cast<const char*>(&dimension_), sizeof(dimension_));
        size_t vector_count = vectors_.size();
        file.write(reinterpret_cast<const char*>(&vector_count), sizeof(vector_count));
        
        // Write vectors
        for (const auto& pair : vectors_) {
            size_t id_length = pair.first.length();
            file.write(reinterpret_cast<const char*>(&id_length), sizeof(id_length));
            file.write(pair.first.c_str(), id_length);
            file.write(reinterpret_cast<const char*>(pair.second.data()), 
                      dimension_ * sizeof(float));
        }
        
        return file.good();
    }
    
    bool load(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file for reading: " << filepath << std::endl;
            return false;
        }
        
        std::lock_guard<std::mutex> lock(database_mutex_);
        
        // Read header
        size_t file_dimension;
        file.read(reinterpret_cast<char*>(&file_dimension), sizeof(file_dimension));
        
        if (file_dimension != dimension_) {
            std::cerr << "Error: File dimension mismatch. Expected " << dimension_ 
                      << ", got " << file_dimension << std::endl;
            return false;
        }

        
        size_t vector_count;
        file.read(reinterpret_cast<char*>(&vector_count), sizeof(vector_count));
        
        // Clear existing vectors
        vectors_.clear();
        
        // Read vectors
        for (size_t i = 0; i < vector_count; ++i) {
            size_t id_length;
            file.read(reinterpret_cast<char*>(&id_length), sizeof(id_length));
            
            std::string id(id_length, '\0');
            file.read(&id[0], id_length);
            
            std::vector<float> vector(dimension_);
            file.read(reinterpret_cast<char*>(vector.data()), dimension_ * sizeof(float));
            
            vectors_[id] = std::move(vector);
        }
        
        return file.good();
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(database_mutex_);
        vectors_.clear();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(database_mutex_);
        return vectors_.size();
    }
    
    size_t dimension() const {
        return dimension_;
    }
    
    // Get vector by ID
    std::vector<float> get_vector(const std::string& id) const {
        std::lock_guard<std::mutex> lock(database_mutex_);
        auto it = vectors_.find(id);
        if (it != vectors_.end()) {
            return it->second;
        }
        return {};
    }
    
    // Check if vector exists
    bool exists(const std::string& id) const {
        std::lock_guard<std::mutex> lock(database_mutex_);
        return vectors_.find(id) != vectors_.end();
    }
    
    // Remove vector
    bool remove(const std::string& id) {
        std::lock_guard<std::mutex> lock(database_mutex_);
        return vectors_.erase(id) > 0;
    }
    
    // Get all vector IDs
    std::vector<std::string> get_all_ids() const {
        std::lock_guard<std::mutex> lock(database_mutex_);
        std::vector<std::string> ids;
        ids.reserve(vectors_.size());
        
        for (const auto& pair : vectors_) {
            ids.push_back(pair.first);
        }
        
        return ids;
    }
    
    // Display database stats
    void print_stats() const {
        std::lock_guard<std::mutex> lock(database_mutex_);
        
        std::cout << "=== VectorDatabase Statistics ===" << std::endl;
        std::cout << "Vector Dimension: " << dimension_ << std::endl;
        std::cout << "Total Vectors: " << vectors_.size() << std::endl;
        std::cout << "Max Capacity: " << config_.max_vectors << std::endl;
        std::cout << "Distance Metric: ";
        
        switch (config_.distance_metric) {
            case DistanceMetric::EUCLIDEAN:
                std::cout << "Euclidean" << std::endl;
                break;
            case DistanceMetric::COSINE:
                std::cout << "Cosine" << std::endl;
                break;
            case DistanceMetric::MANHATTAN:
                std::cout << "Manhattan" << std::endl;
                break;
            case DistanceMetric::DOT_PRODUCT:
                std::cout << "Dot Product" << std::endl;
                break;
        }
        
        std::cout << "Index Type: ";
        switch (config_.index_type) {
            case IndexType::LINEAR:
                std::cout << "Linear" << std::endl;
                break;
            case IndexType::KD_TREE:
                std::cout << "KD-Tree" << std::endl;
                break;
            case IndexType::HASH_TABLE:
                std::cout << "Hash Table" << std::endl;
                break;
        }
        
        std::cout << "Memory Usage (approx): " 
                  << (vectors_.size() * dimension_ * sizeof(float)) / (1024 * 1024) 
                  << " MB" << std::endl;
        std::cout << "=================================" << std::endl;
    }
    
    // Performance benchmark for high-dimensional vectors
    void benchmark_search(size_t num_queries = 100) const {
        if (vectors_.empty()) {
            std::cout << "Cannot benchmark: database is empty" << std::endl;
            return;
        }
        
        std::cout << "\n=== Search Performance Benchmark ===" << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < num_queries; ++i) {
            auto query = VectorUtils::generateRandomVector(dimension_);
            auto results = search(query, 10);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double avg_time = static_cast<double>(duration.count()) / num_queries;
        double queries_per_second = 1000000.0 / avg_time;
        
        std::cout << "Queries: " << num_queries << std::endl;
        std::cout << "Total time: " << duration.count() / 1000.0 << " ms" << std::endl;
        std::cout << "Average time per query: " << avg_time << " μs" << std::endl;
        std::cout << "Queries per second: " << std::fixed << std::setprecision(1) << queries_per_second << std::endl;
        std::cout << "=====================================" << std::endl;
    }
};

// Demo function to show basic usage
void demo_basic_usage() {
    std::cout << "=== VectorDatabase Demo ===" << std::endl;
    
    // Create a 3D vector database
    VectorDatabase db(3);
    
    // Insert some sample vectors
    db.insert("vector1", {1.0f, 2.0f, 3.0f});
    db.insert("vector2", {2.0f, 3.0f, 4.0f});
    db.insert("vector3", {0.0f, 1.0f, 2.0f});
    db.insert("vector4", {3.0f, 4.0f, 5.0f});
    db.insert("vector5", {1.5f, 2.5f, 3.5f});
    
    std::cout << "Inserted 5 vectors into the database." << std::endl;
    
    // Print database stats
    db.print_stats();
    
    // Search for similar vectors
    std::vector<float> query = {1.1f, 2.1f, 3.1f};
    std::cout << "\nSearching for vectors similar to [1.1, 2.1, 3.1]:" << std::endl;
    
    auto results = db.search(query, 3);  // Find top 3 similar vectors
    
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << "Rank " << (i + 1) << ": ID='" << results[i].id 
                  << "', Distance=" << results[i].distance;
        std::cout << ", Vector=[";
        for (size_t j = 0; j < results[i].vector.size(); ++j) {
            std::cout << results[i].vector[j];
            if (j < results[i].vector.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }
}

// Demo function for high-dimensional vectors
void demo_high_dimensional() {
    std::cout << "\n=== High-Dimensional Vector Demo ===" << std::endl;
    
    // Test with different dimensions
    std::vector<size_t> dimensions = {10, 50, 128, 512, 1024};
    
    for (size_t dim : dimensions) {
        std::cout << "\n--- Testing " << dim << "D vectors ---" << std::endl;
        
        VectorDatabase db(dim);
        
        // Insert random vectors
        std::cout << "Inserting 100 random vectors..." << std::endl;
        db.insertRandomVectors(100);
        
        // Create a query vector
        auto query = db.generateRandomVector();
        std::cout << "Query vector: ";
        VectorUtils::printVector(query);
        std::cout << std::endl;
        
        // Search for similar vectors
        auto results = db.search(query, 5);
        std::cout << "Top 5 similar vectors:" << std::endl;
        
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "  " << (i + 1) << ". ID: " << results[i].id 
                      << ", Distance: " << std::fixed << std::setprecision(4) << results[i].distance;
            std::cout << ", Vector: ";
            VectorUtils::printVector(results[i].vector);
            std::cout << std::endl;
        }
        
        // Show stats
        db.print_stats();
        
        // Benchmark performance
        db.benchmark_search(50);
    }
}

// Demo function for clustered high-dimensional data
void demo_clustered_data() {
    std::cout << "\n=== Clustered High-Dimensional Data Demo ===" << std::endl;
    
    const size_t dimension = 256;  // High-dimensional space
    VectorDatabase db(dimension);
    
    // Create cluster centers
    std::vector<std::vector<float>> centers;
    centers.push_back(VectorUtils::generateRandomVector(dimension, 0.0f, 1.0f));    // Cluster 1
    centers.push_back(VectorUtils::generateRandomVector(dimension, -1.0f, 0.0f));   // Cluster 2
    centers.push_back(VectorUtils::generateRandomVector(dimension, 0.5f, 1.5f));    // Cluster 3
    
    std::cout << "Created 3 cluster centers in " << dimension << "D space" << std::endl;
    
    // Insert clustered data
    std::cout << "Inserting 50 vectors per cluster (150 total vectors)..." << std::endl;
    db.insertClusteredVectors(centers, 50, 0.1f);
    
    // Insert the centers themselves
    for (size_t i = 0; i < centers.size(); ++i) {
        db.insert("center_" + std::to_string(i), centers[i]);
    }
    
    db.print_stats();
    
    // Test queries near each cluster center
    for (size_t i = 0; i < centers.size(); ++i) {
        std::cout << "\n--- Querying near cluster " << i << " ---" << std::endl;
        
        // Create query slightly offset from center
        auto query = VectorUtils::generateGaussianVector(centers[i], 0.05f);
        
        auto results = db.search(query, 10);
        
        std::cout << "Top 10 results for cluster " << i << " query:" << std::endl;
        for (size_t j = 0; j < std::min(results.size(), static_cast<size_t>(5)); ++j) {
            std::cout << "  " << (j + 1) << ". " << results[j].id 
                      << " (distance: " << std::fixed << std::setprecision(4) << results[j].distance << ")" << std::endl;
        }
    }
    
    // Benchmark high-dimensional search
    std::cout << "\nBenchmarking high-dimensional search performance:" << std::endl;
    db.benchmark_search(100);
}

// Demo function for different distance metrics
void demo_distance_metrics() {
    std::cout << "\n=== Distance Metrics Demo ===" << std::endl;
    
    std::vector<float> vec1 = {1.0f, 0.0f, 0.0f};
    std::vector<float> vec2 = {0.0f, 1.0f, 0.0f};
    
    // Test different distance metrics
    std::vector<DistanceMetric> metrics = {
        DistanceMetric::EUCLIDEAN,
        DistanceMetric::COSINE,
        DistanceMetric::MANHATTAN,
        DistanceMetric::DOT_PRODUCT
    };
    
    std::vector<std::string> metric_names = {
        "Euclidean", "Cosine", "Manhattan", "Dot Product"
    };
    
    for (size_t i = 0; i < metrics.size(); ++i) {
        VectorDatabaseConfig config;
        config.distance_metric = metrics[i];
        
        VectorDatabase db(3, config);
        db.insert("vec1", vec1);
        db.insert("vec2", vec2);
        
        auto results = db.search(vec1, 2);
        
        std::cout << "\n" << metric_names[i] << " Distance:" << std::endl;
        for (const auto& result : results) {
            std::cout << "  " << result.id << ": " << result.distance << std::endl;
        }
    }
}

// Demo function showcasing various dimensional support
void demo_dimensional_flexibility() {
    std::cout << "\n=== N-Dimensional Flexibility Demo ===" << std::endl;
    
    // Test extreme dimensions
    std::vector<size_t> test_dimensions = {1, 2, 3, 10, 100, 1000, 10000};
    
    for (size_t dim : test_dimensions) {
        std::cout << "\n--- " << dim << "D Vector Database ---" << std::endl;
        
        try {
            VectorDatabase db(dim);
            
            // Insert a few vectors
            for (int i = 0; i < 5; ++i) {
                auto vec = db.generateRandomVector(-1.0f, 1.0f);
                db.insert("vec_" + std::to_string(i), vec);
            }
            
            // Perform a search
            auto query = db.generateRandomVector();
            auto results = db.search(query, 3);
            
            std::cout << "Successfully created and searched " << dim << "D database" << std::endl;
            std::cout << "Inserted: " << db.size() << " vectors" << std::endl;
            std::cout << "Memory usage: " << (db.size() * dim * sizeof(float)) / 1024 << " KB" << std::endl;
            
            if (!results.empty()) {
                std::cout << "Best match: " << results[0].id 
                          << " (distance: " << std::fixed << std::setprecision(6) << results[0].distance << ")" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cout << "Error with " << dim << "D: " << e.what() << std::endl;
        }
    }
}

int main() {
    std::cout << "VectorDatabase C++ Implementation - N-Dimensional Support" << std::endl;
    std::cout << "=========================================================" << std::endl;
    
    try {
        demo_basic_usage();
        demo_dimensional_flexibility();
        demo_high_dimensional();
        demo_clustered_data();
        demo_distance_metrics();
        
        std::cout << "\nAll demos completed successfully!" << std::endl;
        std::cout << "\nThe VectorDatabase supports vectors of ANY dimension (1D to 10,000D+)!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
