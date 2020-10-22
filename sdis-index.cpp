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

#include <sys/time.h>
#include <ctime>
#include <cmath>
#include "sdis-index.h"

namespace sdistream {

size_t index::DT = 0;
size_t index::count_ = 0;
size_t index::zero_ = 0;

bool operator==(const index_entry &e1, const index_entry &e2) {
  return e1.header->stamp == e2.header->stamp;
}

bool operator<(const index_entry &e1, const index_entry &e2) {
  if (e1.value == e2.value) {
    return e1.header->stamp < e2.header->stamp;
  }
  return e1.value < e2.value;
}

bool operator==(const index_header &h1, const index_header &h2) {
  return h1.stamp == h2.stamp;
}

bool operator<(const index_header &h1, const index_header &h2) {
  return h1.stamp < h2.stamp;
}

bool dominate(const index::entry *e1, const index::entry *e2) {
  return dominate(e1->header, e2->header);
}

bool dominate(const index::entry *entry, const value_t *buffer) {
  return dominate(entry->header, buffer);
}

bool dominate(const value_t *buffer, const index::entry *entry) {
  return dominate(buffer, entry->header);
}

bool dominate(const index::header *h1, const index::header *h2) {
  const index::entry *p1 = h1->tuple;
  const index::entry *p2 = h2->tuple;
  ++index::DT;
  bool dominating = false;
  while (p1 && p2) {
    if (p1->value > p2->value) {
      return false;
    } else if (p1->value < p2->value && !dominating) {
      dominating = true;
    }
    p1 = p1->next;
    p2 = p2->next;
  }
  return dominating;
}

bool dominate(const index::header *header, const value_t *buffer) {
  size_t n = 0;
  const index::entry *p = header->tuple;
  ++index::DT;
  bool dominating = false;
  while (p) {
    if (p->value > buffer[n]) {
      return false;
    } else if (p->value < buffer[n] && !dominating) {
      dominating = true;
    }
    ++n;
    p = p->next;
  }
  return dominating;
}

bool dominate(const value_t *buffer, const index::header *header) {
  size_t n = 0;
  const index::entry *p = header->tuple;
  ++index::DT;
  bool dominating = false;
  while (p) {
    if (buffer[n] > p->value) {
      return false;
    } else if (buffer[n] < p->value && !dominating) {
      dominating = true;
    }
    ++n;
    p = p->next;
  }
  return dominating;
}

size_t index::count() {
  return count_;
}

bool &index::skyline(const index::entry *e) {
  return e->header->skyline;
}

stamp_t index::stamp() {
#ifdef WITH_TIME_WINDOW
  struct timeval t{};
  gettimeofday(&t, (struct timezone *) nullptr);
  return t.tv_sec - zero_ + t.tv_usec / 1000000.0;
#else
  return count_;
#endif
}

index::index(size_t width) : width_(width) {
  entry_.header = &header_;
  construct_();
}

index::index(value_t *buffer, size_t width, stamp_t window) : buffer_(buffer), width_(width), window_(window) {
  entry_.header = &header_;
  construct_();
}

index::~index() {
  delete[] indexes_;
}

void index::buffer(value_t *buffer) {
  buffer_ = buffer;
}

void index::compact() {
  if (headers_.back().stamp < window_) {
    return;
  }
  stamp_t stamp = headers_.back().stamp - window_;
  for (auto &&t : headers_.back().tail) {
    auto &&h = t.first;
    if (t.second <= stamp) {
      continue;
    }
    if (h->tuple) {
      auto &&e = h->tuple;
      size_t d = 0;
      while (d < width_ && e) {
        indexes_[d++].erase(*e);
        e = e->next;
      }
      h->tuple = nullptr;
    }
  }
}

std::vector<index::header *> &index::expired() {
  if (headers_.empty()) {
    return expired_;
  }
  expired_.clear();
  stamp_t stamp = headers_.back().stamp - window_;
  auto &&h = headers_.begin();
  while (h != headers_.end() && h->stamp <= stamp) {
    expired_.push_back(&*h);
    ++h;
  }
  return expired_;
}

index::header *index::first() {
  return &headers_.front();
}

std::set<index::entry> &index::get(size_t n) {
  return indexes_[n];
}

index::header *index::last() {
  return &headers_.back();
}

size_t index::lower() {
  size_t d = 0;
  double lower = 1;
  for (size_t i = 0; i < width_; ++i) {
    double e = estimate_(buffer_[i], indexes_[i]);
    if (e == 0) {
      return i;
    } else {
      if (e < lower) {
        lower = e;
        d = i;
      }
    }
  }
  return d;
}

size_t index::lower(const index::header *h) {
  size_t d = 0;
  double lower = 1;
  const index::entry *x = h->tuple;
  for (size_t i = 0; i < width_; ++i) {
    double e = estimate_(x->value, indexes_[i]);
    x = x->next;
    if (e == 0) {
      return i;
    } else {
      if (e < lower) {
        lower = e;
        d = i;
      }
    }
  }
  return d;
}

index::entry &index::mute(value_t value) {
  entry_.value = value;
  header_.stamp = index::stamp() + 1;
  return entry_;
}

stamp_t index::next() {
  next_ = index::stamp();
  return next_;
}

#ifndef WITH_TIME_WINDOW

void index::pop() {
  if (headers_.empty()) {
    return;
  }
  auto &&h = headers_.begin();
  auto &&e = h->tuple;
  size_t d = 0;
  while (d < width_ && e) {
    indexes_[d++].erase(*e);
    e = e->next;
  }
  headers_.erase(h);
}

#else

void index::pop() {
  if (headers_.empty()) {
    return;
  }
  stamp_t stamp = headers_.back().stamp - window_;
  auto &&h = headers_.begin();
  while (h != headers_.end() && h->stamp < stamp) {
    auto &&e = h->tuple;
    size_t d = 0;
    while (d < width_ && e) {
      indexes_[d++].erase(*e);
      e = e->next;
    }
    h = headers_.erase(h);
  }
}

#endif

index::header *index::put() {
  return put(buffer_);
}

index::header *index::put(bool skyline) {
  return put(buffer_, skyline);
}

index::header *index::put(value_t *buffer) {
  return put(buffer, false);
}

index::header *index::put(value_t *buffer, bool skyline) {
  auto stamp = next_ > 0 ? next_ : index::stamp();
  next_ = 0;
  headers_.emplace_back(nullptr, skyline, stamp);
  auto header = &headers_.back();
  const index::entry *next = nullptr;
  size_t n = width_;
  while (n > 0) {
    --n;
    auto &&it = indexes_[n].emplace(header, next, buffer[n]).first;
    next = &(*it);
  }
  header->tuple = next;
  ++count_; // Important!
  return header;
}

size_t index::size() {
  return headers_.size();
}

void index::tail_append(index::header *sky, index::header *tuple) {
  sky->tail.emplace_back(tuple, tuple->stamp);
}

std::vector<index::header *> &index::tail_get(index::header *sky, stamp_t stamp) {
  tail_.clear();
  for (auto &&x : sky->tail) {
    if (x.second > stamp) {
      tail_.push_back(x.first);
    }
  }
  return tail_;
}

void index::tail_move(index::header *tuple, index::header *sky) {
  sky->tail.emplace_back(tuple, tuple->stamp);
  for (auto &&x : tuple->tail) {
    sky->tail.emplace_back(x);
  }
  tuple->tail.clear();
}

void index::tail_remove(index::header *sky) {
  sky->tail.clear();
}

size_t index::upper() {
  size_t d = 0;
  double upper = 0;
  for (size_t i = 0; i < width_; ++i) {
    double e = estimate_(buffer_[i], indexes_[i]);
    if (e == 1) {
      return i;
    } else {
      if (e > upper) {
        upper = e;
        d = i;
      }
    }
  }
  return d;
}

size_t index::upper(const index::header *h) {
  size_t d = 0;
  double upper = 0;
  const index::entry *x = h->tuple;
  for (size_t i = 0; i < width_; ++i) {
    double e = estimate_(x->value, indexes_[i]);
    x = x->next;
    if (e == 1) {
      return i;
    } else {
      if (e > upper) {
        upper = e;
        d = i;
      }
    }
  }
  return d;
}

double index::estimate_(const value_t &v, const std::set<index::entry> &d) {
  if (d.empty()) {
    return 0;
  }
  auto &&first = d.begin()->value;
  auto &&last = d.rbegin()->value;
  if (first == last) {
    return 1;
  }
  if (v < first) {
    return 0;
  }
  if (last < v) {
    return 1;
  }
  return 1.0 * std::abs(v - first) / std::abs(last - first);
}

void index::construct_() {
  indexes_ = new std::set<index::entry>[width_];
  struct timeval t{};
  gettimeofday(&t, (struct timezone *) nullptr);
  zero_ = t.tv_sec;
}

}
