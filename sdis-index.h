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

#ifndef SDIS_INDEX_H
#define SDIS_INDEX_H

#include <iostream>
#include <list>
#include <set>
#include <vector>
#include <unordered_set>
#include "types.h"

namespace sdistream {

class index;
struct index_entry;
struct index_header;

struct index_entry {
  index_header *header = nullptr;
  const index_entry *next = nullptr;
  value_t value = 0;
  index_entry() = default;
  index_entry(index_header *h, const index_entry *n, value_t v) : header(h), next(n), value(v) {
  }
};

bool operator==(const index_entry &, const index_entry &);
bool operator<(const index_entry &, const index_entry &);

struct index_header {
  bool skyline = false;
  stamp_t stamp = 0;
  std::vector<std::pair<index_header *, stamp_t>> tail;
  const index_entry *tuple = nullptr;
  index_header() = default;
  index_header(const index_entry *h, bool k, stamp_t s) : tuple(h), skyline(k), stamp(s) {
  }
  value_t value(size_t n) {
    size_t p = 0;
    const index_entry *e = tuple;
    while (e) {
      if (p++ == n) {
        return e->value;
      }
      e = e->next;
    }
    return 0;
  }
};

bool operator==(const index_header &, const index_header &);
bool operator<(const index_header &, const index_header &);

class index {
public:
  typedef index_entry entry;
  typedef index_header header;
  typedef std::set<index_entry>::iterator iterator;
  typedef std::set<index_entry>::reverse_iterator reverse_iterator;
  static size_t DT;
  static size_t count();
  static bool &skyline(const index::entry *);
  static stamp_t stamp();
  explicit index(size_t);
  index(value_t *, size_t, stamp_t);
  virtual ~index();
  // Put a tuple to index buffer.
  void buffer(value_t *);
  // Compact dimension index.
  void compact();
  // Return all expired tuples.
  std::vector<index::header *> &expired();
  // Return the first stamp.
  index::header *first();
  // Return an dimensional index.
  std::set<index::entry> &get(size_t);
  // Return the last stamp.
  index::header *last();
  // Return the lower bound dimensional index of the buffered tuple.
  size_t lower();
  // Return the lower bound dimensional index of an indexed tuple.
  size_t lower(const index::header *);
  // Return a mute index entry.
  index::entry &mute(value_t);
  // Pre-fetch the next stamp.
  stamp_t next();
  // Pop expired tuples.
  void pop();
  // Put the buffered tuple into the index.
  index::header *put();
  // Put the buffered tuple into the index with skyline flag.
  index::header *put(bool);
  // Put a tuple into the index.
  index::header *put(value_t *);
  // Put a tuple into the index with skyline flag.
  index::header *put(value_t *, bool);
  // Return the number of indexed tuples.
  size_t size();
  // The first skyline tuple dominates the second tuple.
  void tail_append(index::header *, index::header *);
  // Get all dominated tuples of a skyline tuple limited by given stamp.
  std::vector<index::header *> &tail_get(index::header *, stamp_t);
  // Move the first skyline point to the second skyline point.
  void tail_move(index::header *, index::header *);
  // Remove a skyline tuple.
  void tail_remove(index::header *);
  // Return the upper bound dimensional index of the buffered tuple.
  size_t upper();
  // Return the upper bound dimensional index of an indexed tuple.
  size_t upper(const index::header *);
private:
  static double estimate_(const value_t &, const std::set<index::entry> &);
  static size_t count_;
  static size_t zero_;
  void construct_();
  value_t *buffer_ = nullptr;
  index_entry entry_;
  std::vector<index::header *> expired_;
  index::header header_;
  std::list<index::header> headers_;
  std::set<index::entry> *indexes_ = nullptr;
  stamp_t next_ = 0;
  std::vector<index::header *> tail_;
  size_t width_ = 0;
  stamp_t window_ = 0;
};

bool dominate(const index::entry *, const index::entry *);
bool dominate(const index::entry *, const value_t *);
bool dominate(const value_t *, const index::entry *);
bool dominate(const index::header *, const index::header *);
bool dominate(const index::header *, const value_t *);
bool dominate(const value_t *, const index::header *);

}

#endif //SDIS_INDEX_H
