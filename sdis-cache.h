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

#ifndef SDIS_CACHE_H
#define SDIS_CACHE_H

#ifndef CACHE
#define CACHE 1000000
#endif

#ifndef BLOCK
#define BLOCK 16
#endif

#include <iostream>
#include <set>
#include "types.h"

namespace sdistream {

struct cache_entry {
  index_t index = 0;
  value_t value = 0;
  explicit cache_entry(index_t);
  cache_entry(index_t, value_t);
  inline auto set(index_t n) -> cache_entry & {
    index = n;
    return *this;
  }
  cache_entry() = default;
};

auto operator==(const cache_entry &, const cache_entry &) -> bool;
auto operator<(const cache_entry &, const cache_entry &) -> bool;
auto operator<<(std::ostream &, const cache_entry &) -> std::ostream &;

auto estimate(const cache_entry &, const std::set<cache_entry> &) -> double;
auto lower_dimension(const cache_entry *, const std::set<cache_entry> *, size_t) -> size_t;
auto upper_dimension(const cache_entry *, const std::set<cache_entry> *, size_t) -> size_t;

}

#ifndef WITH_TIME_WINDOW

#include "types.h"

namespace sdistream {

class cache {
public:
  cache() = default;
  cache(size_t, size_t);
  virtual ~cache();
  auto get(index_t) -> value_t *;
  auto put(value_t *) -> value_t *;
  auto put(value_t *, bool) -> value_t *;
  auto skyline(index_t) -> bool &;
private:
  value_t *cache_ = nullptr;
  size_t count_ = 0;
  bool *skyline_ = nullptr;
  size_t width_ = 0;
  size_t window_ = 0;
};

}

#else

#include <array>
#include <list>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "types.h"

namespace sdistream {

class cache {
public:
  static auto timestamp() -> double;
  cache() = default;
  cache(size_t, size_t);
  virtual ~cache();
  void clean();
  auto contains(double) -> bool;
  auto expired() -> std::vector<double> &;
  auto get(double) -> value_t *;
  auto put(value_t *) -> double;
  auto put(value_t *, bool) -> double;;
  auto size() -> size_t;
private:
  static auto slice_(double) -> size_t;
  static size_t zero_;
  value_t *cache_ = nullptr;
  size_t count_ = 0;
  std::vector<double> expired_;
  std::vector<value_t *> free_;
  std::array<std::unordered_map<double, value_t *>, BLOCK> index_;
  double latest_ = 0;
  std::list<value_t *> list_;
  size_t skyline_ = 0;
  size_t stamp_ = 0;
  size_t width_ = 0;
  size_t width2_ = 0;
  double window_ = 0;
};

}

#endif

#endif //SDIS_CACHE_H
