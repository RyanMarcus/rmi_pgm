//
// Created by Giorgio Vinciguerra on 27/04/2020.
// Modified by Ryan Marcus, 29/04/2020.
//

#include <fstream>
#include <iostream>
#include <random>
#include <chrono>
#include "rmis/wiki.h"
#include "rmis/fb.h"
#include "rmis/osm.h"
#include "rmis/books.h"
#include "PGM-index/include/pgm_index.hpp"

#define BRANCHLESS

using timer = std::chrono::high_resolution_clock;

uint64_t NUM_LOOKUPS = 10000000;
uint64_t MAX_NUM_QUALIFYING = 100;

std::vector<std::string> DATASET_NAMES = {
    "data/books_200M_uint64",
    "data/osm_cellids_200M_uint64",
    "data/wiki_ts_200M_uint64",
    "data/fb_200M_uint64"
};

struct Row {
  uint64_t key;
  uint64_t value;
};

// Adapted from https://github.com/gvinciguerra/rmi_pgm/blob/357acf668c22f927660d6ed11a15408f722ea348/main.cpp#L29.
// Original code authored by Giorgio Vinciguerra.
// Fixed semantics under negative lookups.
template<class ForwardIt>
ForwardIt lower_bound_branchless(ForwardIt first, ForwardIt last, const uint64_t lookup_key) {
  int n = std::distance(first, last);
  int lower = 0;

  while (const int half = n / 2) {
    const int middle = lower + half;

    // Prefetch next possible middles.
    const void* next_middle1 = &*first + lower + half / 2;
    const void* next_middle2 = &*first + middle + half / 2;
    __builtin_prefetch(next_middle1, 0, 0);
    __builtin_prefetch(next_middle2, 0, 0);

    const Row* middle_ptr = &*first + middle;
    lower = (middle_ptr->key <= lookup_key) ? middle : lower;
    n -= half;
  }

  // Scroll back to the first occurrence.
  while (lower > 0 && (&*first + lower - 1)->key == lookup_key) --lower;

  return std::next(first, lower);
}

template<class T>
void do_not_optimize(T const& value) {
  asm volatile("" : : "r,m"(value) : "memory");
}

template<typename T>
static std::vector<T> load_data(const std::string& filename) {
  std::vector<T> data;

  std::ifstream in(filename, std::ios::binary);
  if (!in.is_open()) {
    std::cerr << "unable to open " << filename << std::endl;
    exit(EXIT_FAILURE);
  }

  // Read size.
  uint64_t size;
  in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
  data.resize(size);

  // Read values.
  in.read(reinterpret_cast<char*>(data.data()), size * sizeof(T));
  in.close();

  return data;
}

static std::vector<Row> add_values(const std::vector<uint64_t>& keys) {
  std::vector<Row> result;
  result.reserve(keys.size());
  for (uint64_t i = 0; i < keys.size(); ++i) {
    Row row;
    row.key = keys[i];
    row.value = i;
    result.push_back(row);
  }
  return result;
}

static std::vector<uint64_t> remove_duplicates(const std::vector<uint64_t>& data) {
  std::vector<uint64_t> result = data;
  auto last = std::unique(result.begin(), result.end());
  result.erase(last, result.end());
  return result;
}

std::vector<std::pair<uint64_t, uint64_t>> generate_queries(const std::vector<uint64_t>& keys,
                                                            const std::vector<Row>& dataset) {
  std::vector<std::pair<uint64_t, uint64_t>> results;
  results.reserve(NUM_LOOKUPS);

  // Create set of keys.
  const std::vector<uint64_t> unique_keys = remove_duplicates(keys);

  std::mt19937 g(42);
  std::uniform_int_distribution<size_t> distribution(0, unique_keys.size());

  size_t num_generated = 0;
  while (num_generated < NUM_LOOKUPS) {
    // Draw lookup key from unique keys.
    const size_t idx = distribution(g);
    const uint64_t key = unique_keys[idx];

    // Perform binary search on original keys.
    auto it = std::lower_bound(dataset.begin(),
                               dataset.end(),
                               key,
                               [](const Row& lhs,
                                  const uint64_t lookup_key) {
                                 return lhs.key < lookup_key;
                               });

    // Sum over all values with that key.
    uint64_t result = 0;
    size_t num_qualifying = 0;
    while (it++ != dataset.end() && it->key == key) {
      result += it->value;
      ++num_qualifying;
    }

    if (num_qualifying > MAX_NUM_QUALIFYING) {
      // Too many qualifying entries.
      // Try a different lookup key.
      continue;
    }

    results.push_back(std::make_pair(key, result));
    ++num_generated;
  }

  return results;
}

