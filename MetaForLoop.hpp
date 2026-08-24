// Copyright © 2024, Marco Battiato <marco.battiato@ntu.edu.sg; battiato.marco@gmail.com>, All rights reserved.
//
// Licensed under the GNU GENERAL PUBLIC LICENSE Version 3 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   https://www.gnu.org/licenses/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.
//
//
//  MetaForLoop
//
//  Created by Marco Battiato on 20/4/24.
//
//  Generates arbitrarily deep nested loops from a single call, with the nesting
//  depth fixed at compile time by the number of bounds supplied.
//
//      metaForLoop(f, 0,3, 0,4, 0,5);      //  equivalent to three nested for loops,
//                                          //  calling f(i,j,k) for every combination
//
//  Bounds are passed as consecutive (begin, end) pairs; a static_assert enforces that
//  they are supplied in pairs. The callable is invoked with one index per level, so
//  its arity must match the number of pairs.
//
//  metaForLoopParallel is the parallel counterpart, distributing the outermost one,
//  two or three levels over a TBB blocked range and running any deeper levels serially
//  within each block.
//
//  Requires C++20 (concepts) and oneTBB for the parallel variants.
//

#ifndef MetaForLoop_hpp
#define MetaForLoop_hpp

#include <concepts>
#include <cstddef>
#include <iterator>

#include "tbb/tbb.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"

namespace MetaForLoop {

// Serial N-dimensional loop. Each recursion peels off one (begin, end) pair and binds
// the corresponding index into the callable, so the innermost call sees all N indices.
template<class Callable, std::incrementable Counter, std::convertible_to<Counter> ...C>
constexpr void metaForLoop(Callable&& functToExecute, Counter start, Counter end, C&&... limits) {
    constexpr std::size_t nPar = sizeof...(C);
    static_assert( !(nPar%2), "Loop bounds must be supplied as (begin, end) pairs" );
    for(Counter i = start; i != end; ++i) {
        if constexpr(nPar == 0) {
            functToExecute(i);
        } else {
            auto bind_an_argument = [i, &functToExecute](auto... args) {
                functToExecute(i, args...);
            };
            metaForLoop(bind_an_argument, limits...);
        }
    }
}

// Parallel over the outermost three levels; deeper levels run serially inside each
// block. Recursing into the parallel version here instead would open a fresh parallel
// region for every innermost iteration, which costs far more than it gains.
template<class Callable, std::incrementable Counter, std::convertible_to<Counter> ...C>
constexpr void metaForLoopParallel(Callable&& functToExecute, Counter start0, Counter end0, Counter start1, Counter end1, Counter start2, Counter end2, C&&...limits) {
    constexpr std::size_t nPar = sizeof...(C);
    static_assert( !(nPar% 2), "Loop bounds must be supplied as (begin, end) pairs" );
    tbb::parallel_for(
                      tbb::blocked_range3d<Counter>(start0, end0, start1, end1, start2, end2),
                      [&](tbb::blocked_range3d<Counter> range) {
                          for(auto i=range.pages().begin(); i<range.pages().end(); i++){
                              for(auto j=range.rows().begin(); j<range.rows().end(); j++){
                                  for(auto k=range.cols().begin(); k<range.cols().end(); k++){
                                      if constexpr(nPar == 0) {
                                          functToExecute(i,j,k);
                                      } else {
                                          auto bind_an_argument = [i,j,k, &functToExecute](auto... args) {
                                              functToExecute(i,j,k, args...);
                                          };
                                          metaForLoop(bind_an_argument, limits...);
                                      }
                                  }
                              }
                          }
                      });
}

// Parallel over both levels of a two-dimensional iteration space.
template<class Callable, std::incrementable Counter>
constexpr void metaForLoopParallel(Callable&& functToExecute, Counter start0, Counter end0, Counter start1, Counter end1) {
    tbb::parallel_for(
                      tbb::blocked_range2d<Counter>(start0, end0, start1, end1),
                      [&](tbb::blocked_range2d<Counter> range) {
                          for(auto i=range.rows().begin(); i<range.rows().end(); i++){
                              for(auto j=range.cols().begin(); j<range.cols().end(); j++){
                                    functToExecute(i,j);
                              }
                          }
                      });
}

// Parallel over a single dimension.
template<class Callable, std::incrementable Counter>
constexpr void metaForLoopParallel(Callable&& functToExecute, Counter start0, Counter end0) {
    tbb::parallel_for(
                      tbb::blocked_range<Counter>(start0, end0),
                      [&](tbb::blocked_range<Counter> range) {
                          for(auto i=range.begin(); i<range.end(); ++i) {
                                functToExecute(i);
                          }
                      });
}

} // namespace MetaForLoop

#endif /* MetaForLoop_hpp */
