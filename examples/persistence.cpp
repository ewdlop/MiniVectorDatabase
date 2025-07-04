// persistence.cpp - Saving and loading databases example
// Demonstrates database persistence, backup/restore, and data integrity

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <fstream>

// Include the VectorDatabase implementation
#include "../VectorDatabase.cpp"

// Utility function to check if file exists
bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

// Utility function to get file size
size_t getFileSize(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 0;
    return static_cast<size_t>(file.tellg());
}

// Function to create a sample dataset
void createSampleDataset(VectorDatabase& db, size_t count, const std::string& prefix = "sample") {
    std::cout << "Creating sample dataset with " << count << " vectors..." << std::endl;
    
    for (size_t i = 0; i < count; ++i) {
        auto vector = db.generateRandomVector(-1.0f, 1.0f);
        std::string id = prefix + "_" + std::to_string(i);
        
        if (!db.insert(id, vector)) {
            std::cerr << "Failed to insert vector " << id << std::endl;
            break;
        }
        
        // Progress indicator for large datasets
        if (count > 1000 && (i + 1) % 1000 == 0) {
            std::cout << "  Inserted " << (i + 1) << "/" << count << " vectors..." << std::endl;
        }
    }
    
    std::cout << "✓ Created dataset with " << db.size() << " vectors" << std::endl;
}

// Function to verify data integrity
bool verifyDataIntegrity(const VectorDatabase& original_db, const VectorDatabase& loaded_db) {
    std::cout << "\nVerifying data integrity..." << std::endl;
    
    // Check sizes match
    if (original_db.size() != loaded_db.size()) {
        std::cout << "✗ Size mismatch: original=" << original_db.size() 
                  << ", loaded=" << loaded_db.size() << std::endl;
        return false;
    }
    
    // Check dimensions match
    if (original_db.dimension() != loaded_db.dimension()) {
        std::cout << "✗ Dimension mismatch: original=" << original_db.dimension() 
                  << ", loaded=" << loaded_db.dimension() << std::endl;
        return false;
    }
    
    // Check all vector IDs exist
    auto original_ids = original_db.get_all_ids();
    std::cout << "  Checking " << original_ids.size() << " vector IDs..." << std::endl;
    
    size_t checked_count = 0;
    for (const auto& id : original_ids) {
        if (!loaded_db.exists(id)) {
            std::cout << "✗ Missing vector ID: " << id << std::endl;
            return false;
        }
        
        // Compare actual vector data
        auto original_vector = original_db.get_vector(id);
        auto loaded_vector = loaded_db.get_vector(id);
        
        if (original_vector.size() != loaded_vector.size()) {
            std::cout << "✗ Vector size mismatch for ID: " << id << std::endl;
            return false;
        }
        
        // Check vector components (with floating point tolerance)
        const float tolerance = 1e-6f;
        for (size_t i = 0; i < original_vector.size(); ++i) {
            if (std::abs(original_vector[i] - loaded_vector[i]) > tolerance) {
                std::cout << "✗ Vector data mismatch for ID: " << id 
                          << " at component " << i << std::endl;
                return false;
            }
        }
        
        checked_count++;
        if (checked_count % 100 == 0 && original_ids.size() > 500) {
            std::cout << "    Verified " << checked_count << "/" << original_ids.size() << " vectors..." << std::endl;
        }
    }
    
    std::cout << "✓ Data integrity verified - all " << checked_count << " vectors match perfectly" << std::endl;
    return true;
}

// Function to test basic save/load functionality
void testBasicPersistence() {
    std::cout << "\n=== Basic Persistence Test ===" << std::endl;
    
    const size_t dimension = 64;
    const size_t vector_count = 100;
    const std::string filename = "test_basic.vdb";
    
    // Create and populate database
    VectorDatabase original_db(dimension);
    createSampleDataset(original_db, vector_count, "basic");
    
    std::cout << "\nOriginal database stats:" << std::endl;
    original_db.print_stats();
    
    // Save database
    std::cout << "\nSaving database to '" << filename << "'..." << std::endl;
    auto start_save = std::chrono::high_resolution_clock::now();
    
    if (!original_db.save(filename)) {
        std::cout << "✗ Failed to save database" << std::endl;
        return;
    }
    
    auto end_save = std::chrono::high_resolution_clock::now();
    auto save_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_save - start_save);
    
    size_t file_size = getFileSize(filename);
    std::cout << "✓ Database saved successfully" << std::endl;
    std::cout << "  File size: " << std::fixed << std::setprecision(2) 
              << (file_size / 1024.0) << " KB" << std::endl;
    std::cout << "  Save time: " << save_duration.count() << " ms" << std::endl;
    
    // Load database
    std::cout << "\nLoading database from '" << filename << "'..." << std::endl;
    auto start_load = std::chrono::high_resolution_clock::now();
    
    VectorDatabase loaded_db(dimension);
    if (!loaded_db.load(filename)) {
        std::cout << "✗ Failed to load database" << std::endl;
        return;
    }
    
    auto end_load = std::chrono::high_resolution_clock::now();
    auto load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_load - start_load);
    
    std::cout << "✓ Database loaded successfully" << std::endl;
    std::cout << "  Load time: " << load_duration.count() << " ms" << std::endl;
    
    std::cout << "\nLoaded database stats:" << std::endl;
    loaded_db.print_stats();
    
    // Verify integrity
    verifyDataIntegrity(original_db, loaded_db);
    
    // Test search functionality
    std::cout << "\nTesting search functionality on loaded database..." << std::endl;
    auto query = loaded_db.generateRandomVector();
    auto results = loaded_db.search(query, 5);
    
    std::cout << "✓ Search returned " << results.size() << " results" << std::endl;
    for (size_t i = 0; i < std::min(results.size(), static_cast<size_t>(3)); ++i) {
        std::cout << "  " << (i+1) << ". " << results[i].id 
                  << " (distance: " << std::fixed << std::setprecision(4) << results[i].distance << ")" << std::endl;
    }
    
    // Clean up
    std::remove(filename.c_str());
    std::cout << "✓ Test file cleaned up" << std::endl;
}

int main() {
    std::cout << "=== VectorDatabase Persistence Example ===" << std::endl;
    std::cout << "Testing save/load functionality and data integrity" << std::endl;
    
    try {
        testBasicPersistence();
        
        std::cout << "\n=== Persistence Example Completed Successfully! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 