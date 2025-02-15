/*  Common classes for uni module
    Copyright 2020-2021 Lukas Cone

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#pragma once
#include "datas/deleter_hybrid.hpp"
#include "datas/settings.hpp"
#include <memory>

class Reflector;

namespace uni {
template <class C> using Element = std::unique_ptr<C, es::deleter_hybrid>;
using MetadataConst = Element<const Reflector>;

class PC_EXTERN Base {
public:
  virtual ~Base() = default;
  virtual MetadataConst Metadata() const;
};

enum TransformType { TMTYPE_RTS, TMTYPE_MATRIX };

using BaseElementConst = Element<const Base>;
using BaseElement = Element<Base>;

template <class C> auto ToElement(std::unique_ptr<C> &uptr) {
  return Element<C>{uptr.release()};
}

template <class C> auto ToElement(std::unique_ptr<C> &&uptr) {
  return Element<C>{uptr.release()};
}
} // namespace uni
