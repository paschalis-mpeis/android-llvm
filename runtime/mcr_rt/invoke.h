/**
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
#ifndef ART_RUNTIME_MCR_INVOKE_H_
#define ART_RUNTIME_MCR_INVOKE_H_

#include "scoped_thread_state_change.h"
#include "stack_reference.h"

namespace art {

class ArtMethod;
union JValue;

namespace mcr {

class ArgArray {
 public:
  explicit ArgArray(const char* shorty, uint32_t shorty_len);

  void DebugInvoke(const ScopedObjectAccessAlreadyRunnable& soa,
              ArtMethod* method, JValue* result,
              const char* shorty);

  void Invoke(const ScopedObjectAccessAlreadyRunnable& soa,
              ArtMethod* method, JValue* result,
              const char* shorty);

  uint32_t* GetArgs() {
    return arg_array_;
  }

  uint32_t GetNumBytes() {
    return num_bytes_;
  }

  void Append(uint32_t value) {
    arg_array_[num_bytes_ / 4] = value;
    num_bytes_ += 4;
  }


  void Append(ObjPtr<mirror::Object> obj)
    REQUIRES_SHARED(Locks::mutator_lock_) {
    Append(StackReference<mirror::Object>::FromMirrorPtr(obj.Ptr()).AsVRegValue());
  }

  void AppendWide(uint64_t value) {
    arg_array_[num_bytes_ / 4] = value;
    arg_array_[(num_bytes_ / 4) + 1] = value >> 32;
    num_bytes_ += 8;
  }

  void AppendFloat(float value) {
    jvalue jv;
    jv.f = value;
    Append(jv.i);
  }

  void AppendDouble(double value) {
    jvalue jv;
    jv.d = value;
    AppendWide(jv.j);
  }

  void BuildArgArrayFromVarArgs(const ScopedObjectAccessAlreadyRunnable& soa,
      mirror::Object* receiver, va_list ap)
    REQUIRES_SHARED(Locks::mutator_lock_) {
      // Set receiver if non-null (method is not static)
      if (receiver != nullptr) {
        Append(receiver);
      }
      for (size_t i = 1; i < shorty_len_; ++i) {
        switch (shorty_[i]) {
          case 'Z':
          case 'B':
          case 'C':
          case 'S':
          case 'I':
            Append(va_arg(ap, jint));
            break;
          case 'F':
            AppendFloat(va_arg(ap, jdouble));
            break;
          case 'L':
            Append(soa.Decode<mirror::Object>(va_arg(ap, jobject)));
            break;
          case 'D':
            AppendDouble(va_arg(ap, jdouble));
            break;
          case 'J':
            AppendWide(va_arg(ap, jlong));
            break;
#ifndef NDEBUG
          default:
            DLOG(FATAL) << "Unexpected shorty character: " << shorty_[i];
#endif
        }
      }
    }

 private:
  enum { kSmallArgArraySize = 16 };
  const char* const shorty_;
  const uint32_t shorty_len_;
  uint32_t num_bytes_;
  uint32_t* arg_array_;
  uint32_t small_arg_array_[kSmallArgArraySize];
  std::unique_ptr<uint32_t[]> large_arg_array_;
};

}  // namespace mcr
}  // namespace art

#endif  // ART_RUNTIME_MCR_INVOKE_H_
