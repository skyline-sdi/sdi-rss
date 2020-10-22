/*-
 * Copyright (c) 2007-2012 Dominique Li <dominique.li@univ-tours.fr>
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
 * $Id: timer.h 1115 2016-12-27 20:57:29Z li $
 */

#include <ctime>
#include <sys/time.h>
#include "timer.h"

auto timer::microtime() -> double {
  struct timeval t{};
  gettimeofday(&t, (struct timezone *) nullptr);
  return t.tv_sec + t.tv_usec / 1000000.0;
}

auto timer::now() -> time_t {
  return time(nullptr);
}

auto timer::reset() -> double {
  stop_ = 0;
  total_ = 0;
  return start_ = clock();
}

auto timer::runtime() -> double {
  if (!stop_) {
    stop_ = clock();
  }
  return (double) (stop_ - start_) / CLOCKS_PER_SEC;
}

auto timer::start() -> double {
  stop_ = 0;
  return start_ = clock();
}

auto timer::stop() -> double {
  stop_ = clock();
  total_ += stop_ - start_;
  return stop_;
}

auto timer::total() -> double {
  return (double) total_ / CLOCKS_PER_SEC;
}
