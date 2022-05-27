#include "BlockIndex.h"
#include "FileExceptions.h"

#include "Poco/File.h"
#include "../NodeTransactionEntry.h"
#include "../../controller/BlockIndex.h"
//#include "Poco/FileStream.h"

#include "../../SingletonManager/LoggerManager.h"

namespace model {
	namespace files {

		

		void BlockIndex::DataBlock::writeIntoFile(VirtualFile* vFile)
		{
			// first part, write block type, transaction nr and address index count
			// write type
			Block::writeIntoFile(vFile);

			vFile->write(&transactionNr, sizeof(uint64_t));
			vFile->write(&fileCursor, sizeof(int32_t));
			auto coinGroupIdSize = static_cast<uint16_t>(coinGroupId.size());
			vFile->write(&coinGroupIdSize, sizeof(uint16_t));
			if (coinGroupIdSize) {
				vFile->write(coinGroupId.data(), coinGroupId.size() * sizeof(char));
			}			
			vFile->write(&addressIndicesCount, sizeof(uint8_t));
			//vFile->write(this, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint16_t));
			
			// second part, write address indices
			vFile->write(addressIndices, sizeof(uint32_t) * addressIndicesCount);
		}

		bool BlockIndex::DataBlock::readFromFile(VirtualFile* vFile)
		{
			// first part, read block type, transaction nr and address index count
			//if (!vFile->read(this, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint16_t))) return false;;
			if(!vFile->read(&transactionNr, sizeof(uint64_t))) return false;
			if (!vFile->read(&fileCursor, sizeof(int32_t))) return false;
			uint16_t coinGroupIdSize = 0;
			if (!vFile->read(&coinGroupIdSize, sizeof(uint16_t))) return false;
			if (coinGroupIdSize > 400) {
				throw InvalidReadBlockSize("coin group size is to big", "block index file", vFile->getCursor(), coinGroupIdSize);
			}
			if (coinGroupIdSize) {
				coinGroupId.resize(coinGroupIdSize);
				if (!vFile->read(coinGroupId.data(), coinGroupId.size() * sizeof(char))) return false;
			}
			if(!vFile->read(&addressIndicesCount, sizeof(uint8_t))) return false;

			auto addressIndexSize = sizeof(uint32_t) * addressIndicesCount;
			addressIndices = (uint32_t*)malloc(addressIndexSize);

			// second part, read address indices
			return vFile->read(addressIndices, addressIndexSize);
		}

		void BlockIndex::DataBlock::updateHash(crypto_generichash_state* state)
		{
			Block::updateHash(state);
			// first part
			//crypto_generichash_update(state, (const unsigned char*)this, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint16_t));
			crypto_generichash_update(state, (const unsigned char*)&transactionNr, sizeof(uint64_t));		
			crypto_generichash_update(state, (const unsigned char*)&fileCursor, sizeof(int32_t));
			if (coinGroupId.size()) {
				crypto_generichash_update(state, (const unsigned char*)coinGroupId.data(), coinGroupId.size());
			}
			crypto_generichash_update(state, (const unsigned char*)&addressIndicesCount, sizeof(uint8_t));

			// second part
			crypto_generichash_update(state, (const unsigned char*)addressIndices, sizeof(uint32_t) * addressIndicesCount);
		}

		Poco::SharedPtr<NodeTransactionEntry> BlockIndex::DataBlock::createTransactionEntry(uint8_t month, uint16_t year)
		{
			// TransactionEntry(uint64_t transactionNr, int32_t fileCursor, uint8_t month, uint16_t year, uint32_t* addressIndices, uint8_t addressIndiceCount);
			auto transactionEntry = new NodeTransactionEntry(
				transactionNr, month, year, coinGroupId, addressIndices, addressIndicesCount
			);
			transactionEntry->setFileCursor(fileCursor);
			return transactionEntry;
		}

		// **************************************************************************

		BlockIndex::BlockIndex(const Poco::Path& groupFolderPath, Poco::UInt32 blockNr)
			: mDataBlockSumSize(0), mFileName(groupFolderPath)
		{
			char fileName[24]; memset(fileName, 0, 24);
#ifdef _MSC_VER
			sprintf_s(fileName, 24, "blk%08d.index", blockNr);
#else 
			sprintf(fileName, "blk%08d.index", blockNr);
#endif
			mFileName.append(fileName);
			try {
				Poco::File file(mFileName);
				if (!file.exists()) {
					file.createFile();
				}
			}
			catch (Poco::Exception& ex) {
				auto lm = LoggerManager::getInstance();
				std::string functionName = __FUNCTION__;
				lm->mErrorLogging.error("[%s] Poco Exception: %s\n", functionName, ex.displayText());
				//printf("[%s] Poco Exception: %s\n", __FUNCTION__, ex.displayText().data());
			}

		}

		BlockIndex::BlockIndex(const Poco::Path& filename)
			: mDataBlockSumSize(0), mFileName(filename)
		{

		}

		BlockIndex::~BlockIndex()
		{
			reset();
		}

		void BlockIndex::reset()
		{
			while (!mDataBlocks.empty())
			{
				auto block = mDataBlocks.front();
				mDataBlocks.pop();
				delete block;
			}
			mDataBlockSumSize = 0;
		}

