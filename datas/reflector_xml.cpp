/*  a XML I/O source for Reflector class
    more info in README for PreCore Project

    Copyright 2019-2021 Lukas Cone

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

#include "reflector_xml.hpp"
#include "master_printer.hpp"
#include "pugiex.hpp"

struct ReflectedInstanceFriend : ReflectedInstance {
  void *Instance() { return instance; }
  const void *Instance() const { return constInstance; }
  const reflectorStatic *Refl() const { return rfStatic; }
};

class ReflectorFriend : public Reflector {
public:
  using Reflector::GetReflectedInstance;
  using Reflector::GetReflectedType;
};

auto GetMakeAttribute(pugi::xml_node node, const char *name) {
  if (auto child = node.attribute(name); child) {
    return child;
  } else {
    return node.append_attribute(name);
  }
}

auto GetMakeChild(pugi::xml_node node, const char *name) {
  if (auto child = node.child(name); child) {
    return child;
  } else {
    return node.append_child(name);
  }
}

static bool SaveV2(const ReflType &cType, const Reflector &ri,
                   pugi::xml_node thisNode, size_t t,
                   const std::string &varName,
                   ReflectorXMLUtil::flag_type flags) {
  switch (cType.type) {
  case REFType::String:
  case REFType::CString: {
    if (!flags[ReflectorXMLUtil::Flags_StringAsAttribute]) {
      return false;
    }
  }
    [[fallthrough]];
  case REFType::Bool:
  case REFType::Enum:
  case REFType::FloatingPoint:
  case REFType::Integer:
  case REFType::UnsignedInteger:
  case REFType::BitFieldMember: {
    std::string str = ri.GetReflectedValue(t);
    auto cNode = GetMakeAttribute(thisNode, varName.data());
    cNode.set_value(str.data());
    return true;
  }
  case REFType::Vector: {
    static const char axes[4][2]{"x", "y", "z", "w"};
    pugi::xml_node cNode = GetMakeChild(thisNode, varName.c_str());

    for (size_t a = 0; a < cType.asVector.numItems; a++) {
      std::string str = ri.GetReflectedValue(t, a);
      GetMakeAttribute(cNode, axes[a]).set_value(str.data());
    }

    return true;
  }

  case REFType::EnumFlags: {
    if (!ReflectedEnum::Registry().count(JenHash(cType.asClass.typeHash))) {
      return false;
    }

    auto cEnum = ReflectedEnum::Registry().at(JenHash(cType.asClass.typeHash));
    pugi::xml_node cNode = GetMakeChild(thisNode, varName.c_str());

    for (size_t e = 0; e < cEnum->numMembers; e++) {
      auto name = cEnum->names[e];
      const uint64 value = cEnum->values[e];
      auto valueName = ri.GetReflectedValue(t, value);
      GetMakeAttribute(cNode, name).set_value(valueName.data());
    }

    return true;
  }
  default:
    return false;
  }
}

static pugi::xml_node MakeNode(const Reflector &, const reflectorStatic *stat,
                               pugi::xml_node node) {
  std::string className;

  if (stat->className)
    className = stat->className;
  else {
    className.resize(15);
    const auto cHash = stat->classHash.raw();
    snprintf(&className[0], 15, "h:%X", cHash);
  }

  return GetMakeChild(node, className.c_str());
}

static std::string GetName(const Reflector &, const reflectorStatic *stat,
                           const ReflType &cType, size_t t) {
  std::string varName;

  if (stat->typeNames && stat->typeNames[t]) {
    varName = stat->typeNames[t];
  } else {
    varName.resize(15);
    const auto cHash = cType.valueNameHash.raw();
    snprintf(&varName[0], 15, "h:%X", cHash);
  }

  return varName;
}

pugi::xml_node ReflectorXMLUtil::Save(const Reflector &ri, pugi::xml_node node,
                                      bool asNewNode) {
  auto &&rif = static_cast<const ReflectorFriend &>(ri);
  auto stat =
      static_cast<ReflectedInstanceFriend &&>(rif.GetReflectedInstance())
          .Refl();
  pugi::xml_node thisNode = asNewNode ? MakeNode(ri, stat, node) : node;

  for (size_t t = 0; t < stat->nTypes; t++) {
    auto &&cType = stat->types[t];
    std::string varName = GetName(ri, stat, cType, t);
    pugi::xml_node cNode = GetMakeChild(thisNode, varName.c_str());

    if (ri.IsReflectedSubClass(t)) {
      if (ri.IsArray(t)) {
        const int numItems = cType.asArray.numItems;

        for (int s = 0; s < numItems; s++) {
          auto subRef = ri.GetReflectedSubClass(t, s);
          if (!subRef) {
            throw std::runtime_error("Class not registered!");
          }
          ReflectorPureWrap subCl(subRef);
          ReflectorXMLUtil::Save(
              subCl, GetMakeChild(cNode, ("i:" + std::to_string(s)).c_str()),
              false);
        }

      } else {
        auto subRef = ri.GetReflectedSubClass(t);
        if (!subRef) {
          throw std::runtime_error("Class not registered!");
        }
        ReflectorPureWrap subCl(subRef);
        ReflectorXMLUtil::Save(subCl, cNode, false);
      }
    } else {
      std::string str = ri.GetReflectedValue(t);
      cNode.append_buffer(str.c_str(), str.size());
    }
  }

  return thisNode;
}

pugi::xml_node ReflectorXMLUtil::SaveV2(const Reflector &ri,
                                        pugi::xml_node node, bool asNewNode) {
  flag_type opts;
  opts.Set(Flags_ClassNode, asNewNode);
  return SaveV2a(ri, node, opts);
}

pugi::xml_node ReflectorXMLUtil::SaveV2a(const Reflector &ri,
                                         pugi::xml_node node, flag_type opts) {
  auto &&rif = static_cast<const ReflectorFriend &>(ri);
  auto stat =
      static_cast<ReflectedInstanceFriend &&>(rif.GetReflectedInstance())
          .Refl();
  pugi::xml_node thisNode =
      opts[Flags_ClassNode] ? MakeNode(ri, stat, node) : node;

  for (size_t t = 0; t < stat->nTypes; t++) {
    auto &&cType = stat->types[t];
    std::string varName = GetName(ri, stat, cType, t);

    if (::SaveV2(cType, ri, thisNode, t, varName, opts)) {
      continue;
    }

    if (ri.IsReflectedSubClass(t)) {
      if (ri.IsArray(t)) {
        const int numItems = cType.asArray.numItems;

        for (int s = 0; s < numItems; s++) {
          auto subRef = ri.GetReflectedSubClass(t, s);
          if (!subRef) {
            throw std::runtime_error("Class not registered!");
          }
          ReflectorPureWrap subCl(subRef);
          auto nodeName = varName + '-' + std::to_string(s);
          pugi::xml_node cNode = GetMakeChild(thisNode, nodeName.c_str());
          auto subOpts = opts;
          subOpts -= Flags_ClassNode;
          SaveV2a(subCl, cNode, subOpts);
        }

      } else {
        pugi::xml_node cNode = GetMakeChild(thisNode, varName.c_str());
        auto subRef = ri.GetReflectedSubClass(t);
        if (!subRef) {
          throw std::runtime_error("Class not registered!");
        }
        ReflectorPureWrap subCl(subRef);
        auto subOpts = opts;
        subOpts -= Flags_ClassNode;
        SaveV2a(subCl, cNode, subOpts);
      }
    } else if (ri.IsArray(t)) {
      const auto &arr = cType.asArray;
      const int numItems = arr.numItems;
      switch (arr.type) {
      case REFType::Bool:
      case REFType::Enum:
      case REFType::FloatingPoint:
      case REFType::Integer:
      case REFType::UnsignedInteger:
      case REFType::BitFieldMember: {
        for (int s = 0; s < numItems; s++) {
          std::string str = ri.GetReflectedValue(t, s);
          auto nodeName = varName + '-' + std::to_string(s);
          auto cNode = GetMakeAttribute(thisNode, nodeName.data());
          cNode.set_value(str.data());
        }
        break;
      }
      case REFType::Vector: {
        for (int s = 0; s < numItems; s++) {
          static const char axes[4][2]{"x", "y", "z", "w"};
          auto nodeName = varName + '-' + std::to_string(s);
          pugi::xml_node cNode = GetMakeChild(thisNode, nodeName.c_str());

          for (size_t a = 0; a < arr.asVector.numItems; a++) {
            std::string str = ri.GetReflectedValue(t, s, a);
            GetMakeAttribute(cNode, axes[a]).set_value(str.data());
          }
        }
        break;
      }
      case REFType::EnumFlags: {
        if (!ReflectedEnum::Registry().count(JenHash(arr.asClass.typeHash))) {
          break;
        }

        auto &&cEnum =
            ReflectedEnum::Registry().at(JenHash(arr.asClass.typeHash));

        for (int s = 0; s < numItems; s++) {
          auto nodeName = varName + '-' + std::to_string(s);
          pugi::xml_node cNode = GetMakeChild(thisNode, nodeName.c_str());

          for (size_t e = 0; e < cEnum->numMembers; e++) {
            auto name = cEnum->names[e];
            const uint64 value = cEnum->values[e];
            auto valueName = ri.GetReflectedValue(t, s, value);
            GetMakeAttribute(cNode, name).set_value(valueName.data());
          }
        }
        break;
      }
      default: {
        pugi::xml_node cNode = GetMakeChild(thisNode, varName.c_str());
        std::string str = ri.GetReflectedValue(t);
        cNode.append_buffer(str.c_str(), str.size());
        break;
      }
      }
    } else {
      pugi::xml_node cNode = GetMakeChild(thisNode, varName.c_str());
      std::string str = ri.GetReflectedValue(t);
      cNode.append_buffer(str.c_str(), str.size());
    }
  }

  return thisNode;
}

pugi::xml_node ReflectorXMLUtil::LoadV2(Reflector &ri, pugi::xml_node node,
                                        bool lookupClassNode) {
  auto &&rif = static_cast<ReflectorFriend &>(ri);
  const reflectorStatic *stat =
      static_cast<ReflectedInstanceFriend &&>(rif.GetReflectedInstance())
          .Refl();
  pugi::xml_node thisNode;
  static constexpr size_t nan_ = -1;
  struct retval {
    JenHash hash;
    size_t index = nan_;
  };

  auto MakeHash = [](es::string_view name) -> JenHash {
    if (name[0] == 'h' && name[1] == ':') {
      name.remove_prefix(2);
      return JenHash(strtoul(name.data(), nullptr, 16));
    } else {
      return name;
    }
  };

  auto MakeNode = [MakeHash](auto a) {
    es::string_view name(a.name());
    retval retVal;
    const size_t found = name.find_last_of('-');

    if (found != name.npos) {
      char *endChar = nullptr;
      const char *startChar = name.data() + found + 1;
      auto index = strtoll(startChar, &endChar, 10);

      if (startChar != endChar) {
        name = name.substr(0, found);
        retVal.index = index;
      }
    }

    retVal.hash = MakeHash(name);

    return retVal;
  };

  if (lookupClassNode) {
    thisNode = node.find_child([&](pugi::xml_node nde) {
      return !nde.empty() && MakeHash(nde.name()) == stat->classHash;
    });
  } else {
    thisNode = node;
  }

  for (auto a : thisNode.attributes()) {
    auto node = MakeNode(a);
    if (node.index == nan_) {
      ri.SetReflectedValue(node.hash, a.value());
    } else {
      ri.SetReflectedValue(node.hash, a.value(), node.index);
    }
  }

  for (auto a : thisNode.children()) {
    auto node = MakeNode(a);

    if (ri.IsReflectedSubClass(node.hash)) {
      if (node.index == nan_) {
        node.index = 0;
      }

      auto rfInst = ri.GetReflectedSubClass(node.hash, node.index);
      if (!rfInst) {
        throw std::runtime_error("Class not registered!");
      }
      ReflectorPureWrap subRefl(rfInst);
      ReflectorXMLUtil::LoadV2(subRefl, a);
      continue;
    }

    if (a.attributes_begin() == a.attributes_end()) {
      ri.SetReflectedValue(node.hash, a.text().as_string());
      continue;
    }

    auto refType = rif.GetReflectedType(node.hash);

    if (!refType) {
      continue;
    }

    auto DoVector = [&] {
      for (auto t : a.attributes()) {
        size_t element = -1;

        switch (*t.name()) {
        case 'x':
          element = 0;
          break;
        case 'y':
          element = 1;
          break;
        case 'z':
          element = 2;
          break;
        case 'w':
          element = 3;
          break;
        }

        if (node.index == nan_) {
          ri.SetReflectedValue(refType->index, t.value(), element);
        } else {
          ri.SetReflectedValue(refType->index, t.value(), node.index, element);
        }
      }
    };

    auto DoFlags = [&] {
      std::string cpString;

      for (auto t : a.attributes()) {
        if (t.as_bool()) {
          cpString.append(t.name());
          cpString.push_back('|');
        }
      }

      if (cpString.empty()) {
        cpString = "NULL";
      } else {
        cpString.pop_back();
      }

      if (node.index == nan_) {
        ri.SetReflectedValue(refType->index, cpString.data());
      } else {
        ri.SetReflectedValue(refType->index, cpString.data(), node.index);
      }
    };

    switch (refType->type) {
    case REFType::EnumFlags: {
      DoFlags();
      break;
    }
    case REFType::Vector:
      DoVector();
      break;

    case REFType::Array: {
      switch (refType->asArray.type) {
      case REFType::Vector:
        DoVector();
        break;
      case REFType::EnumFlags:
        DoFlags();
        break;

      default:
        break;
      }
    }

    default:
      break;
    }
  }

  return thisNode;
}

pugi::xml_node ReflectorXMLUtil::Load(Reflector &ri, pugi::xml_node node,
                                      bool lookupClassNode) {
  auto &&rif = static_cast<ReflectorFriend &>(ri);
  const reflectorStatic *stat =
      static_cast<ReflectedInstanceFriend &&>(rif.GetReflectedInstance())
          .Refl();
  pugi::xml_node thisNode;

  auto MakeHash = [](es::string_view name) -> JenHash {
    if (name[0] == 'h' && name[1] == ':') {
      name.remove_prefix(2);
      return JenHash(strtoul(name.data(), nullptr, 16));
    } else {
      return name;
    }
  };

  if (lookupClassNode) {
    thisNode = node.find_child([&](pugi::xml_node nde) {
      return !nde.empty() && MakeHash(nde.name()) == stat->classHash;
    });
  } else {
    thisNode = node;
  }

  if (thisNode.empty()) {
    return thisNode;
  }

  for (auto &a : thisNode.children()) {
    auto hash = MakeHash(a.name());

    if (ri.IsReflectedSubClass(hash)) {
      if (ri.IsArray(hash)) {
        for (auto sc : a.children()) {
          auto index = atoll(sc.name() + 2);
          auto rfInst = ri.GetReflectedSubClass(hash, index);
          if (!rfInst) {
            throw std::runtime_error("Class not registered!");
          }
          ReflectorPureWrap subRefl(rfInst);
          ReflectorXMLUtil::Load(subRefl, sc);
        }

      } else {
        auto rfInst = ri.GetReflectedSubClass(hash);
        if (!rfInst) {
          throw std::runtime_error("Class not registered!");
        }
        ReflectorPureWrap subRefl(rfInst);
        ReflectorXMLUtil::Load(subRefl, a);
      }
      continue;
    }

    ri.SetReflectedValue(hash, a.text().as_string());
  }

  return thisNode;
}
