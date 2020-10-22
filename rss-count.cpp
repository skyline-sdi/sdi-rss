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

#include <fstream>
#include <iostream>
#include "rss-count.h"
#include "timer.h"
using namespace sdistream;

auto run_skyline(const char *name, size_t dimensionality, size_t window, const char *stream) -> bool {
  timer t;
  std::cerr << "Running..." << std::endl;
  if (stream) {
    std::ifstream in(stream);
    if (!in.good()) {
      std::cerr << "Cannot open stream " << stream << std::endl;
      return false;
    }
    t.start();
    skyline_update<std::ifstream>(in, dimensionality, window);
    t.stop();
    in.close();
  } else {
    t.start();
    skyline_update<std::istream>(std::cin, dimensionality, window);
    t.stop();
  }
  return true;
}

auto main(int argc, char **argv) -> int {
  if (argc < 3) {
    std::cout << "Usage: rss-count DIMENSIONALITY WINDOW [STREAM]" << std::endl;
    return 0;
  }
  size_t dimensionality = strtoul(argv[1], nullptr, 10);
  size_t window = strtoul(argv[2], nullptr, 10);
  const char *stream = argc > 3 ? argv[3] : nullptr;
  run_skyline("RSS-COUNT", dimensionality, window, stream);
  return 0;
}
