/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jit_LICM_h
#define jit_LICM_h

// This file represents the Loop Invariant Code Motion optimization pass

#include "mozilla/Attributes.h"

namespace js {
namespace jit {

class MIRGenerator;
class MIRGraph;

MOZ_MUST_USE bool LICM(MIRGenerator* mir, MIRGraph& graph);

}  // namespace jit
}  // namespace js

#endif /* jit_LICM_h */