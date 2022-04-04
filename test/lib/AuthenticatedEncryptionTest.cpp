#include "AuthenticatedEncryptionTest.h"
#include "gradido_blockchain/crypto/AuthenticatedEncryption.h"
#include "gradido_blockchain/lib/DataTypeConverter.h"

const char* testMessage1 = "Hallo Welt";

TEST(AuthenticatedEncryptionTest, EncryptDecryptMessage)
{
	auto mm = MemoryManager::getInstance();
	auto encryptionKeyPair1 = new AuthenticatedEncryption();
	auto encryptionKeyPair2 = new AuthenticatedEncryption();
	
	auto message = mm->getMemory(strlen(testMessage1)+1);
	memcpy(*message, testMessage1, message->size());
	auto encryptedMessage = encryptionKeyPair1->encrypt(message, encryptionKeyPair2);
	ASSERT_NE(encryptedMessage, nullptr);
	ASSERT_EQ(encryptedMessage->size(), message->size() + crypto_box_NONCEBYTES + crypto_box_MACBYTES);
	//printf("encrypted: %s\n", DataTypeConverter::binToHex(encryptedMessage).data());
	auto decryptedMessage = encryptionKeyPair2->decrypt(encryptedMessage, encryptionKeyPair1);
	ASSERT_NE(decryptedMessage, nullptr);
	ASSERT_EQ(decryptedMessage->size(), message->size());

	EXPECT_EQ(memcmp(*message, *decryptedMessage, message->size()), 0);

	mm->releaseMemory(message);
	mm->releaseMemory(encryptedMessage);
	mm->releaseMemory(decryptedMessage);
	delete encryptionKeyPair1;
	delete encryptionKeyPair2;
}

TEST(AuthenticatedEncryptionTest, PrecalculateSecret)
{
	auto mm = MemoryManager::getInstance();
	auto encryptionKeyPair = new AuthenticatedEncryption();
	const int recipiantKeyCount = 5;
	AuthenticatedEncryption recipiantKeys[recipiantKeyCount];
	for (int i = 0; i < recipiantKeyCount; i++) {
		EXPECT_EQ(encryptionKeyPair->precalculateSharedSecret(&recipiantKeys[i]), i+1);
	}
	// remove work
	EXPECT_TRUE(encryptionKeyPair->removePrecalculatedSharedSecret(3));
	// after it was removed additional call to remove return false
	EXPECT_FALSE(encryptionKeyPair->removePrecalculatedSharedSecret(3));

	EXPECT_EQ(encryptionKeyPair->precalculateSharedSecret(&recipiantKeys[4]), recipiantKeyCount+1);
}

TEST(AuthenticatedEncryptionTest, EncryptPrecalculatedSecretDecryptMessage)
{
	auto mm = MemoryManager::getInstance();
	auto encryptionKeyPair1 = new AuthenticatedEncryption();
	auto encryptionKeyPair2 = new AuthenticatedEncryption();

	auto index = encryptionKeyPair1->precalculateSharedSecret(encryptionKeyPair2);

	auto message = mm->getMemory(strlen(testMessage1) + 1);
	memcpy(*message, testMessage1, message->size());
	auto encryptedMessage = encryptionKeyPair1->encrypt(message, index);
	ASSERT_NE(encryptedMessage, nullptr);
	ASSERT_EQ(encryptedMessage->size(), message->size() + crypto_box_NONCEBYTES + crypto_box_MACBYTES);
	//printf("encrypted: %s\n", DataTypeConverter::binToHex(encryptedMessage).data());
	auto decryptedMessage = encryptionKeyPair2->decrypt(encryptedMessage, encryptionKeyPair1);
	ASSERT_NE(decryptedMessage, nullptr);
	ASSERT_EQ(decryptedMessage->size(), message->size());

	EXPECT_EQ(memcmp(*message, *decryptedMessage, message->size()), 0);

	mm->releaseMemory(message);
	mm->releaseMemory(encryptedMessage);
	mm->releaseMemory(decryptedMessage);
	delete encryptionKeyPair1;
	delete encryptionKeyPair2;
}

TEST(AuthenticatedEncryptionTest, EncryptDecryptPrecalculatedSecretMessage)
{
	auto mm = MemoryManager::getInstance();
	auto encryptionKeyPair1 = new AuthenticatedEncryption();
	auto encryptionKeyPair2 = new AuthenticatedEncryption();

	auto index = encryptionKeyPair2->precalculateSharedSecret(encryptionKeyPair1);

	auto message = mm->getMemory(strlen(testMessage1) + 1);
	memcpy(*message, testMessage1, message->size());
	auto encryptedMessage = encryptionKeyPair1->encrypt(message, encryptionKeyPair2);
	ASSERT_NE(encryptedMessage, nullptr);
	ASSERT_EQ(encryptedMessage->size(), message->size() + crypto_box_NONCEBYTES + crypto_box_MACBYTES);
	//printf("encrypted: %s\n", DataTypeConverter::binToHex(encryptedMessage).data());
	auto decryptedMessage = encryptionKeyPair2->decrypt(encryptedMessage, index);
	ASSERT_NE(decryptedMessage, nullptr);
	ASSERT_EQ(decryptedMessage->size(), message->size());

	EXPECT_EQ(memcmp(*message, *decryptedMessage, message->size()), 0);

	mm->releaseMemory(message);
	mm->releaseMemory(encryptedMessage);
	mm->releaseMemory(decryptedMessage);
	delete encryptionKeyPair1;
	delete encryptionKeyPair2;
}