template<typename F, class V>
size_t query_time(F f, const V& queries) {
  auto start = timer::now();

  uint64_t cnt = 0;
  for (auto& q : queries) {
    cnt += f(q.first, q.second);
  }
  do_not_optimize(cnt);

  auto stop = timer::now();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count() / queries.size();
}

template<int dataset_idx, uint64_t build_time, size_t rmi_size,
    uint64_t (* RMI_FUNC)(uint64_t, size_t*)>
void measure_performance() {
  std::string dataset_name = DATASET_NAMES[dataset_idx];
  //std::cout << "Reading " << dataset_name << std::endl;
  const auto keys = load_data<uint64_t>(dataset_name);

  // Add artificial values to keys.
  const auto dataset = add_values(keys);

  //std::cout << "Generating queries..." << std::endl;
  const std::vector<std::pair<uint64_t, uint64_t>> queries = generate_queries(keys, dataset);

  //std::cout << "Elements \t" << dataset.size() << std::endl;
  //std::cout << "RMI construct. \t" << build_time << " ns" << std::endl;
  //std::cout << "RMI index size \t" << rmi_size << " bytes" << std::endl;

  // Test lookups for RMI.
  auto rmi_ns = query_time([&dataset](const uint64_t lookup_key, const uint64_t expected_result) {
    auto data_size_ = dataset.size();
    size_t error;
    uint64_t guess = RMI_FUNC(lookup_key, &error);
    uint64_t start = (guess < error ? 0 : guess - error);
    uint64_t stop = (guess + error >= data_size_ ? data_size_ : guess + error);

#ifdef BRANCHLESS
    auto lb_result = lower_bound_branchless(dataset.begin() + start, dataset.begin() + stop + 1, lookup_key);
#else
    auto lb_result = std::lower_bound(dataset.begin() + start,
                                      dataset.begin() + stop + 1,
                                      lookup_key,
                                      [](const Row& lhs,
                                         const uint64_t lookup_key) {
                                        return lhs.key < lookup_key;
                                      });
#endif

    // Sum over all values with that key.
    uint64_t result = 0;
    while (lb_result++ != dataset.end() && lb_result->key == lookup_key) result += lb_result->value;

    if (result != expected_result) {
      std::cerr << "RMI returned incorrect result for lookup key " << lookup_key << std::endl;
      std::cerr << "Returned result: " << result << std::endl;
      std::cerr << "Expected result: " << expected_result << std::endl;
      std::cerr << "Start: " << start << " Stop: " << stop << std::endl;
      exit(-1);
    }
    return result;
  }, queries);

  // Test lookups for PGM.
  PGMIndex<uint64_t, 64> index(keys);
  auto pgm_ns = query_time([&index, &dataset](const uint64_t lookup_key, const uint64_t expected_result) {
    auto approx_range = index.find_approximate_position(lookup_key);

#ifdef BRANCHLESS
    auto
        lb_result =
        lower_bound_branchless(dataset.begin() + approx_range.lo, dataset.begin() + approx_range.hi + 1, lookup_key);
#else
    auto lb_result = std::lower_bound(dataset.begin() + approx_range.lo,
                                      dataset.begin() + approx_range.hi + 1,
                                      lookup_key,
                                      [](const Row& lhs,
                                         const uint64_t lookup_key) {
                                        return lhs.key < lookup_key;
                                      });
#endif

    // Sum over all values with that key.
    uint64_t result = 0;
    while (lb_result++ != dataset.end() && lb_result->key == lookup_key) result += lb_result->value;

    if (result != expected_result) {
      std::cerr << "RMI returned incorrect result for lookup key " << lookup_key << std::endl;
      std::cerr << "Returned result: " << result << std::endl;
      std::cerr << "Expected result: " << expected_result << std::endl;
      std::cerr << "Start: " << approx_range.lo << " Stop: " << approx_range.hi << std::endl;
      exit(-1);
    }
    return result;
  }, queries);

  std::cout << dataset_name << ","
            << rmi_ns << ","
            << pgm_ns << std::endl;
}

int main(int argc, char** argv) {
  // load each RMI
  if (wiki::load("rmi_data/")
      && fb::load("rmi_data/")
      && osm::load("rmi_data/")
      && books::load("rmi_data/")) {
    // RMIs loaded.
  } else {
    std::cerr << "Failed to load RMIs" << std::endl;
    exit(-1);
  }
  std::cout << "Dataset,RMI,PGM" << std::endl;
  measure_performance<0, books::RMI_SIZE, books::BUILD_TIME_NS, books::lookup>();
  measure_performance<1, osm::RMI_SIZE, osm::BUILD_TIME_NS, osm::lookup>();
  measure_performance<2, wiki::RMI_SIZE, wiki::BUILD_TIME_NS, wiki::lookup>();
  measure_performance<3, fb::RMI_SIZE, fb::BUILD_TIME_NS, fb::lookup>();

  return 0;
}
