/**
 * Adding code that is relevant to a few HGraph's simplifying instructions,
 * that can also be applied in LLVM.
 *
 * Copyright (C) 2021  Paschalis Mpeis (paschalis.mpeis-AT-gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "hgraph_to_llvm.h"

#include "optimizing/escape.h"
#include "hgraph_to_llvm-inl.h"
#include "llvm_macros_irb_.h"

namespace art {
namespace LLVM {

// Helper method for StringBuffer escape analysis.
static bool NoEscapeForStringBufferReference(HInstruction* reference, HInstruction* user) {
  if (user->IsInvokeStaticOrDirect()) {
    // Any constructor on StringBuffer is okay.
    return user->AsInvokeStaticOrDirect()->GetResolvedMethod() != nullptr &&
      user->AsInvokeStaticOrDirect()->GetResolvedMethod()->IsConstructor() &&
      user->InputAt(0) == reference;
  } else if (user->IsInvokeVirtual()) {
    switch (user->AsInvokeVirtual()->GetIntrinsic()) {
      case Intrinsics::kStringBufferLength:
      case Intrinsics::kStringBufferToString:
        DCHECK_EQ(user->InputAt(0), reference); return true;
      case Intrinsics::kStringBufferAppend:
        // Returns "this", so only okay if no further uses.
        DCHECK_EQ(user->InputAt(0), reference);
        DCHECK_NE(user->InputAt(1), reference);
        return !user->HasUses();
      default:
        break;
    }
  }
  return false;
}

// Certain allocation intrinsics are not removed by dead code elimination
// because of potentially throwing an OOM exception or other side effects.
// This method removes such intrinsics when special circumstances allow.
bool HGraphToLLVM::SimplifyAllocationIntrinsic(HInvoke* invoke) {
  if (!invoke->HasUses()) {
    // Instruction has no uses. If unsynchronized, we can remove right away, safely ignoring
    // the potential OOM of course. Otherwise, we must ensure the receiver object of this
    // call does not escape since only thread-local synchronization may be removed.
    bool is_synchronized = invoke->GetIntrinsic() == Intrinsics::kStringBufferToString;
    HInstruction* receiver = invoke->InputAt(0);
    if (!is_synchronized || DoesNotEscape(receiver, NoEscapeForStringBufferReference)) {
      VERIFY_LLVMD3("SimplifyAllocationIntrinsic: removed "
        << GetTwine(invoke));
      return true;
    }
  }

  // the call wasn't removed (so it have to be handled, by invoking the method)
  return false;
}

void HGraphToLLVM::SimplifyReturnThis(HInvoke* invoke) {
  if (invoke->HasUses()) {
    HInstruction* receiver = invoke->InputAt(0);
    invoke->ReplaceWith(receiver);
  }
}

#include "llvm_macros_undef.h"

}  // namespace LLVM
}  // namespace art
