/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <iostream>
#include <tuple>

// this is needed for a test below, but should be defined here
std::ostream& operator<<(std::ostream& os, const std::tuple<double, double, double>& tuple) {
  os << std::get<0>(tuple) << ", " << std::get<1>(tuple) << ", " << std::get<2>(tuple);
  return os;
}

#include <catch.hpp>
#include <tuple_sketch.hpp>
#include <test_type.hpp>

namespace datasketches {

TEST_CASE("tuple sketch float: builder", "[tuple_sketch]") {
  auto builder = update_tuple_sketch<float>::builder();
  builder.set_lg_k(10).set_p(0.5).set_resize_factor(theta_constants::resize_factor::X2).set_seed(123);
  auto sketch = builder.build();
  REQUIRE(sketch.get_lg_k() == 10);
  REQUIRE(sketch.get_theta() == 0.5);
  REQUIRE(sketch.get_rf() == theta_constants::resize_factor::X2);
  REQUIRE(sketch.get_seed_hash() == compute_seed_hash(123));
}

TEST_CASE("tuple sketch float: empty", "[tuple_sketch]") {
  auto update_sketch = update_tuple_sketch<float>::builder().build();
  REQUIRE(update_sketch.is_empty());
  REQUIRE(!update_sketch.is_estimation_mode());
  REQUIRE(update_sketch.get_estimate() == 0);
  REQUIRE(update_sketch.get_lower_bound(1) == 0);
  REQUIRE(update_sketch.get_upper_bound(1) == 0);
  REQUIRE(update_sketch.get_theta() == 1);
  REQUIRE(update_sketch.get_num_retained() == 0);
  REQUIRE(!update_sketch.is_ordered());

  auto compact_sketch = update_sketch.compact();
  REQUIRE(compact_sketch.is_empty());
  REQUIRE(!compact_sketch.is_estimation_mode());
  REQUIRE(compact_sketch.get_estimate() == 0);
  REQUIRE(compact_sketch.get_lower_bound(1) == 0);
  REQUIRE(compact_sketch.get_upper_bound(1) == 0);
  REQUIRE(compact_sketch.get_theta() == 1);
  REQUIRE(compact_sketch.get_num_retained() == 0);
  REQUIRE(compact_sketch.is_ordered());
}

TEST_CASE("tuple sketch float: exact mode", "[tuple_sketch]") {
  auto update_sketch = update_tuple_sketch<float>::builder().build();
  update_sketch.update(1, 1);
  update_sketch.update(2, 2);
  update_sketch.update(1, 1);
  std::cout << update_sketch.to_string(true);
  REQUIRE(!update_sketch.is_empty());
  REQUIRE(!update_sketch.is_estimation_mode());
  REQUIRE(update_sketch.get_estimate() == 2);
  REQUIRE(update_sketch.get_lower_bound(1) == 2);
  REQUIRE(update_sketch.get_upper_bound(1) == 2);
  REQUIRE(update_sketch.get_theta() == 1);
  REQUIRE(update_sketch.get_num_retained() == 2);
  REQUIRE(!update_sketch.is_ordered());
  int count = 0;
  for (const auto& entry: update_sketch) {
    REQUIRE(entry.second == 2);
    ++count;
  }
  REQUIRE(count == 2);

  auto compact_sketch = update_sketch.compact();
  std::cout << compact_sketch.to_string(true);
  REQUIRE(!compact_sketch.is_empty());
  REQUIRE(!compact_sketch.is_estimation_mode());
  REQUIRE(compact_sketch.get_estimate() == 2);
  REQUIRE(compact_sketch.get_lower_bound(1) == 2);
  REQUIRE(compact_sketch.get_upper_bound(1) == 2);
  REQUIRE(compact_sketch.get_theta() == 1);
  REQUIRE(compact_sketch.get_num_retained() == 2);
  REQUIRE(compact_sketch.is_ordered());
  count = 0;
  for (const auto& entry: compact_sketch) {
    REQUIRE(entry.second == 2);
    ++count;
  }
  REQUIRE(count == 2);
}

template<typename T>
class max_value_policy {
public:
  max_value_policy(const T& initial_value): initial_value(initial_value) {}
  T create() const { return initial_value; }
  void update(T& summary, const T& update) const { summary = std::max(summary, update); }
private:
  T initial_value;
};

typedef update_tuple_sketch<float, float, max_value_policy<float>> max_float_update_tuple_sketch;

TEST_CASE("tuple sketch: float, custom policy", "[tuple_sketch]") {
  auto update_sketch = max_float_update_tuple_sketch::builder(max_value_policy<float>(5)).build();
  update_sketch.update(1, 1);
  update_sketch.update(1, 2);
  update_sketch.update(2, 10);
  update_sketch.update(3, 3);
  update_sketch.update(3, 7);
  std::cout << update_sketch.to_string(true);
  int count = 0;
  float sum = 0;
  for (const auto& entry: update_sketch) {
    sum += entry.second;
    ++count;
  }
  REQUIRE(count == 3);
  REQUIRE(sum == 22); // 5 + 10 + 7
}

struct test_type_replace_policy {
  test_type create() const { return test_type(0); }
  void update(test_type& summary, const test_type& update) const {
    //std::cerr << "policy::update lvalue begin" << std::endl;
    summary = update;
    //std::cerr << "policy::update lvalue end" << std::endl;
  }
  void update(test_type& summary, test_type&& update) const {
    //std::cerr << "policy::update rvalue begin" << std::endl;
    summary = std::move(update);
    //std::cerr << "policy::update rvalue end" << std::endl;
  }
};

TEST_CASE("tuple sketch: test type with replace policy", "[tuple_sketch]") {
  auto sketch = update_tuple_sketch<test_type, test_type, test_type_replace_policy, test_type_serde>::builder().build();
  test_type a(1);
  sketch.update(1, a); // this should copy
  sketch.update(2, 2); // this should move
  sketch.update(1, 2); // this should move
  std::cout << sketch.to_string(true);
  REQUIRE(sketch.get_num_retained() == 2);
  for (const auto& entry: sketch) {
    REQUIRE(entry.second.get_value() == 2);
  }
}

struct three_doubles_update_policy {
  std::tuple<double, double, double> create() const {
    return std::tuple<double, double, double>(0, 0, 0);
  }
  void update(std::tuple<double, double, double>& summary, const std::tuple<double, double, double>& update) const {
    std::get<0>(summary) += std::get<0>(update);
    std::get<1>(summary) += std::get<1>(update);
    std::get<2>(summary) += std::get<2>(update);
  }
};

TEST_CASE("tuple sketch: array of doubles", "[tuple_sketch]") {
  using three_doubles = std::tuple<double, double, double>;
  using three_doubles_update_tuple_sketch = update_tuple_sketch<three_doubles, three_doubles, three_doubles_update_policy>;
  auto update_sketch = three_doubles_update_tuple_sketch::builder().build();
  update_sketch.update(1, three_doubles(1, 2, 3));
  std::cout << update_sketch.to_string(true);
  const auto& entry = *update_sketch.begin();
  REQUIRE(std::get<0>(entry.second) == 1.0);
  REQUIRE(std::get<1>(entry.second) == 2.0);
  REQUIRE(std::get<2>(entry.second) == 3.0);

  auto compact_sketch = update_sketch.compact();
  std::cout << compact_sketch.to_string(true);
}

} /* namespace datasketches */