/*  Python binding definitions for uni::Skeleton
    part of uni module
    Copyright 2020-2022 Lukas Cone

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

#include "datas/python/matrix44.hpp"
#include "datas/python/vectors.hpp"
#include "pyenum.hpp"
#include "pylist.hpp"
#include "pymotion.hpp"
#include "pyrts.hpp"
#include <vector>
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

namespace UniPy {

static const struct {
  int id;
  es::string_view name;
} trackTypes[]{
    {uni::MotionTrack::Position, "Position"},
    {uni::MotionTrack::Rotation, "Rotation"},
    {uni::MotionTrack::Scale, "Scale"},
    {uni::MotionTrack::Matrix, "Matrix"},
    {uni::MotionTrack::PositionRotationScale, "PositionRotationScale"},
    {uni::MotionTrack::SingleFloat, "SingleFloat"},
};

struct TrackTypeInfo {
  using value_type = decltype(trackTypes[0]);
  using iterator_type = std::add_pointer<value_type>::type;

  static constexpr const char *GetName() { return "uniMotionTrackType"; }
  static constexpr const char *GetDoc() { return "Uni Motion Track Type Enum"; }
  static size_t Len(PyObject *) {
    return sizeof(trackTypes) / sizeof(value_type);
  }
  static iterator_type begin() { return std::begin(trackTypes); }
  static iterator_type end() { return std::end(trackTypes); }
};

using TrackTypeEnum = Enum<TrackTypeInfo>;

static const struct {
  int id;
  es::string_view name;
} motionTypes[]{
    {uni::Motion::Absolute, "Absolute"},
    {uni::Motion::Relative, "Relative"},
    {uni::Motion::Additive, "Additive"},
    {uni::Motion::Delta, "Delta"},
};

struct MotionTypeInfo {
  using value_type = decltype(motionTypes[0]);
  using iterator_type = std::add_pointer<value_type>::type;

  static constexpr const char *GetName() { return "uniMotionType"; }
  static constexpr const char *GetDoc() { return "Uni Motion Type Enum"; }
  static size_t Len(PyObject *) {
    return sizeof(motionTypes) / sizeof(value_type);
  }
  static iterator_type begin() { return std::begin(motionTypes); }
  static iterator_type end() { return std::end(motionTypes); }
};

using MotionTypeEnum = Enum<MotionTypeInfo>;

struct MotionTrackListInfo {
  using item_type = uni::MotionTrack;
  using wrap_type = MotionTrack;
  static constexpr const char *GetName() { return "uni::MotionTrackList"; }
  static constexpr const char *GetDoc() {
    return "Uni MotionTracks iterator/list";
  }
};

using MotionTrackList = List<MotionTrackListInfo>;

struct MotionListInfo {
  using item_type = uni::Motion;
  using wrap_type = Motion;
  static constexpr const char *GetName() { return "uni::MotionList"; }
  static constexpr const char *GetDoc() { return "Uni Motion iterator/list"; }
};

using MotionList = List<MotionListInfo>;

PyTypeObject *MotionTrack::GetType() {
  static PyMethodDef motionTrackMethods[] = {
      {"get_values", (PyCFunction)MotionTrack::GetValues, METH_VARARGS,
       "Get sampled values from times."},
      {NULL},
  };

  static PyGetSetDef motionTrackGetSets[] = {
      {"track_type", (getter)MotionTrack::TrackType, nullptr,
       "Motion track type."},
      {"bone_index", (getter)MotionTrack::BoneIndex, nullptr, "Bone index."},
      {NULL},
  };

  static PyTypeObject motionTrackType{
      .tp_name = "uni::MotionTrack",
      .tp_basicsize = sizeof(MotionTrack),
      .tp_dealloc = (destructor)MotionTrack::Dealloc,
      .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IS_ABSTRACT,
      .tp_doc = "Uni MotionTrack interface",
      .tp_methods = motionTrackMethods,
      .tp_getset = (PyGetSetDef *)motionTrackGetSets,
  };

  return &motionTrackType;
}

void MotionTrack::Dealloc(MotionTrack *self) {
  auto t0 = std::move(self->item);
  Py_TYPE(self)->tp_free(self);
}

PyObject *MotionTrack::TrackType(MotionTrack *self, void *) {
  return PyLong_FromSize_t(self->item->TrackType());
}

PyObject *MotionTrack::BoneIndex(MotionTrack *self, void *) {
  return PyLong_FromSize_t(self->item->BoneIndex());
}

PyObject *MotionTrack::GetValues(MotionTrack *self, PyObject *index) {
  PyListObject *pTimes = nullptr;

  if (!PyArg_ParseTuple(index, "O!", &PyList_Type, &pTimes)) {
    return nullptr;
  }

  std::vector<float> times(PyList_Size((PyObject *)pTimes));
  auto cObj = pTimes->ob_item;

  for (auto &t : times) {
    t = static_cast<float>(PyFloat_AsDouble(*cObj++));
  }

  auto retList = PyList_New(times.size());
  size_t curItem = 0;

  switch (self->item->TrackType()) {
  case uni::MotionTrack::Position:
  case uni::MotionTrack::Rotation:
  case uni::MotionTrack::Scale: {
    Vector4A16 rtVal;
    for (auto &t : times) {
      self->item->GetValue(rtVal, t);
      PyList_SetItem(retList, curItem++, Py_BuildValue(rtVal));
    }
    break;
  }
  case uni::MotionTrack::SingleFloat: {
    float rtVal;
    for (auto &t : times) {
      self->item->GetValue(rtVal, t);
      auto cVal = PyFloat_FromDouble(rtVal);
      PyList_SetItem(retList, curItem++, cVal);
    }
    break;
  }
  case uni::MotionTrack::PositionRotationScale: {
    uni::RTSValue rts;
    for (auto &t : times) {
      self->item->GetValue(rts, t);
      PyList_SetItem(retList, curItem++, Py_BuildValue(rts));
    }
    break;
  }
  case uni::MotionTrack::Matrix: {
    es::Matrix44 mtx;
    for (auto &t : times) {
      self->item->GetValue(mtx, t);
      PyList_SetItem(retList, curItem++, Py_BuildValue(mtx));
    }
    break;
  }
  default:
    Py_RETURN_NONE;
  }

  return retList;
}

PyTypeObject *Motion::GetType() {
  static PyGetSetDef motionGetSets[] = {
      {"name", (getter)Motion::Name, nullptr, "Motion name."},
      {"duration", (getter)Motion::Duration, nullptr,
       "Motion duration in seconds."},
      {"motion_type", (getter)Motion::MotionType, nullptr, "Motion type."},
      {"framerate", (getter)Motion::FrameRate, (setter)Motion::SetFrameRate,
       "Motion framerate."},
      {"tracks", (getter)Motion::Tracks, nullptr, "Motion tracks."},
      {NULL},
  };

  static constexpr size_t motionTypeFlags =
      Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IS_ABSTRACT | Py_TPFLAGS_BASETYPE;

  static PyTypeObject motionType{
      .tp_name = "uni::Motion",
      .tp_basicsize = sizeof(Motion),
      .tp_dealloc = (destructor)Motion::Dealloc,
      .tp_flags = motionTypeFlags,
      .tp_doc = "Uni Motion interface",
      .tp_getset = (PyGetSetDef *)motionGetSets,
  };

  return &motionType;
}

PyTypeObject *Motion::GetListType() { return MotionList::GetType(); }

void Motion::Dealloc(Motion *self) {
  auto t0 = std::move(self->item);
  Py_TYPE(self)->tp_free(self);
}

PyObject *Motion::MotionType(Motion *self, void *) {
  return PyLong_FromSize_t(self->item->MotionType());
}

PyObject *Motion::Name(Motion *self, void *) {
  const auto rName = self->item->Name();
  return PyUnicode_FromStringAndSize(rName.data(), rName.size());
}

PyObject *Motion::Duration(Motion *self, void *) {
  return PyFloat_FromDouble(self->item->Duration());
}

PyObject *Motion::Tracks(Motion *self, void *) {
  return MotionTrackList::Create(self->item->Tracks());
}

PyObject *Motion::FrameRate(Motion *self, void *) {
  return PyLong_FromLong(self->item->FrameRate());
}

int Motion::SetFrameRate(Motion *self, PyObject *var, void *) {
  self->sitem->FrameRate(PyLong_AsLong(var));
  return 0;
}

Motion *Motion::Create(uni::Element<const uni::Motion> &&tp) {
  auto obj = reinterpret_cast<Motion *>(
      PyType_GenericNew(GetType(), nullptr, nullptr));
  obj->item = std::move(tp);
  return obj;
}

PyObject *Motion::Create(uni::MotionsConst &&tp) {
  return MotionList::Create(std::move(tp));
}

void Motion::InitType(PyObject *module) {
  PyAddTypes<TrackTypeEnum, MotionTypeEnum, MotionTrack, MotionTrackList,
             Motion, MotionList>(module);
}

} // namespace UniPy
#pragma GCC diagnostic pop
