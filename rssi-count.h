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
#include "sdis-index.h"
#include "sdis-stream.h"
#include "timer.h"
#include "types.h"

namespace sdistream {

static std::unordered_set<index::header *> skyline;

template<class IN>
void skyline_update(IN &in, size_t width, size_t window) {
  auto buffer = new value_t[width]; // Tuple input buffer.
  size_t count = 0;
  std::unordered_set<index::header *> deal;
  class index index(buffer, width, window); // Dimensional indexes.
  stamp_t stamp; // Current tuple stamp.
  index::header *header; // Current tuple herder.
  timer t; // Timer for performance evaluation.
  // Process the first incoming tuple.
  if (!input(in, width, buffer)) {
    delete[] buffer;
    return;
  }
  t.start();
  header = index.put(true);
  skyline.insert(header);
  t.stop();
  std::cout << "# " << header->stamp << " + " << t.runtime() << " " << skyline.size() << " " << index.size() << " "
            << count << std::endl;
  // Process incoming tuples.
  while (input(in, width, buffer)) {
    if (count >= POST_WINDOW_COUNT) {
      break;
    }
    // The buffered tuple is automatically associated with dimensional indexes.
    t.start();
    // Get the next stamp.
    stamp = index.next();
    // Remove the expired tuple.
    if (stamp >= window) {
      // Build index entry of the tuple to remove.
      auto &&remove = index.first();
      ///std::cout << remove->stamp << (remove->skyline ? " + " : " - ") << " " << remove->tail.size() << std::endl;
      // The expired tuple is a skyline tuple.
      if (remove->skyline) {
        deal.clear();
        for (auto &&update : index.tail_get(remove, remove->stamp)) {
          deal.insert(update);
          auto &&update_tuple = update->tuple;
          auto &&lower_dimension = index.lower(update);
          auto &&lower_index = index.get(lower_dimension);
          auto &&lower_iterator = lower_index.begin();
          bool dominated = false;
          while (lower_iterator != lower_index.end()
              && lower_iterator->header->value(lower_dimension) < update->value(lower_dimension)) {
            auto &&lower = lower_iterator->header;
            auto &&lower_tuple = lower->tuple;
            // If the lower tuple is not in skyline set or is the expired tuple,
            // ignore it.
            if (!lower->skyline || lower->stamp == remove->stamp) {
              ++lower_iterator;
              continue;
            }
            // If current tuple is dominated ALSO by the lower tuple, do break.
            if (dominate(lower_tuple, update_tuple)) {
              index.tail_append(lower, update);
              dominated = true;
              break;
            }
            ++lower_iterator;
          }
          if (!dominated) {
            update->skyline = true;
            skyline.insert(update);
          }
          // Dominance tree entries do not respect dimensional indexing order,
          // a local BNL must be applied to fix this problem.
          for (auto &&x : deal) {
            if (x != update && x->skyline) {
              if (dominate(update_tuple, x->tuple)) {
                x->skyline = false;
                index.tail_move(x, update);
                skyline.erase(x);
              }
            }
          }
        }
        skyline.erase(remove);
      }
      index.pop();
    }
    // Put buffered incoming tuple to index.
    header = index.put();
    // Do lower-bound dominance checking.
    bool dominated = false;
    auto &&lower_dimension = index.lower();
    auto &&lower_index = index.get(lower_dimension);
    auto &&lower_iterator = lower_index.begin();
    while (lower_iterator != lower_index.end() && lower_iterator->value <= buffer[lower_dimension]) {
      auto &&lower = lower_iterator->header;
      auto &&lower_tuple = lower->tuple;
      // Only compare the incoming tuple with skyline tuples.
      if (!lower->skyline) {
        ++lower_iterator;
        continue;
      }
      // If the incoming tuple is dominated by a lower skyline tuple, do break.
      // The skyline flag of the incoming tuple will be set while adding it
      // to the cache.
      if (dominate(lower_tuple, buffer)) {
        dominated = true;
        index.tail_append(lower, header);
        break;
      }
      // If the incoming tuple is not dominated by the lower tuple, however the
      // lower tuple has the same value as the current tuple, then do reverse
      // dominance checking.
      if (lower_iterator->value == buffer[lower_dimension] && dominate(buffer, lower_tuple)) {
        lower->skyline = false;
        index.tail_move(lower, header);
        skyline.insert(header);
        skyline.erase(lower);
      }
      ++lower_iterator;
    }
    // Do upper-bound dominance checking.
    if (!dominated) {
      skyline.insert(header);
      header->skyline = true;
      auto &&upper_dimension = index.upper();
      auto &&upper_index = index.get(upper_dimension);
      auto &&upper_entry = index.mute(buffer[upper_dimension]);
      auto &&upper_repeat_iterator = std::set<index::entry>::reverse_iterator(upper_index.lower_bound(upper_entry));
      // For repeating dimensional values.
      while (upper_repeat_iterator != upper_index.rend()) {
        auto &&upper_repeat = upper_repeat_iterator->header;
        auto &&upper_repeat_tuple = upper_repeat->tuple;
        if (!upper_repeat_tuple) {
          continue;
        }
        if (!index::skyline(upper_repeat_tuple)) {
          ++upper_repeat_iterator;
          continue;
        }
        if (upper_repeat_iterator->value < buffer[upper_dimension]) {
          break;
        }
        // A tuple with repeat dimensional value is dominated by the incoming
        // tuple.
        if (dominate(buffer, upper_repeat_tuple)) {
          index::skyline(upper_repeat_tuple) = false;
          index.tail_move(upper_repeat, header);
          skyline.erase(upper_repeat);
        }
        ++upper_repeat_iterator;
      }
      // Find all upper skyline tuples that are dominated by the
      // incoming tuple.
      auto &&upper_iterator = upper_index.upper_bound(upper_entry);
      while (upper_iterator != upper_index.end()) {
        auto &&upper = upper_iterator->header;
        auto &&upper_tuple = upper->tuple;
        if (!upper_tuple) {
          continue;
        }
        if (!skyline.count(upper)) {
          ++upper_iterator;
          continue;
        }
        if (dominate(buffer, upper_tuple)) {
          upper->skyline = false;
          index.tail_move(upper, header);
          skyline.erase(upper);
        }
        ++upper_iterator;
      }
      index.compact();
    } else {
      skyline.erase(header);
    }
    t.stop();
    if (header->stamp >= window) {
      ++count;
      std::cout << header->stamp << (dominated ? " - " : " + ") << t.runtime() << " " << skyline.size() << " "
                << index.size() << " " << count << std::endl;
    } else {
      std::cout << "# " << header->stamp << (dominated ? " - " : " + ") << t.runtime() << " " << skyline.size() << " "
                << index.size() << " " << count << std::endl;
    }
  }
  delete[] buffer;
  std::cout << "# Mean processing time: " << (count ? t.total() / count : 0) << " sec/tuple" << std::endl;
}

}

#endif //SDIS_RSS_COUNT_H
