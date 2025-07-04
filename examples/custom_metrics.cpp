// custom_metrics.cpp - Using custom distance metrics example
// Demonstrates different distance metrics and their characteristics

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <iomanip>

// Include the VectorDatabase implementation
#include "../VectorDatabase.cpp"

// Helper function to create test vectors with known relationships
std::vector<std::vector<float>> createTestVectors() {
    return {
        {1.0f, 0.0f, 0.0f},    // Unit vector along X-axis
        {0.0f, 1.0f, 0.0f},    // Unit vector along Y-axis  
        {0.0f, 0.0f, 1.0f},    // Unit vector along Z-axis
        {0.707f, 0.707f, 0.0f}, // 45-degree angle in XY plane
        {0.577f, 0.577f, 0.577f}, // Equal components (normalized)
        {2.0f, 0.0f, 0.0f},    // Scaled version of first vector
        {-1.0f, 0.0f, 0.0f},   // Opposite direction of first vector
        {0.5f, 0.5f, 0.0f},    // Same direction as 4th, different magnitude
        {0.0f, 0.0f, 0.0f},    // Zero vector
        {1.0f, 1.0f, 1.0f}     // All ones vector
    };
}

std::vector<std::string> getTestVectorNames() {
    return {
        "unit_x", "unit_y", "unit_z", "xy_diagonal", "xyz_equal",
        "scaled_x", "negative_x", "small_xy", "zero", "ones"
    };
}

