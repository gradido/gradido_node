#include "BlockIndex.h"
#include "FileExceptions.h"

#include "../../blockchain/NodeTransactionEntry.h"
#include "../../blockchain/FileBasedProvider.h"
#include "../../cache/BlockIndex.h"
#include "../../SingletonManager/FileLockManager.h"

#include "loguru/loguru.hpp"
#include "magic_enum/magic_enum.hpp"

#include <fstream>
#include <filesystem>

using namespace gradido;
using namespace blockchain;
using namespace data;
using namespace magic_enum;

namespace model {
	namespace files {

		void BlockIndex::DataBlock::writeIntoFile(VirtualFile* vFile)
		{
			// first part, write block type, transaction nr and address index count
			// write type
			Block::writeIntoFile(vFile);

			vFile->write(&transactionNr, sizeof(uint64_t));
			vFile->write(&fileCursor, sizeof(int32_t));
			vFile->write(&transactionType, sizeof(TransactionType));
			vFile->write(&coinCommunityIdIndex, sizeof(uint32_t));
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
			if(!vFile->read(&fileCursor, sizeof(int32_t))) return false;
			if(!vFile->read(&transactionType, sizeof(TransactionType))) return false;
			if (!enum_contains<TransactionType>(transactionType)) {
				throw GradidoUnknownEnumException(
					"invalid transaction type readed from block index",
					"gradido::data::TransactionType", 
					std::to_string((uint8_t)transactionType).data()
				);
			}
			if(!vFile->read(&coinCommunityIdIndex, sizeof(uint32_t))) return false;
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
			crypto_generichash_update(state, (const unsigned char*)&coinCommunityIdIndex, sizeof(uint32_t));
			crypto_generichash_update(state, (const unsigned char*)&addressIndicesCount, sizeof(uint8_t));

			// second part
			crypto_generichash_update(state, (const unsigned char*)addressIndices, sizeof(uint32_t) * addressIndicesCount);
		}

		std::shared_ptr<blockchain::NodeTransactionEntry> BlockIndex::DataBlock::createTransactionEntry(date::month month, date::year year)
		{
			auto coinCommunityId = FileBasedProvider::getInstance()->getCommunityIdString(coinCommunityIdIndex);
			// TransactionEntry(uint64_t transactionNr, int32_t fileCursor, uint8_t month, uint16_t year, uint32_t* addressIndices, uint8_t addressIndiceCount);
			auto transactionEntry = std::make_shared<blockchain::NodeTransactionEntry>(
				transactionNr, month, year, transactionType, coinCommunityId, addressIndices, addressIndicesCount
			);
			transactionEntry->setFileCursor(fileCursor);
			return transactionEntry;
		}

		// **************************************************************************

		BlockIndex::BlockIndex(std::string_view groupFolderPath, uint32_t blockNr)
			: mDataBlockSumSize(0), mFileName(groupFolderPath)
		{
			std::stringstream fileNameStream;
			fileNameStream << "/blk" << std::setw(8) << std::setfill('0') << blockNr << ".index";
			// sprintf(fileName, "blk%08d.index", blockNr);

			mFileName.append(fileNameStream.str());
			
			// create file if not exist
			std::ofstream file(mFileName, std::ofstream::binary);
			if(file.fail()) {
				LOG_F(ERROR, "could not create block index file: %s", mFileName.data());
			}

		}

		BlockIndex::BlockIndex(std::string_view filename)
			: mDataBlockSumSize(0), mFileName(filename)
		{

		}

		BlockIndex::~BlockIndex()
		{
			exit();
		}

		void BlockIndex::exit()
		{
			while (!mDataBlocks.empty())
			{
				auto block = mDataBlocks.front();
				mDataBlocks.pop();
				delete block;
			}
			mDataBlockSumSize = 0;
		}

		void BlockIndex::reset()
		{
			auto fl = FileLockManager::getInstance();
			int timeoutRounds = 100;
			if (!fl->tryLockTimeout(mFileName, timeoutRounds)) {
				throw LockException("cannot lock file for reset", mFileName);
			}
			std::filesystem::remove(mFileName);
			fl->unlock(mFileName);
		}

		bool BlockIndex::writeToFile()
		{
			memory::Block hash(crypto_generichash_BYTES);

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

			crypto_generichash_final(&state, hash, hash.size());
			uint8_t hash_type = HASH_BLOCK;
			vFile.write(&hash_type, sizeof(uint8_t));
			vFile.write((const char*)*hash, hash.size());
			//printf("block index write hash to file: %s\n", DataTypeConverter::binToHex(hash).data());

			return vFile.writeToFile(mFileName.data());

		}

		bool BlockIndex::readFromFile(IBlockIndexReceiver* receiver)
		{
			assert(receiver);

			VirtualFile* vFile = VirtualFile::readFromFile(mFileName.data());
			if (!vFile || !vFile->getSize()) {
				return false;
			}

			unsigned char hashFromFile[crypto_generichash_BYTES];
			unsigned char hashCalculated[crypto_generichash_BYTES];
			crypto_generichash_state state;

			crypto_generichash_init(&state, nullptr, 0, crypto_generichash_BYTES);

			date::month monthCursor(0);
			date::year yearCursor(0);

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
				case HASH_BLOCK: vFile->read(hashFromFile, crypto_generichash_BYTES); break;
				}
				int32_t debugFileCursor = 0;
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
						if(dataBlock->fileCursor < debugFileCursor) {
							LOG_F(ERROR, "error reading blockindex from file, file cursor is: %d, last value was: %d",
								dataBlock->fileCursor,
								debugFileCursor
							);
							throw std::runtime_error("corrupted index file?");
						}
						debugFileCursor = dataBlock->fileCursor;
						receiver->addIndicesForTransaction(
							dataBlock->transactionType,
							dataBlock->coinCommunityIdIndex,
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
			delete vFile;
			crypto_generichash_final(&state, hashCalculated, sizeof(hashCalculated));

			int hashCompareResult = sodium_compare(hashFromFile, hashCalculated, sizeof(hashCalculated));
			if (hashCompareResult) {
				memory::Block hashFromFileBlock(sizeof(hashFromFile), hashFromFile);
				memory::Block hashCalculatedBlock(sizeof(hashCalculated), hashCalculated);
				throw HashMismatchException("block index file hash mismatch", hashFromFileBlock, hashCalculatedBlock);
			}
			return 0 == hashCompareResult;
		}

		std::unique_ptr<VirtualFile> BlockIndex::serialize()
		{
			unsigned char hash[crypto_generichash_BYTES];

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

			crypto_generichash_final(&state, hash, sizeof(hash));
			uint8_t hash_type = HASH_BLOCK;
			vFile->write(&hash_type, sizeof(uint8_t));
			vFile->write((const char*)hash, sizeof(hash));
			//printf("block index write hash to file: %s\n", DataTypeConverter::binToHex(hash).data());

			return std::move(vFile);
		}
	}

}