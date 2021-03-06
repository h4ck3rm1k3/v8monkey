/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: sw=2 ts=2 sts=2 expandtab
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is the Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Andrew Sutherland <asutherland@asutherland.org>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef mozilla_storage_IStorageBindingParamsInternal_h_
#define mozilla_storage_IStorageBindingParamsInternal_h_

#include "nsISupports.h"

struct sqlite3_stmt;
class mozIStorageError;

namespace mozilla {
namespace storage {

#define ISTORAGEBINDINGPARAMSINTERNAL_IID \
  {0x4c43d33a, 0xc620, 0x41b8, {0xba, 0x1d, 0x50, 0xc5, 0xb1, 0xe9, 0x1a, 0x04}}

/**
 * Implementation-only interface for mozIStorageBindingParams.  This defines the
 * set of methods required by the asynchronous execution code in order to
 * consume the contents stored in mozIStorageBindingParams instances.
 */
class IStorageBindingParamsInternal : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(ISTORAGEBINDINGPARAMSINTERNAL_IID)

  /**
   * Binds our stored data to the statement.
   *
   * @param aStatement
   *        The statement to bind our data to.
   * @return nsnull on success, or a mozIStorageError object if an error
   *         occurred.
   */
  virtual already_AddRefed<mozIStorageError> bind(sqlite3_stmt *aStatement) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(IStorageBindingParamsInternal,
                              ISTORAGEBINDINGPARAMSINTERNAL_IID)

#define NS_DECL_ISTORAGEBINDINGPARAMSINTERNAL \
  already_AddRefed<mozIStorageError> bind(sqlite3_stmt *aStatement);

} // storage
} // mozilla

#endif // mozilla_storage_IStorageBindingParamsInternal_h_
