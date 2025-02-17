/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FormData.h"
#include "nsIVariant.h"
#include "nsIInputStream.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/HTMLFormElement.h"

#include "MultipartBlobImpl.h"

using namespace mozilla;
using namespace mozilla::dom;

FormData::FormData(nsISupports* aOwner)
  : nsFormSubmission(NS_LITERAL_CSTRING("UTF-8"), nullptr)
  , mOwner(aOwner)
{
}

namespace {

// Implements steps 3 and 4 of the "create an entry" algorithm of FormData.
already_AddRefed<File>
CreateNewFileInstance(Blob& aBlob, const Optional<nsAString>& aFilename,
                      ErrorResult& aRv)
{
  // Step 3 "If value is a Blob object and not a File object, set value to
  // a new File object, representing the same bytes, whose name attribute value
  // is "blob"."
  // Step 4 "If value is a File object and filename is given, set value to
  // a new File object, representing the same bytes, whose name attribute
  // value is filename."
  nsAutoString filename;
  if (aFilename.WasPassed()) {
    filename = aFilename.Value();
  } else {
    // If value is already a File and filename is not passed, the spec says not
    // to create a new instance.
    RefPtr<File> file = aBlob.ToFile();
    if (file) {
      return file.forget();
    }

    filename = NS_LITERAL_STRING("blob");
  }

  RefPtr<File> file = aBlob.ToFile(filename, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  return file.forget();
}

} // namespace

// -------------------------------------------------------------------------
// nsISupports

NS_IMPL_CYCLE_COLLECTION_CLASS(FormData)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(FormData)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mOwner)

  for (uint32_t i = 0, len = tmp->mFormData.Length(); i < len; ++i) {
    ImplCycleCollectionUnlink(tmp->mFormData[i].value);
  }

  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(FormData)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOwner)

  for (uint32_t i = 0, len = tmp->mFormData.Length(); i < len; ++i) {
    ImplCycleCollectionTraverse(cb, tmp->mFormData[i].value,
                                "mFormData[i].GetAsFile()", 0);
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(FormData)

NS_IMPL_CYCLE_COLLECTING_ADDREF(FormData)
NS_IMPL_CYCLE_COLLECTING_RELEASE(FormData)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(FormData)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMFormData)
  NS_INTERFACE_MAP_ENTRY(nsIXHRSendable)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMFormData)
NS_INTERFACE_MAP_END

// -------------------------------------------------------------------------
// nsFormSubmission
nsresult
FormData::GetEncodedSubmission(nsIURI* aURI,
                               nsIInputStream** aPostDataStream)
{
  NS_NOTREACHED("Shouldn't call FormData::GetEncodedSubmission");
  return NS_OK;
}

void
FormData::Append(const nsAString& aName, const nsAString& aValue,
                 ErrorResult& aRv)
{
  AddNameValuePair(aName, aValue);
}

void
FormData::Append(const nsAString& aName, Blob& aBlob,
                 const Optional<nsAString>& aFilename,
                   ErrorResult& aRv)
{
  RefPtr<File> file = CreateNewFileInstance(aBlob, aFilename, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return;
  }

  AddNameFilePair(aName, file);
}

void
FormData::Delete(const nsAString& aName)
{
  // We have to use this slightly awkward for loop since uint32_t >= 0 is an
  // error for being always true.
  for (uint32_t i = mFormData.Length(); i-- > 0; ) {
    if (aName.Equals(mFormData[i].name)) {
      mFormData.RemoveElementAt(i);
    }
  }
}

void
FormData::Get(const nsAString& aName,
              Nullable<OwningFileOrUSVString>& aOutValue)
{
  for (uint32_t i = 0; i < mFormData.Length(); ++i) {
    if (aName.Equals(mFormData[i].name)) {
      aOutValue.SetValue() = mFormData[i].value;
      return;
    }
  }

  aOutValue.SetNull();
}

void
FormData::GetAll(const nsAString& aName,
                 nsTArray<OwningFileOrUSVString>& aValues)
{
  for (uint32_t i = 0; i < mFormData.Length(); ++i) {
    if (aName.Equals(mFormData[i].name)) {
      OwningFileOrUSVString* element = aValues.AppendElement();
      *element = mFormData[i].value;
    }
  }
}

bool
FormData::Has(const nsAString& aName)
{
  for (uint32_t i = 0; i < mFormData.Length(); ++i) {
    if (aName.Equals(mFormData[i].name)) {
      return true;
    }
  }

  return false;
}

nsresult
FormData::AddNameFilePair(const nsAString& aName, File* aFile)
{
  MOZ_ASSERT(aFile);

  FormDataTuple* data = mFormData.AppendElement();
  SetNameFilePair(data, aName, aFile);
  return NS_OK;
}

FormData::FormDataTuple*
FormData::RemoveAllOthersAndGetFirstFormDataTuple(const nsAString& aName)
{
  FormDataTuple* lastFoundTuple = nullptr;
  uint32_t lastFoundIndex = mFormData.Length();
  // We have to use this slightly awkward for loop since uint32_t >= 0 is an
  // error for being always true.
  for (uint32_t i = mFormData.Length(); i-- > 0; ) {
    if (aName.Equals(mFormData[i].name)) {
      if (lastFoundTuple) {
        // The one we found earlier was not the first one, we can remove it.
        mFormData.RemoveElementAt(lastFoundIndex);
      }

      lastFoundTuple = &mFormData[i];
      lastFoundIndex = i;
    }
  }

  return lastFoundTuple;
}

