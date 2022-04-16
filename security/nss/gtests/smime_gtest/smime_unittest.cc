/* This Source Code Form is subject to the terms of the Mozilla Public
 * License v. 2.0. If a copy of the MPL was not distributed with this file
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "gtest/gtest.h"

#include "scoped_ptrs_smime.h"
#include "smime.h"

namespace nss_test {

// See bug 1507174; this is a CMS serialization (RFC 5652) that claims to be
// 12336 bytes long, which ensures CMS validates the streaming decoder's
// incorrect length.
static const unsigned char kHugeLenAsn1[] = {
    0x30, 0x82, 0x30, 0x30, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7,
    0x0D, 0x01, 0x07, 0x02, 0xA0, 0x82, 0x02, 0x30, 0x30, 0x30, 0x02,
    0x01, 0x30, 0x31, 0x0F, 0x30, 0x0D, 0x06, 0x09, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x30, 0x0B, 0x06,
    0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x05};

// secp256r1 signature with no certs and no attrs
static unsigned char kValidSignature[] = {
    0x30, 0x81, 0xFE, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01,
    0x07, 0x02, 0xA0, 0x81, 0xF0, 0x30, 0x81, 0xED, 0x02, 0x01, 0x01, 0x31,
    0x0F, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04,
    0x02, 0x01, 0x05, 0x00, 0x30, 0x0B, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86,
    0xF7, 0x0D, 0x01, 0x07, 0x01, 0x31, 0x81, 0xC9, 0x30, 0x81, 0xC6, 0x02,
    0x01, 0x01, 0x30, 0x5D, 0x30, 0x45, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03,
    0x55, 0x04, 0x06, 0x13, 0x02, 0x41, 0x55, 0x31, 0x13, 0x30, 0x11, 0x06,
    0x03, 0x55, 0x04, 0x08, 0x0C, 0x0A, 0x53, 0x6F, 0x6D, 0x65, 0x2D, 0x53,
    0x74, 0x61, 0x74, 0x65, 0x31, 0x21, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x04,
    0x0A, 0x0C, 0x18, 0x49, 0x6E, 0x74, 0x65, 0x72, 0x6E, 0x65, 0x74, 0x20,
    0x57, 0x69, 0x64, 0x67, 0x69, 0x74, 0x73, 0x20, 0x50, 0x74, 0x79, 0x20,
    0x4C, 0x74, 0x64, 0x02, 0x14, 0x6B, 0x22, 0xCA, 0x91, 0xE0, 0x71, 0x97,
    0xEB, 0x45, 0x0D, 0x68, 0xC0, 0xD4, 0xB6, 0xE9, 0x45, 0x38, 0x4C, 0xDD,
    0xA3, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04,
    0x02, 0x01, 0x05, 0x00, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE,
    0x3D, 0x04, 0x03, 0x02, 0x04, 0x47, 0x30, 0x45, 0x02, 0x20, 0x48, 0xEB,
    0xE6, 0xBA, 0xFC, 0xFD, 0x83, 0xB3, 0xA2, 0xB5, 0x59, 0x35, 0x0C, 0xA1,
    0x31, 0x0E, 0x2F, 0xE3, 0x8D, 0x81, 0xD8, 0xF5, 0x33, 0xE4, 0x83, 0x87,
    0xB1, 0xFD, 0x43, 0x9D, 0x95, 0x7D, 0x02, 0x21, 0x00, 0xD0, 0x05, 0x0E,
    0x05, 0xA6, 0x80, 0x3C, 0x1A, 0xFE, 0x51, 0xFC, 0x4D, 0x1A, 0x25, 0x05,
    0x78, 0xB5, 0x42, 0xF5, 0xDE, 0x4E, 0x8A, 0xF8, 0xE3, 0xD8, 0x52, 0xDC,
    0x2B, 0x73, 0x80, 0x4A, 0x1A};

// See bug 1507135; this is a CMS signature that contains only the OID
static unsigned char kTruncatedSignature[] = {0x30, 0x0B, 0x06, 0x09, 0x2A,
                                              0x86, 0x48, 0x86, 0xF7, 0x0D,
                                              0x01, 0x07, 0x02};

// secp256r1 signature that's truncated by one byte.
static unsigned char kSlightlyTruncatedSignature[] = {
    0x30, 0x81, 0xFE, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01,
    0x07, 0x02, 0xA0, 0x81, 0xF0, 0x30, 0x81, 0xED, 0x02, 0x01, 0x01, 0x31,
    0x0F, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04,
    0x02, 0x01, 0x05, 0x00, 0x30, 0x0B, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86,
    0xF7, 0x0D, 0x01, 0x07, 0x01, 0x31, 0x81, 0xC9, 0x30, 0x81, 0xC6, 0x02,
    0x01, 0x01, 0x30, 0x5D, 0x30, 0x45, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03,
    0x55, 0x04, 0x06, 0x13, 0x02, 0x41, 0x55, 0x31, 0x13, 0x30, 0x11, 0x06,
    0x03, 0x55, 0x04, 0x08, 0x0C, 0x0A, 0x53, 0x6F, 0x6D, 0x65, 0x2D, 0x53,
    0x74, 0x61, 0x74, 0x65, 0x31, 0x21, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x04,
    0x0A, 0x0C, 0x18, 0x49, 0x6E, 0x74, 0x65, 0x72, 0x6E, 0x65, 0x74, 0x20,
    0x57, 0x69, 0x64, 0x67, 0x69, 0x74, 0x73, 0x20, 0x50, 0x74, 0x79, 0x20,
    0x4C, 0x74, 0x64, 0x02, 0x14, 0x6B, 0x22, 0xCA, 0x91, 0xE0, 0x71, 0x97,
    0xEB, 0x45, 0x0D, 0x68, 0xC0, 0xD4, 0xB6, 0xE9, 0x45, 0x38, 0x4C, 0xDD,
    0xA3, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04,
    0x02, 0x01, 0x05, 0x00, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE,
    0x3D, 0x04, 0x03, 0x02, 0x04, 0x47, 0x30, 0x45, 0x02, 0x20, 0x48, 0xEB,
    0xE6, 0xBA, 0xFC, 0xFD, 0x83, 0xB3, 0xA2, 0xB5, 0x59, 0x35, 0x0C, 0xA1,
    0x31, 0x0E, 0x2F, 0xE3, 0x8D, 0x81, 0xD8, 0xF5, 0x33, 0xE4, 0x83, 0x87,
    0xB1, 0xFD, 0x43, 0x9D, 0x95, 0x7D, 0x02, 0x21, 0x00, 0xD0, 0x05, 0x0E,
    0x05, 0xA6, 0x80, 0x3C, 0x1A, 0xFE, 0x51, 0xFC, 0x4D, 0x1A, 0x25, 0x05,
    0x78, 0xB5, 0x42, 0xF5, 0xDE, 0x4E, 0x8A, 0xF8, 0xE3, 0xD8, 0x52, 0xDC,
    0x2B, 0x73, 0x80, 0x4A};

class SMimeTest : public ::testing::Test {};

TEST_F(SMimeTest, InvalidDER) {
  PK11SymKey* bulk_key = nullptr;
  NSSCMSDecoderContext* dcx =
      NSS_CMSDecoder_Start(nullptr, nullptr, nullptr, /* content callback  */
                           nullptr, nullptr,          /* password callback */
                           nullptr,                   /* key callback      */
                           bulk_key);
  ASSERT_NE(nullptr, dcx);
  EXPECT_EQ(SECSuccess, NSS_CMSDecoder_Update(
                            dcx, reinterpret_cast<const char*>(kHugeLenAsn1),
                            sizeof(kHugeLenAsn1)));
  EXPECT_EQ(nullptr, bulk_key);
  ASSERT_FALSE(NSS_CMSDecoder_Finish(dcx));
}

