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

#ifndef SDIS_SKYLINE_H
#define SDIS_SKYLINE_H

#ifndef SLICE
#define SLICE 32
#endif

#include <array>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "types.h"

namespace sdistream {

class skyline {
  friend auto operator<<(std::ostream &, const skyline &) -> std::ostream &;
public:
  static size_t DT;
  skyline() = default;
  virtual ~skyline() = default;
  // Add a skyline point to d-tree.
  auto add(const index_t &) -> skyline &;
  // The first skyline point dominates the second point.
  auto append(const index_t &, const index_t &) -> skyline &;
  // Check whether a point is skyline point.
  auto contains(const index_t &) -> bool;
  // Get all dominated points of a skyline point.
  auto get(const index_t &) -> std::vector<index_t> &;
  // Move the first skyline point to the second skyline point.
  auto move(const index_t &, const index_t &) -> skyline &;
  // Remove a skyline point.
  auto remove(const index_t &) -> skyline &;
  // The number of skyline points.
  auto size() -> size_t;
private:
  static auto slice_(index_t) -> size_t;
  size_t count_ = 0;
  std::vector<index_t> empty_;
  std::array<std::unordered_map<index_t, std::vector<index_t>>, SLICE> tree_;
};

template<class V>
auto dominate(V *row1, V *row2, size_t width) -> bool {
  ++skyline::DT;
  V *p1 = row1;
  V *p2 = row2;
  bool dominating = false;
  for (size_t i = 0; i < width; ++i, ++p1, ++p2) {
    if (*p1 > *p2) {
      return false;
    } else if (*p1 < *p2 && !dominating) {
      dominating = true;
    }
  }
  return dominating;
}

}

#endif //SDIS_SKYLINE_H