void
FormData::Set(const nsAString& aName, Blob& aBlob,
              const Optional<nsAString>& aFilename,
              ErrorResult& aRv)
{
  FormDataTuple* tuple = RemoveAllOthersAndGetFirstFormDataTuple(aName);
  if (tuple) {
    RefPtr<File> file = CreateNewFileInstance(aBlob, aFilename, aRv);
    if (NS_WARN_IF(aRv.Failed())) {
      return;
    }

    SetNameFilePair(tuple, aName, file);
  } else {
    Append(aName, aBlob, aFilename, aRv);
  }
}

void
FormData::Set(const nsAString& aName, const nsAString& aValue,
              ErrorResult& aRv)
{
  FormDataTuple* tuple = RemoveAllOthersAndGetFirstFormDataTuple(aName);
  if (tuple) {
    SetNameValuePair(tuple, aName, aValue);
  } else {
    Append(aName, aValue, aRv);
  }
}

uint32_t
FormData::GetIterableLength() const
{
  return mFormData.Length();
}

const nsAString&
FormData::GetKeyAtIndex(uint32_t aIndex) const
{
  MOZ_ASSERT(aIndex < mFormData.Length());
  return mFormData[aIndex].name;
}

const OwningFileOrUSVString&
FormData::GetValueAtIndex(uint32_t aIndex) const
{
  MOZ_ASSERT(aIndex < mFormData.Length());
  return mFormData[aIndex].value;
}

void
FormData::SetNameValuePair(FormDataTuple* aData,
                           const nsAString& aName,
                           const nsAString& aValue)
{
  MOZ_ASSERT(aData);
  aData->name = aName;
  aData->value.SetAsUSVString() = aValue;
}

void
FormData::SetNameFilePair(FormDataTuple* aData,
                          const nsAString& aName,
                          File* aFile)
{
  MOZ_ASSERT(aData);
  MOZ_ASSERT(aFile);

  aData->name = aName;
  aData->value.SetAsFile() = aFile;
}

// -------------------------------------------------------------------------
// nsIDOMFormData

NS_IMETHODIMP
FormData::Append(const nsAString& aName, nsIVariant* aValue)
{
  uint16_t dataType;
  nsresult rv = aValue->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (dataType == nsIDataType::VTYPE_INTERFACE ||
      dataType == nsIDataType::VTYPE_INTERFACE_IS) {
    nsCOMPtr<nsISupports> supports;
    nsID *iid;
    rv = aValue->GetAsInterface(&iid, getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    moz_free(iid);

    nsCOMPtr<nsIDOMBlob> domBlob = do_QueryInterface(supports);
    RefPtr<Blob> blob = static_cast<Blob*>(domBlob.get());
    if (domBlob) {
      Optional<nsAString> temp;
      ErrorResult rv;
      Append(aName, *blob, temp, rv);
      if (NS_WARN_IF(rv.Failed())) {
        return rv.StealNSResult();
      }

      return NS_OK;
    }
  }

  char16_t* stringData = nullptr;
  uint32_t stringLen = 0;
  rv = aValue->GetAsWStringWithSize(&stringLen, &stringData);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString valAsString;
  valAsString.Adopt(stringData, stringLen);

  ErrorResult error;
  Append(aName, valAsString, error);
  if (NS_WARN_IF(error.Failed())) {
    return error.StealNSResult();
  }

  return NS_OK;
}

/* virtual */ JSObject*
FormData::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return FormDataBinding::Wrap(aCx, this, aGivenProto);
}

/* static */ already_AddRefed<FormData>
FormData::Constructor(const GlobalObject& aGlobal,
                      const Optional<NonNull<HTMLFormElement> >& aFormElement,
                      ErrorResult& aRv)
{
  RefPtr<FormData> formData = new FormData(aGlobal.GetAsSupports());
  if (aFormElement.WasPassed()) {
    aRv = aFormElement.Value().WalkFormElements(formData);
  }
  return formData.forget();
}

// -------------------------------------------------------------------------
// nsIXHRSendable

NS_IMETHODIMP
FormData::GetSendInfo(nsIInputStream** aBody, uint64_t* aContentLength,
                      nsACString& aContentType, nsACString& aCharset)
{
  nsFSMultipartFormData fs(NS_LITERAL_CSTRING("UTF-8"), nullptr);

  for (uint32_t i = 0; i < mFormData.Length(); ++i) {
    if (mFormData[i].value.IsFile()) {
      fs.AddNameFilePair(mFormData[i].name, mFormData[i].value.GetAsFile());
    } else if (mFormData[i].value.IsUSVString()) {
      fs.AddNameValuePair(mFormData[i].name,
                          mFormData[i].value.GetAsUSVString());
    } else {
      MOZ_CRASH("This should no be possible.");
    }
  }

  fs.GetContentType(aContentType);
  aCharset.Truncate();
  *aContentLength = 0;
  NS_ADDREF(*aBody = fs.GetSubmissionBody(aContentLength));

  return NS_OK;
}
