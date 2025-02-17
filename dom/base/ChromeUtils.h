/* -*-  Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_ChromeUtils__
#define mozilla_dom_ChromeUtils__

#include "mozilla/AlreadyAddRefed.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/ChromeUtilsBinding.h"
#include "mozilla/dom/ThreadSafeChromeUtilsBinding.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {

namespace devtools {
class HeapSnapshot;
} // namespace devtools

namespace dom {

class ThreadSafeChromeUtils
{
public:
  // Implemented in toolkit/devtools/server/HeapSnapshot.cpp
  static void SaveHeapSnapshot(GlobalObject& global,
                               const HeapSnapshotBoundaries& boundaries,
                               nsAString& filePath,
                               ErrorResult& rv);

  // Implemented in toolkit/devtools/server/HeapSnapshot.cpp
  static already_AddRefed<devtools::HeapSnapshot> ReadHeapSnapshot(GlobalObject& global,
                                                                   const nsAString& filePath,
                                                                   ErrorResult& rv);

  static void NondeterministicGetWeakMapKeys(GlobalObject& aGlobal,
                                             JS::Handle<JS::Value> aMap,
                                             JS::MutableHandle<JS::Value> aRetval,
                                             ErrorResult& aRv);

  static void NondeterministicGetWeakSetKeys(GlobalObject& aGlobal,
                                             JS::Handle<JS::Value> aSet,
                                             JS::MutableHandle<JS::Value> aRetval,
                                             ErrorResult& aRv);
};

class ChromeUtils : public ThreadSafeChromeUtils
{
public:
  static void
  OriginAttributesToSuffix(dom::GlobalObject& aGlobal,
                           const dom::OriginAttributesDictionary& aAttrs,
                           nsCString& aSuffix);

  static bool
  OriginAttributesMatchPattern(dom::GlobalObject& aGlobal,
                               const dom::OriginAttributesDictionary& aAttrs,
                               const dom::OriginAttributesPatternDictionary& aPattern);
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_ChromeUtils__
