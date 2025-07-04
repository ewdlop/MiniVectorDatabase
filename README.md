# VectorDatabase

A high-performance vector database implementation in C++ designed for efficient storage, indexing, and similarity search of high-dimensional vectors.

## Overview

VectorDatabase is a lightweight, fast vector database that supports:
- **High-dimensional vector storage** - Store vectors of arbitrary dimensions
- **Similarity search** - Find nearest neighbors using various distance metrics
- **Indexing algorithms** - Multiple indexing strategies for optimal performance
- **Memory efficiency** - Optimized memory usage for large vector collections
- **Cross-platform** - Builds on Windows, Linux, and macOS

## Features

- ✅ **Multiple Distance Metrics**: Euclidean, Cosine, Manhattan, and Dot Product
- [ ] **Indexing Support**: Linear search, KD-Trees, and Approximate Nearest Neighbor (ANN)
- ✅ **Batch Operations**: Insert, update, and delete vectors in batches
- ✅ **Persistence**: Save and load vector databases to/from disk
- ✅ **Thread Safety**: Multi-threaded operations support
- ✅ **Memory Management**: Efficient memory allocation and deallocation
- ✅ **Query Optimization**: Fast similarity search with configurable parameters

## Installation

### Prerequisites

- **C++17** or later
- **CMake 3.15+** (or Visual Studio 2019+)
- **Git** for cloning the repository

### Building on Windows (Visual Studio)

```bash
# Clone the repository
git clone https://github.com/yourusername/VectorDatabase.git
cd VectorDatabase

# Open the solution file
VectorDatabase.sln
```

Build using Visual Studio IDE or from command line:
```bash
# Using MSBuild
msbuild VectorDatabase.sln /p:Configuration=Release
```

## Quick Start

### Basic Usage

```cpp
#include "VectorDatabase.h"

int main() {
    // Create a vector database for 128-dimensional vectors
    VectorDatabase db(128);
    
    // Insert vectors
    std::vector<float> vector1 = {1.0f, 2.0f, 3.0f, /* ... 128 dimensions */};
    std::vector<float> vector2 = {2.0f, 3.0f, 4.0f, /* ... 128 dimensions */};
    
    db.insert("vector1", vector1);
    db.insert("vector2", vector2);
    
    // Search for similar vectors
    auto results = db.search(vector1, 5); // Find top 5 similar vectors
    
    for (const auto& result : results) {
        std::cout << "ID: " << result.id 
                  << ", Distance: " << result.distance << std::endl;
    }
    
    return 0;
}
```

### Advanced Configuration

```cpp
// Configure distance metric and indexing
VectorDatabaseConfig config;
config.distance_metric = DistanceMetric::COSINE;
config.index_type = IndexType::KD_TREE;
config.max_vectors = 1000000;

VectorDatabase db(128, config);
```

## API Reference

### Core Classes

#### `VectorDatabase`
Main database class for vector operations.

**Constructor:**
```cpp
VectorDatabase(size_t dimension);
VectorDatabase(size_t dimension, const VectorDatabaseConfig& config);
```

**Methods:**
```cpp
// Insert operations
bool insert(const std::string& id, const std::vector<float>& vector);
bool insert_batch(const std::map<std::string, std::vector<float>>& vectors);

// Search operations
std::vector<SearchResult> search(const std::vector<float>& query, size_t k);
std::vector<SearchResult> search_radius(const std::vector<float>& query, float radius);

// Database operations
bool save(const std::string& filepath);
bool load(const std::string& filepath);
void clear();
size_t size() const;
```

#### `SearchResult`
Structure containing search results.

```cpp
struct SearchResult {
    std::string id;
    float distance;
    std::vector<float> vector;
};
```

## Configuration Options

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `distance_metric` | `DistanceMetric` | `EUCLIDEAN` | Distance calculation method |
| `index_type` | `IndexType` | `LINEAR` | Indexing algorithm |
| `max_vectors` | `size_t` | `100000` | Maximum number of vectors |
| `thread_count` | `size_t` | `std::thread::hardware_concurrency()` | Number of threads for parallel operations |

## Performance

## Examples

Check out the `examples/` directory for more comprehensive examples:

- `basic_usage.cpp` - Simple vector storage and search
- `batch_operations.cpp` - Batch insert and update operations
- `custom_metrics.cpp` - Using custom distance metrics
- `persistence.cpp` - Saving and loading databases
- `benchmarks.cpp` - Performance testing

## Contributing

We welcome contributions! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request


## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Roadmap

- [ ] **GPU Acceleration** - CUDA support for vector operations
- [ ] **Distributed Storage** - Multi-node vector database clustering
- [ ] **Advanced Indexing** - LSH (Locality Sensitive Hashing) support
- [ ] **Compression** - Vector quantization for reduced memory usage
- [ ] **REST API** - HTTP interface for remote access
- [ ] **Python Bindings** - PyBind11 integration
