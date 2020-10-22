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

#include "sdis-skyline.h"

namespace sdistream {

size_t skyline::DT = 0;

auto operator<<(std::ostream &out, const skyline &skyline) -> std::ostream & {
  for (auto &&tree : skyline.tree_) {
    for (auto &&s: tree) {
      out << s.first << std::endl;
    }
  }
  return out;
}

auto skyline::add(const index_t &s) -> skyline & {
  auto &&it = tree_[slice_(s)].find(s);
  if (it != tree_[slice_(s)].end()) {
    return *this;
  }
  std::vector<index_t> v;
  tree_[slice_(s)].insert(std::make_pair(s, v));
  ++count_;
  return *this;
}

auto skyline::append(const index_t &s, const index_t &p) -> skyline & {
  auto &&it = tree_[slice_(s)].find(s);
  if (it == tree_[slice_(s)].end()) {
    std::vector<index_t> v;
    v.push_back(p);
    tree_[slice_(s)].insert(std::make_pair(s, v));
    return *this;
  }
  it->second.push_back(p);
  return *this;
}

auto skyline::contains(const index_t &p) -> bool {
  return tree_[slice_(p)].count(p);
}

auto skyline::get(const index_t &s) -> std::vector<index_t> & {
  auto &&it = tree_[slice_(s)].find(s);
  if (it == tree_[slice_(s)].end()) {
    return empty_;
  }
  return it->second;
}

auto skyline::move(const index_t &s1, const index_t &s2) -> skyline & {
  auto &&it1 = tree_[slice_(s1)].find(s1);
  if (it1 == tree_[slice_(s1)].end()) {
    return *this;
  }
  auto &&it2 = tree_[slice_(s2)].find(s2);
  if (it2 == tree_[slice_(s2)].end()) {
    return *this;
  }
  it2->second.push_back(s1);
  it2->second.insert(it2->second.end(), it1->second.begin(), it1->second.end());
  tree_[slice_(s1)].erase(it1);
  --count_;
  return *this;
}

auto skyline::remove(const index_t &s) -> skyline & {
  tree_[slice_(s)].erase(s);
  --count_;
  return *this;
}

auto skyline::size() -> size_t {
  return count_;
}

auto skyline::slice_(index_t index) -> size_t {
  return (int) index % SLICE;
}

}
