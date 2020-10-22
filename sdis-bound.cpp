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
#include "sdis-bound.h"

namespace sdistream {

auto estimate(const entry &ent, const std::set<entry> &dim) -> double {
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

auto lower_dimension(const entry *entries, const std::set<entry> *indexes, size_t width) -> size_t {
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

auto upper_dimension(const entry *entries, const std::set<entry> *indexes, size_t width) -> size_t {
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