// Function to analyze distance metric behavior
void analyzeDistanceMetric(DistanceMetric metric, const std::string& metric_name) {
    std::cout << "\n=== " << metric_name << " Distance Analysis ===" << std::endl;
    
    VectorDatabaseConfig config;
    config.distance_metric = metric;
    
    VectorDatabase db(3, config);
    
    auto test_vectors = createTestVectors();
    auto vector_names = getTestVectorNames();
    
    // Insert test vectors
    for (size_t i = 0; i < test_vectors.size(); ++i) {
        db.insert(vector_names[i], test_vectors[i]);
    }
    
    // Test various query scenarios
    std::vector<std::pair<std::vector<float>, std::string>> test_queries = {
        {{1.0f, 0.0f, 0.0f}, "Query: unit_x [1, 0, 0]"},
        {{0.0f, 1.0f, 0.0f}, "Query: unit_y [0, 1, 0]"},
        {{0.707f, 0.707f, 0.0f}, "Query: normalized diagonal [0.707, 0.707, 0]"},
        {{2.0f, 2.0f, 0.0f}, "Query: scaled diagonal [2, 2, 0]"},
        {{-1.0f, 0.0f, 0.0f}, "Query: negative_x [-1, 0, 0]"}
    };
    
    for (const auto& query_pair : test_queries) {
        std::cout << "\n" << query_pair.second << ":" << std::endl;
        
        auto results = db.search(query_pair.first, 5);
        
        for (size_t i = 0; i < std::min(results.size(), static_cast<size_t>(5)); ++i) {
            std::cout << "  " << (i + 1) << ". " << std::setw(12) << results[i].id 
                      << " distance: " << std::fixed << std::setprecision(4) << results[i].distance;
            
            // Show the actual vector for context
            std::cout << " [";
            for (size_t j = 0; j < results[i].vector.size(); ++j) {
                std::cout << std::fixed << std::setprecision(3) << results[i].vector[j];
                if (j < results[i].vector.size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }
    }
}

// Function to compare all distance metrics side by side
void compareDistanceMetrics() {
    std::cout << "\n=== Distance Metrics Comparison ===" << std::endl;
    
    auto test_vectors = createTestVectors();
    auto vector_names = getTestVectorNames();
    
    // Test vector pairs for comparison
    std::vector<std::pair<std::vector<float>, std::vector<float>>> test_pairs = {
        {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // Orthogonal unit vectors
        {{1.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}},  // Same direction, different magnitude
        {{1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}}, // Opposite directions
        {{1.0f, 1.0f, 0.0f}, {0.707f, 0.707f, 0.0f}}, // Same direction, different magnitude
        {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}}   // Any vector vs zero
    };
    
    std::vector<std::string> pair_descriptions = {
        "Orthogonal unit vectors",
        "Same direction, different magnitude", 
        "Opposite directions",
        "Same direction (normalized vs not)",
        "Any vector vs zero vector"
    };
    
    std::vector<DistanceMetric> metrics = {
        DistanceMetric::EUCLIDEAN,
        DistanceMetric::COSINE,
        DistanceMetric::MANHATTAN,
        DistanceMetric::DOT_PRODUCT
    };
    
    std::vector<std::string> metric_names = {
        "Euclidean", "Cosine", "Manhattan", "Dot Product"
    };
    
    // Print header
    std::cout << std::setw(35) << "Vector Pair";
    for (const auto& name : metric_names) {
        std::cout << std::setw(12) << name;
    }
    std::cout << std::endl;
    std::cout << std::string(35 + 12 * metric_names.size(), '-') << std::endl;
    
    // Calculate distances for each pair with each metric
    for (size_t p = 0; p < test_pairs.size(); ++p) {
        std::cout << std::setw(35) << pair_descriptions[p];
        
        for (size_t m = 0; m < metrics.size(); ++m) {
            VectorDatabaseConfig config;
            config.distance_metric = metrics[m];
            VectorDatabase db(3, config);
            
            db.insert("vec1", test_pairs[p].first);
            db.insert("vec2", test_pairs[p].second);
            
            auto results = db.search(test_pairs[p].first, 2);
            
            // Find the distance to the second vector
            float distance = 0.0f;
            for (const auto& result : results) {
                if (result.id == "vec2") {
                    distance = result.distance;
                    break;
                }
            }
            
            std::cout << std::setw(12) << std::fixed << std::setprecision(4) << distance;
        }
        std::cout << std::endl;
    }
}

// Function to demonstrate metric-specific use cases
void demonstrateUseCases() {
    std::cout << "\n=== Distance Metric Use Cases ===" << std::endl;
    
    // 1. Euclidean Distance - Good for spatial data
    std::cout << "\n1. Euclidean Distance - Spatial Data Example:" << std::endl;
    std::cout << "   Use case: 2D points, RGB color similarity, feature vectors" << std::endl;
    
    VectorDatabaseConfig euclidean_config;
    euclidean_config.distance_metric = DistanceMetric::EUCLIDEAN;
    VectorDatabase spatial_db(2, euclidean_config);
    
    // Insert 2D points representing locations
    spatial_db.insert("home", {0.0f, 0.0f});
    spatial_db.insert("work", {5.0f, 3.0f});
    spatial_db.insert("store", {2.0f, 1.0f});
    spatial_db.insert("park", {1.0f, 4.0f});
    spatial_db.insert("school", {3.0f, 2.0f});
    
    std::vector<float> current_location = {1.5f, 1.5f};
    auto nearest_places = spatial_db.search(current_location, 3);
    
    std::cout << "   Current location: [1.5, 1.5]" << std::endl;
    std::cout << "   Nearest places:" << std::endl;
    for (const auto& place : nearest_places) {
        std::cout << "     " << place.id << " (distance: " << std::fixed << std::setprecision(2) 
                  << place.distance << ")" << std::endl;
    }
    
    // 2. Cosine Distance - Good for document similarity, normalized features
    std::cout << "\n2. Cosine Distance - Document Similarity Example:" << std::endl;
    std::cout << "   Use case: Text similarity, user preferences, normalized features" << std::endl;
    
    VectorDatabaseConfig cosine_config;
    cosine_config.distance_metric = DistanceMetric::COSINE;
    VectorDatabase doc_db(4, cosine_config);
    
    // Insert document feature vectors (word frequencies)
    doc_db.insert("sports_article", {10.0f, 2.0f, 0.0f, 1.0f});     // High sports terms
    doc_db.insert("tech_article", {1.0f, 15.0f, 8.0f, 0.0f});       // High tech terms
    doc_db.insert("cooking_recipe", {0.0f, 1.0f, 2.0f, 12.0f});     // High cooking terms
    doc_db.insert("sports_tech", {8.0f, 10.0f, 3.0f, 0.0f});        // Mixed sports/tech
    
    std::vector<float> query_doc = {12.0f, 3.0f, 1.0f, 0.0f}; // Sports-focused query
    auto similar_docs = doc_db.search(query_doc, 3);
    
    std::cout << "   Query document features: [12, 3, 1, 0] (sports-focused)" << std::endl;
    std::cout << "   Most similar documents:" << std::endl;
    for (const auto& doc : similar_docs) {
        std::cout << "     " << doc.id << " (cosine distance: " << std::fixed << std::setprecision(4) 
                  << doc.distance << ")" << std::endl;
    }
    
    // 3. Manhattan Distance - Good for categorical data, sparse vectors
    std::cout << "\n3. Manhattan Distance - Categorical Data Example:" << std::endl;
    std::cout << "   Use case: Categorical features, sparse data, preference vectors" << std::endl;
    
    VectorDatabaseConfig manhattan_config;
    manhattan_config.distance_metric = DistanceMetric::MANHATTAN;
    VectorDatabase category_db(5, manhattan_config);
    
    // Insert user preference vectors (ratings 0-5 for different categories)
    category_db.insert("user_alice", {5.0f, 2.0f, 4.0f, 1.0f, 3.0f});   // Loves movies, cooking
    category_db.insert("user_bob", {1.0f, 5.0f, 2.0f, 4.0f, 3.0f});     // Loves sports, games  
    category_db.insert("user_carol", {4.0f, 1.0f, 5.0f, 2.0f, 4.0f});   // Loves movies, music
    category_db.insert("user_david", {2.0f, 4.0f, 3.0f, 5.0f, 2.0f});   // Loves sports, games
    
    std::vector<float> new_user = {4.0f, 2.0f, 4.0f, 2.0f, 3.0f}; // Similar to Alice/Carol
    auto similar_users = category_db.search(new_user, 3);
    
    std::cout << "   New user preferences: [4, 2, 4, 2, 3] (movies, sports, cooking, games, music)" << std::endl;
    std::cout << "   Most similar users:" << std::endl;
    for (const auto& user : similar_users) {
        std::cout << "     " << user.id << " (Manhattan distance: " << std::fixed << std::setprecision(1) 
                  << user.distance << ")" << std::endl;
    }
    
    // 4. Dot Product - Good for similarity scoring, recommendation systems
    std::cout << "\n4. Dot Product Distance - Recommendation System Example:" << std::endl;
    std::cout << "   Use case: Recommendation systems, similarity scoring, neural networks" << std::endl;
    
    VectorDatabaseConfig dot_config;
    dot_config.distance_metric = DistanceMetric::DOT_PRODUCT;
    VectorDatabase rec_db(4, dot_config);
    
    // Insert item feature vectors (positive values indicate strong features)
    rec_db.insert("action_movie", {0.9f, 0.1f, 0.8f, 0.2f});      // High action, adventure
    rec_db.insert("romance_movie", {0.1f, 0.9f, 0.2f, 0.7f});     // High romance, drama
    rec_db.insert("comedy_movie", {0.3f, 0.4f, 0.1f, 0.9f});      // High comedy
    rec_db.insert("thriller_movie", {0.7f, 0.2f, 0.9f, 0.3f});    // High action, suspense
    
    std::vector<float> user_preferences = {0.8f, 0.3f, 0.9f, 0.1f}; // Likes action/suspense
    auto recommendations = rec_db.search(user_preferences, 3);
    
    std::cout << "   User preferences: [0.8, 0.3, 0.9, 0.1] (action, romance, suspense, comedy)" << std::endl;
    std::cout << "   Recommended movies (higher negative distance = better match):" << std::endl;
    for (const auto& movie : recommendations) {
        std::cout << "     " << movie.id << " (dot product distance: " << std::fixed << std::setprecision(4) 
                  << movie.distance << ")" << std::endl;
    }
}

// Function to show when different metrics give different rankings
void showRankingDifferences() {
    std::cout << "\n=== Ranking Differences Between Metrics ===" << std::endl;
    
    const size_t dimension = 3;
    std::vector<float> query = {1.0f, 1.0f, 0.0f};
    
    // Test vectors designed to show ranking differences
    std::map<std::string, std::vector<float>> test_vectors = {
        {"vector_a", {2.0f, 2.0f, 0.0f}},     // Same direction, larger magnitude
        {"vector_b", {0.5f, 0.5f, 0.0f}},     // Same direction, smaller magnitude  
        {"vector_c", {1.0f, 0.0f, 1.0f}},     // Orthogonal, same magnitude
        {"vector_d", {0.0f, 0.0f, 2.0f}},     // Orthogonal, larger magnitude
        {"vector_e", {1.5f, 0.5f, 0.0f}}      // Similar direction, different magnitude
    };
    
    std::vector<DistanceMetric> metrics = {
        DistanceMetric::EUCLIDEAN,
        DistanceMetric::COSINE,
        DistanceMetric::MANHATTAN,
        DistanceMetric::DOT_PRODUCT
    };
    
    std::vector<std::string> metric_names = {
        "Euclidean", "Cosine", "Manhattan", "Dot Product"
    };
    
    std::cout << "Query vector: [1, 1, 0]" << std::endl;
    std::cout << "\nRanking comparison:" << std::endl;
    
    for (size_t m = 0; m < metrics.size(); ++m) {
        VectorDatabaseConfig config;
        config.distance_metric = metrics[m];
        VectorDatabase db(dimension, config);
        
        // Insert test vectors
        for (const auto& pair : test_vectors) {
            db.insert(pair.first, pair.second);
        }
        
        auto results = db.search(query, test_vectors.size());
        
        std::cout << "\n" << metric_names[m] << " ranking:" << std::endl;
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << std::setw(10) << results[i].id 
                      << " (distance: " << std::fixed << std::setprecision(4) << results[i].distance << ")";
            
            // Show vector
            std::cout << " [";
            for (size_t j = 0; j < results[i].vector.size(); ++j) {
                std::cout << std::fixed << std::setprecision(1) << results[i].vector[j];
                if (j < results[i].vector.size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }
    }
}

int main() {
    std::cout << "=== VectorDatabase Custom Distance Metrics Example ===" << std::endl;
    
    try {
        // Analyze each distance metric individually
        analyzeDistanceMetric(DistanceMetric::EUCLIDEAN, "Euclidean");
        analyzeDistanceMetric(DistanceMetric::COSINE, "Cosine");
        analyzeDistanceMetric(DistanceMetric::MANHATTAN, "Manhattan (L1)");
        analyzeDistanceMetric(DistanceMetric::DOT_PRODUCT, "Dot Product");
        
        // Compare metrics side by side
        compareDistanceMetrics();
        
        // Show real-world use cases
        demonstrateUseCases();
        
        // Show how rankings differ between metrics
        showRankingDifferences();
        
        std::cout << "\n=== Distance Metric Guidelines ===" << std::endl;
        std::cout << "• Euclidean:    Best for spatial data, image features, when magnitude matters" << std::endl;
        std::cout << "• Cosine:       Best for text similarity, normalized features, when direction matters" << std::endl;
        std::cout << "• Manhattan:    Best for categorical data, sparse vectors, when differences should be linear" << std::endl;
        std::cout << "• Dot Product:  Best for recommendation systems, when similarity scoring is needed" << std::endl;
        
        std::cout << "\n=== Custom Metrics Example Completed Successfully! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 