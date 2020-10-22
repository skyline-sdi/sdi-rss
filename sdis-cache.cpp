/*-
 * Copyright (c) 2019 Rui Liu and Dominique Li <dominique.li@univ-tours.fr>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: log.h 998 2014-12-18 12:07:14Z li $
 */

#include <cmath>
#include "sdis-cache.h"

namespace sdistream {

cache_entry::cache_entry(index_t index) : index(index) {
}

cache_entry::cache_entry(index_t index, value_t value) : index(index), value(value) {
}

auto operator==(const cache_entry &row1, const cache_entry &row2) -> bool {
  return row1.index == row2.index;
}

auto operator<(const cache_entry &row1, const cache_entry &row2) -> bool {
  if (row1.value == row2.value) {
    return row1.index < row2.index;
  }
  return row1.value < row2.value;
}

auto operator<<(std::ostream &out, const cache_entry &row) -> std::ostream & {
  out << row.value << ":" << row.index;
  return out;
}

auto estimate(const cache_entry &ent, const std::set<cache_entry> &dim) -> double {
  if (dim.empty()) {
    return 0;
  }
  auto &&first = *(dim.begin());
  auto &&last = *(dim.rbegin());
  if (first == last) {
    return 1;
  }
  if (ent < first) {
    return 0;
  }
  if (last < ent) {
    return 1;
  }
  return 1.0 * std::abs(ent.value - first.value) / std::abs(last.value - first.value);
}

auto lower_dimension(const cache_entry *entries, const std::set<cache_entry> *indexes, size_t width) -> size_t {
  size_t d = 0;
  double lower = 1;
  for (size_t i = 0; i < width; ++i) {
    double est = estimate(entries[i], indexes[i]);
    if (est == 0) {
      return i;
    } else {
      if (est < lower) {
        lower = est;
        d = i;
      }
    }
  }
  return d;
}

auto upper_dimension(const cache_entry *entries, const std::set<cache_entry> *indexes, size_t width) -> size_t {
  size_t d = 0;
  double upper = 0;
  for (size_t i = 0; i < width; ++i) {
    double est = estimate(entries[i], indexes[i]);
    if (est == 1) {
      return i;
    } else {
      if (est > upper) {
        upper = est;
        d = i;
      }
    }
  }
  return d;
}

}

#ifndef WITH_TIME_WINDOW

#include <cstring>
#include "sdis-cache.h"

namespace sdistream {

cache::cache(size_t width, size_t window) : count_(0), width_(width), window_(window) {
  cache_ = new value_t[width_ * window_];
  skyline_ = new bool[window_];
}

cache::~cache() {
  delete[] cache_;
  delete[] skyline_;
}

auto cache::get(index_t index) -> value_t * {
  if (index > count_) {
    return nullptr;
  }
  return &cache_[(index % window_) * width_];
}

auto cache::put(value_t *buffer) -> value_t * {
  size_t index = count_ % window_;
  value_t *base = &cache_[index * width_];
  std::memcpy(base, buffer, sizeof(value_t) * width_);
  skyline_[index] = false;
  ++count_;
  return base;
}

auto cache::put(value_t *buffer, bool skyline) -> value_t * {
  size_t index = count_ % window_;
  value_t *base = &cache_[index * width_];
  std::memcpy(base, buffer, sizeof(value_t) * width_);
  skyline_[index] = skyline;
  ++count_;
  return base;
}

auto cache::skyline(index_t index) -> bool & {
  return skyline_[index % window_];
}

}

#else

#include <sys/time.h>
#include <ctime>
#include <cstring>
#include <iostream>
#include "sdis-cache.h"

namespace sdistream {

size_t cache::zero_ = 0;

auto cache::timestamp() -> double {
  struct timeval t{};
  gettimeofday(&t, (struct timezone *) nullptr);
  return (double) (t.tv_sec - zero_) + t.tv_usec / 1000000.0;
}

cache::cache(size_t width, size_t window) : free_(CACHE), skyline_(width), stamp_(width + 1), width_(width), width2_(width + 2), window_(window) {
  cache_ = new value_t[width2_ * CACHE]; // Tuple + Skyline flag (-1/1) + Timestamp
  for (size_t i = 0; i < CACHE; ++i) {
    free_[i] = &cache_[width2_ * i];
  }
  struct timeval t{};
  gettimeofday(&t, (struct timezone *) nullptr);
  zero_ = t.tv_sec;
}

cache::~cache() {
  delete[] cache_;
}

void cache::clean() {
  for (auto &&it = list_.begin(); it != list_.end();) {
    auto &&early = (*it)[stamp_];
    if (latest_ - early > window_) {
      free_.push_back(*it);
      index_[slice_(early)].erase(early);
      it = list_.erase(it);
    } else {
      break;
    }
  }
}

auto cache::contains(double stamp) -> bool {
  return index_[slice_(stamp)].count(stamp);
}

auto cache::expired() -> std::vector<double> & {
  expired_.clear();
  for (auto &&it = list_.begin(); it != list_.end(); ++it) {
    auto &&early = (*it)[stamp_];
    if (latest_ - early > window_) {
      expired_.push_back(early);
    } else {
      break;
    }
  }
  return expired_;
}

auto cache::get(double stamp) -> value_t * {
  auto &&it = index_[slice_(stamp)].find(stamp);
  if (it == index_[slice_(stamp)].end()) {
    return nullptr;
  }
  return it->second;
}

auto cache::put(value_t *buffer) -> double {
  latest_ = timestamp();
  auto &&row = free_.back();
  std::memcpy(row, buffer, sizeof(value_t) * width_);
  free_.pop_back();
  index_[slice_(latest_)].insert(std::make_pair(latest_, row));
  list_.push_back(row);
  row[stamp_] = latest_;
  ++count_;
  return latest_;
}

auto cache::put(value_t *buffer, bool skyline) -> double {
  latest_ = timestamp();
  auto &&row = free_.back();
  std::memcpy(row, buffer, sizeof(value_t) * width_);
  free_.pop_back();
  index_[slice_(latest_)].insert(std::make_pair(latest_, row));
  list_.push_back(row);
  row[stamp_] = latest_;
  ++count_;
  return latest_;
}

auto cache::size() -> size_t {
  return CACHE - free_.size();
}

auto cache::slice_(double index) -> size_t {
  return (int) index % BLOCK;
}

}

#endif