		bool BlockIndex::writeToFile()
		{
			auto mm = MemoryManager::getInstance();

			auto hash = mm->getMemory(crypto_generichash_BYTES);

			crypto_generichash_state state;
			crypto_generichash_init(&state, nullptr, 0, crypto_generichash_BYTES);

			VirtualFile vFile(mDataBlockSumSize + crypto_generichash_BYTES + sizeof(uint8_t));

			// maybe has more speed (cache hits) with list and calling update hash in a row, but I think must be profiled and depend on CPU type
			// auto-profiling and strategy choosing?
			while (!mDataBlocks.empty())
			{
				auto block = mDataBlocks.front();
				mDataBlocks.pop();
				block->writeIntoFile(&vFile);
				block->updateHash(&state);
				delete block;
			}

			crypto_generichash_final(&state, *hash, hash->size());
			uint8_t hash_type = HASH_BLOCK;
			vFile.write(&hash_type, sizeof(uint8_t));
			vFile.write((const char*)*hash, hash->size());
			mm->releaseMemory(hash);
			//printf("block index write hash to file: %s\n", DataTypeConverter::binToHex(hash).data());

			return vFile.writeToFile(mFileName.toString(Poco::Path::PATH_NATIVE).data());

		}

		bool BlockIndex::readFromFile(IBlockIndexReceiver* receiver)
		{
			assert(receiver);

			VirtualFile* vFile = VirtualFile::readFromFile(mFileName.toString(Poco::Path::PATH_NATIVE).data());
			if (!vFile || !vFile->getSize()) {
				return false;
			}

			auto mm = MemoryManager::getInstance();
			auto hashFromFile = mm->getMemory(crypto_generichash_BYTES);
			auto hashCalculated = mm->getMemory(crypto_generichash_BYTES);
			crypto_generichash_state state;

			crypto_generichash_init(&state, nullptr, 0, crypto_generichash_BYTES);

			uint8_t monthCursor = 0;
			uint16_t yearCursor = 0;

			while (vFile->getCursor() < vFile->getSize()) 
			{
				uint8_t blockType = NONE_BLOCK;
				Block* block = nullptr;
				// read type (uint8_t)
				vFile->read(&blockType, sizeof(uint8_t));

				switch (blockType) {
				case DATA_BLOCK: block = new DataBlock; break;
				case MONTH_BLOCK: block = new MonthBlock; break;
				case YEAR_BLOCK: block = new YearBlock; break;
				case HASH_BLOCK: vFile->read(*hashFromFile, crypto_generichash_BYTES); break;
				}
				// read block
				if (block) {
					// file end reached
					if (!block->readFromFile(vFile)) {
						break;
					}
					block->updateHash(&state);
					// call receiver
					if (YEAR_BLOCK == blockType) {
						yearCursor = static_cast<YearBlock*>(block)->year;
					}
					else if (MONTH_BLOCK == blockType) {
						monthCursor = static_cast<MonthBlock*>(block)->month;
					}
					else if (DATA_BLOCK == blockType) {
						auto dataBlock = static_cast<DataBlock*>(block);
						//receiver->addIndicesForTransaction(dataBlock->createTransactionEntry(monthCursor, yearCursor));
						//bool addIndicesForTransaction(uint16_t year, uint8_t month, uint64_t transactionNr, const std::vector<uint32_t>& addressIndices);
						receiver->addIndicesForTransaction(
							dataBlock->coinGroupId,
							yearCursor, monthCursor, 
							dataBlock->transactionNr, dataBlock->fileCursor, 
							dataBlock->addressIndices, dataBlock->addressIndicesCount
						);
					}

				}
				// if block is nullptr, means hash found and file should be end, ignore data after hash
				else {
					break;
				}
			}
			crypto_generichash_final(&state, *hashCalculated, hashCalculated->size());

			int hashCompareResult = sodium_compare(*hashFromFile, *hashCalculated, hashCalculated->size());
			// TODO: use RAII
			if (hashCompareResult) {
				HashMismatchException exception("block index file hash mismatch", hashFromFile, hashCalculated);
				mm->releaseMemory(hashFromFile);
				mm->releaseMemory(hashCalculated);
				delete vFile;
				throw exception;
			}
			else {
				mm->releaseMemory(hashFromFile);
				mm->releaseMemory(hashCalculated);
				delete vFile;
			}

			return 0 == hashCompareResult;
		}

		std::unique_ptr<VirtualFile> BlockIndex::serialize()
		{
			auto mm = MemoryManager::getInstance();
			auto hash = mm->getMemory(crypto_generichash_BYTES);

			crypto_generichash_state state;
			crypto_generichash_init(&state, nullptr, 0, crypto_generichash_BYTES);

			auto vFile = std::make_unique<VirtualFile>(mDataBlockSumSize + crypto_generichash_BYTES + sizeof(uint8_t));

			// maybe has more speed (cache hits) with list and calling update hash in a row, but I think must be profiled and depend on CPU type
			// auto-profiling and strategy choosing?
			while (!mDataBlocks.empty())
			{
				auto block = mDataBlocks.front();
				mDataBlocks.pop();
				block->writeIntoFile(vFile.get());
				block->updateHash(&state);
				delete block;
			}

			crypto_generichash_final(&state, *hash, hash->size());
			uint8_t hash_type = HASH_BLOCK;
			vFile->write(&hash_type, sizeof(uint8_t));
			vFile->write((const char*)*hash, hash->size());
			mm->releaseMemory(hash);
			//printf("block index write hash to file: %s\n", DataTypeConverter::binToHex(hash).data());

			return std::move(vFile);
		}
	}

}