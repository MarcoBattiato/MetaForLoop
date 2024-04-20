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
// DISCLAIMER: This is a version under active development and testing.
// Not all features have been sufficiently tested, and several features
// are only partially implemented. Moreover both the core and the interface
// may change. You are discouraged from using this version to publish
// results without the supervision of the developer.
//
// If you want, you are welcome to act as a beta tester. In that case
// please contact Marco Battiato at:
// battiato.marco@gmail.com
//
// Check if a newer, stable and tested version has, in the meanwhile,
// been made available at:
// https://github.com/MarcoBattiato/MetaForLoop
//
//
//  MetaForLoop
//
//  Created by Marco Battiato on 20/4/24.
//

#ifndef MetaForLoop_hpp
#define MetaForLoop_hpp

#include "tbb/tbb.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"

namespace MetaForLoop {

template<class Callable, std::incrementable Counter, std::convertible_to<Counter> ...C>
constexpr void metaForLoop(Callable&& functToExecute, Counter start, Counter end, C&&... limits) {
    constexpr std::size_t nPar = sizeof...(C);
    static_assert( !(nPar%2) );
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

template<class Callable, std::incrementable Counter, std::convertible_to<Counter> ...C>
constexpr void metaForLoopParallel(Callable&& functToExecute, Counter start0, Counter end0, Counter start1, Counter end1, Counter start2, Counter end2, C&&...limits) {
    constexpr std::size_t nPar = sizeof...(C);
    static_assert( !(nPar% 2) );
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
                                          metaForLoopParallel(bind_an_argument, limits...);
                                      }
                                  }
                              }
                          }
                      });
}

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
