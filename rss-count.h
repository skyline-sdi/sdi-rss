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

#ifndef SDIS_RSS_COUNT_H
#define SDIS_RSS_COUNT_H

#ifndef POST_WINDOW_COUNT
#define POST_WINDOW_COUNT 2000
#endif

#include <unordered_set>
#include "sdis-cache.h"
#include "sdis-skyline.h"
#include "sdis-stream.h"
#include "timer.h"
#include "types.h"

namespace sdistream {

static skyline skyline;

template<class IN>
void skyline_update(IN &in, size_t width, size_t window) {
  cache cache(width, window); // Tuple cache.
  size_t count = 0;
  std::unordered_set<index_t> deal;
  auto entries = new cache_entry[width]; // Index entry buffer.
  auto entries_remove = new cache_entry[width]; // Index entry of the tuple to remove.
  auto entries_update = new cache_entry[width]; // Index entry of the non-skyline tuple to update while removing a tuple.
  index_t index = 0; // Index ID of the incoming tuple.
  auto indexes = new std::set<cache_entry>[width]; // Dimensional indexes.
  auto tuple = new value_t[width]; // Tuple input buffer.
  timer t; // Timer for performance evaluation.
  // Add the first tuple.
  if (!input(in, width, tuple)) {
    delete[] tuple;
    return;
  }
  t.start();
  for (size_t i = 0; i < width; ++i) {
    entries[i].index = index;
    entries[i].value = tuple[i];
    indexes[i].insert(entries[i]);
  }
  cache.put(tuple, true);
  skyline.add(index);
  ++index;
  t.stop();
  std::cout << "# " << index << " + " << t.runtime() << " " << skyline.size() << " " << count << std::endl;
  // Process incoming tuples.
  while (input(in, width, tuple)) {
    if (count >= POST_WINDOW_COUNT) {
      break;
    }
    t.start();
    for (size_t i = 0; i < width; ++i) {
      entries[i].index = index;
      entries[i].value = tuple[i];
    }
    // Remove the expired tuple.
    if (index >= window) {
      ++count;
      // Build index entry of the tuple to remove.
      auto &&index_remove = index - window;
      auto &&tuple_remove = cache.get(index_remove);
      for (size_t i = 0; i < width; ++i) {
        entries_remove[i].index = index_remove;
        entries_remove[i].value = tuple_remove[i];
      }
      // The expired tuple is a skyline tuple.
      if (cache.skyline(index_remove)) {
        deal.clear();
        for (auto &&index_update : skyline.get(index_remove)) {
          // Ignore tuples that have already been removed.
          if (index_update < index_remove) {
            continue;
          }
          deal.insert(index_update);
          auto &&tuple_update = cache.get(index_update); // Green warm.
          for (size_t i = 0; i < width; ++i) {
            entries_update[i].index = index_update;
            entries_update[i].value = tuple_update[i];
          }
          auto &&lower_bound_dimension = lower_dimension(entries_update, indexes, width);
          auto &&lower_bound_entry = entries_update[lower_bound_dimension];
          auto &&lower_bound_index = indexes[lower_bound_dimension];
          auto &&lower = lower_bound_index.begin();
          bool dominated = false;
          while (lower->value <= lower_bound_entry.value && lower != lower_bound_index.end()) {
            // If the lower tuple is not in skyline set or is the expired tuple,
            // ignore it.
            if (!cache.skyline(lower->index) || lower->index == index_remove) {
              ++lower;
              continue;
            }
            // If current tuple is dominated ALSO by the lower tuple, do break.
            if (dominate<value_t>(cache.get(lower->index), tuple_update, width)) {
              skyline.append(lower->index, index_update);
              dominated = true;
              break;
            }
            // If current tuple is not dominated by the lower tuple, even the
            // lower tuple has the same value as the current tuple, the reverse
            // dominance checking is not necessary. Just comment the following
            // useless code.
            //if (lower->value == lower_bound_entry.value && dominate<value_t>(tuple, cache.get(lower->index), width)) {
            //  skyline.move(lower->index, index);
            //}
            ++lower;
          }
          if (!dominated) {
            cache.skyline(index_update) = true;
            skyline.add(index_update);
          }
          // Dominance tree entries do not respect dimensional indexing order,
          // a local BNL must be applied to fix this problem.
          for (auto &&x : deal) {
            if (x != index_update && cache.skyline(x)) {
              if (dominate<value_t>(tuple_update, cache.get(x), width)) {
                cache.skyline(x) = false;
                skyline.move(x, index_update);
              }
            }
          }
        }
        cache.skyline(index_remove) = false; // Not really necessary.
        skyline.remove(index_remove);
      }
      // Remove expired tuple from all dimensional indexes.
      for (size_t i = 0; i < width; ++i) {
        indexes[i].erase(entries_remove[i]);
      }
    }
    // Do lower-bound dominance checking.
    bool dominated = false;
    auto &&lower_bound_dimension = lower_dimension(entries, indexes, width);
    auto &&lower_bound_entry = entries[lower_bound_dimension];
    auto &&lower_bound_index = indexes[lower_bound_dimension];
    auto &&lower = lower_bound_index.begin();
    while (lower->value <= lower_bound_entry.value && lower != lower_bound_index.end()) {
      // Only compare the incoming tuple with skyline tuples.
      if (!cache.skyline(lower->index)) {
        ++lower;
        continue;
      }
      // If the incoming tuple is dominated by a lower skyline tuple, do break.
      // The skyline flag of the incoming tuple will be set while adding it
      // to the cache.
      if (dominate<value_t>(cache.get(lower->index), tuple, width)) {
        dominated = true;
        skyline.append(lower->index, index);
        break;
      }
      // If the incoming tuple is not dominated by the lower tuple, however the
      // lower tuple has the same value as the current tuple, then do reverse
      // dominance checking.
      if (lower->value == lower_bound_entry.value && dominate<value_t>(tuple, cache.get(lower->index), width)) {
        cache.skyline(lower->index) = false;
        skyline.move(lower->index, index);
      }
      ++lower;
    }
    // Do upper-bound dominance checking.
    if (!dominated) {
      skyline.add(index);
      auto &&upper_bound_dimension = upper_dimension(entries, indexes, width);
      auto &&upper_bound_entry = entries[upper_bound_dimension];
      auto &&upper_bound_index = indexes[upper_bound_dimension];
      auto &&upper_repeat = std::set<cache_entry>::reverse_iterator(upper_bound_index.lower_bound(upper_bound_entry));
      // For repeating dimensional values.
      while (upper_repeat != upper_bound_index.rend()) {
        if (!cache.skyline(upper_repeat->index)) {
          ++upper_repeat;
          continue;
        }
        if (upper_repeat->value < upper_bound_entry.value) {
          break;
        }
        // A tuple with repeat dimensional value is dominated by the incoming
        // tuple.
        if (dominate<value_t>(tuple, cache.get(upper_repeat->index), width)) {
          cache.skyline(upper_repeat->index) = false;
          skyline.move(upper_repeat->index, index);
        }
        ++upper_repeat;
      }
      // Find all upper skyline tuples that are dominated by the
      // incoming tuple.
      auto &&upper = upper_bound_index.upper_bound(upper_bound_entry);
      while (upper != upper_bound_index.end()) {
        if (!skyline.contains(upper->index)) {
          //if (!cache.skyline(upper->index)) {
          ++upper;
          continue;
        }
        if (dominate<value_t>(tuple, cache.get(upper->index), width)) {
          cache.skyline(upper->index) = false;
          skyline.move(upper->index, index);
        }
        ++upper;
      }
    }
    // Add the incoming tuple to all dimensional indexes.
    for (size_t i = 0; i < width; ++i) {
      indexes[i].insert(entries[i]);
    }
    // Finally, replace the expired tuple by the incoming tuple.
    cache.put(tuple, !dominated);
    ++index;
    t.stop();
    if (index >= window) {
      std::cout << index << (dominated ? " - " : " + ") << t.runtime() << " " << skyline.size() << " " << count
                << std::endl;
    } else {
      std::cout << "# " << index << (dominated ? " - " : " + ") << t.runtime() << " " << skyline.size() << " " << count
                << std::endl;
    }
  }
  delete[] entries;
  delete[] entries_remove;
  delete[] entries_update;
  delete[] indexes;
  delete[] tuple;
  std::cout << "# Mean processing time: " << (count ? t.total() / count : 0) << " sec/tuple" << std::endl;
}

}

#endif //SDIS_RSS_COUNT_H
