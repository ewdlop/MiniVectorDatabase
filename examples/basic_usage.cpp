// basic_usage.cpp - Simple vector storage and search example
// Demonstrates the fundamental operations of VectorDatabase

#include <iostream>
#include <vector>
#include <string>

// Include the VectorDatabase header (assuming it's been separated)
// For this example, we'll include the main implementation
#include "../VectorDatabase.cpp"

int main() {
    std::cout << "=== VectorDatabase Basic Usage Example ===" << std::endl;
    
    try {
        // 1. Create a vector database for 4-dimensional vectors
        std::cout << "\n1. Creating a 4D vector database..." << std::endl;
        VectorDatabase db(4);
        
        // 2. Insert individual vectors
        std::cout << "\n2. Inserting individual vectors..." << std::endl;
        
        // Insert some sample 4D vectors representing different categories
        db.insert("apple", {0.8f, 0.1f, 0.2f, 0.9f});      // Red fruit
        db.insert("banana", {0.9f, 0.9f, 0.1f, 0.8f});     // Yellow fruit
        db.insert("grass", {0.1f, 0.8f, 0.1f, 0.7f});      // Green plant
        db.insert("sky", {0.2f, 0.3f, 0.9f, 0.6f});        // Blue sky
        db.insert("orange", {0.9f, 0.5f, 0.1f, 0.8f});     // Orange fruit
        db.insert("leaf", {0.2f, 0.7f, 0.2f, 0.6f});       // Green leaf
        
        std::cout << "Inserted " << db.size() << " vectors successfully!" << std::endl;
        
        // 3. Display database statistics
        std::cout << "\n3. Database statistics:" << std::endl;
        db.print_stats();
        
        // 4. Perform similarity searches
        std::cout << "\n4. Similarity search examples:" << std::endl;
        
        // Search for vectors similar to a red object
        std::vector<float> red_query = {0.9f, 0.1f, 0.1f, 0.8f};
        std::cout << "\nSearching for red-like objects [0.9, 0.1, 0.1, 0.8]:" << std::endl;
        auto red_results = db.search(red_query, 3);
        
        for (size_t i = 0; i < red_results.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << red_results[i].id 
                      << " (distance: " << std::fixed << std::setprecision(4) 
                      << red_results[i].distance << ")" << std::endl;
        }
        
        // Search for vectors similar to a green object
        std::vector<float> green_query = {0.1f, 0.8f, 0.1f, 0.7f};
        std::cout << "\nSearching for green-like objects [0.1, 0.8, 0.1, 0.7]:" << std::endl;
        auto green_results = db.search(green_query, 3);
        
        for (size_t i = 0; i < green_results.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << green_results[i].id 
                      << " (distance: " << std::fixed << std::setprecision(4) 
                      << green_results[i].distance << ")" << std::endl;
        }
        
        // 5. Radius search
        std::cout << "\n5. Radius search example:" << std::endl;
        std::vector<float> fruit_query = {0.8f, 0.4f, 0.2f, 0.8f};
        std::cout << "Finding all objects within distance 0.5 of [0.8, 0.4, 0.2, 0.8]:" << std::endl;
        auto radius_results = db.search_radius(fruit_query, 0.5f);
        
        if (radius_results.empty()) {
            std::cout << "  No objects found within radius 0.5" << std::endl;
        } else {
            for (const auto& result : radius_results) {
                std::cout << "  - " << result.id 
                          << " (distance: " << std::fixed << std::setprecision(4) 
                          << result.distance << ")" << std::endl;
            }
        }
        
        // 6. Check if vectors exist
        std::cout << "\n6. Checking vector existence:" << std::endl;
        std::vector<std::string> check_ids = {"apple", "grape", "sky", "ocean"};
        for (const auto& id : check_ids) {
            std::cout << "  '" << id << "': " << (db.exists(id) ? "EXISTS" : "NOT FOUND") << std::endl;
        }
        
        // 7. Retrieve specific vectors
        std::cout << "\n7. Retrieving specific vectors:" << std::endl;
        auto apple_vector = db.get_vector("apple");
        if (!apple_vector.empty()) {
            std::cout << "  Apple vector: [";
            for (size_t i = 0; i < apple_vector.size(); ++i) {
                std::cout << apple_vector[i];
                if (i < apple_vector.size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }
        
        // 8. Get all vector IDs
        std::cout << "\n8. All vectors in database:" << std::endl;
        auto all_ids = db.get_all_ids();
        std::cout << "  Vector IDs: ";
        for (size_t i = 0; i < all_ids.size(); ++i) {
            std::cout << "'" << all_ids[i] << "'";
            if (i < all_ids.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        // 9. Remove a vector
        std::cout << "\n9. Removing a vector:" << std::endl;
        std::cout << "  Removing 'sky'..." << std::endl;
        if (db.remove("sky")) {
            std::cout << "  Successfully removed 'sky'" << std::endl;
            std::cout << "  Database now contains " << db.size() << " vectors" << std::endl;
        } else {
            std::cout << "  Failed to remove 'sky'" << std::endl;
        }
        
        // 10. Final search to confirm removal
        std::cout << "\n10. Confirming removal with final search:" << std::endl;
        auto final_results = db.search(red_query, 5);
        std::cout << "  Top " << final_results.size() << " results:" << std::endl;
        for (size_t i = 0; i < final_results.size(); ++i) {
            std::cout << "    " << (i + 1) << ". " << final_results[i].id 
                      << " (distance: " << std::fixed << std::setprecision(4) 
                      << final_results[i].distance << ")" << std::endl;
        }
        
        std::cout << "\n=== Basic Usage Example Completed Successfully! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 