TEST_F(SMimeTest, IsSignedValid) {
  SECItem sig_der_item = {siBuffer, kValidSignature, sizeof(kValidSignature)};

  ScopedNSSCMSMessage cms_msg(NSS_CMSMessage_CreateFromDER(
      &sig_der_item, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));

  ASSERT_TRUE(cms_msg);

  ASSERT_TRUE(NSS_CMSMessage_IsSigned(cms_msg.get()));
}

TEST_F(SMimeTest, TruncatedCmsSignature) {
  SECItem sig_der_item = {siBuffer, kTruncatedSignature,
                          sizeof(kTruncatedSignature)};

  ScopedNSSCMSMessage cms_msg(NSS_CMSMessage_CreateFromDER(
      &sig_der_item, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));

  ASSERT_TRUE(cms_msg);

  ASSERT_FALSE(NSS_CMSMessage_IsSigned(cms_msg.get()));
}

TEST_F(SMimeTest, SlightlyTruncatedCmsSignature) {
  SECItem sig_der_item = {siBuffer, kSlightlyTruncatedSignature,
                          sizeof(kSlightlyTruncatedSignature)};

  ScopedNSSCMSMessage cms_msg(NSS_CMSMessage_CreateFromDER(
      &sig_der_item, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr));

  ASSERT_FALSE(cms_msg);

  ASSERT_FALSE(NSS_CMSMessage_IsSigned(cms_msg.get()));
}

TEST_F(SMimeTest, IsSignedNull) {
  ASSERT_FALSE(NSS_CMSMessage_IsSigned(nullptr));
}

}  // namespace nss_test