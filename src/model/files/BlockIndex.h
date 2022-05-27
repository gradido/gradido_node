#ifndef __GRADIDO_NODE_MODEL_FILES_BLOCK_INDEX_H
#define __GRADIDO_NODE_MODEL_FILES_BLOCK_INDEX_H


#include "FileBase.h"

#include "../../lib/VirtualFile.h"

#include "Poco/SharedPtr.h"

#include "sodium.h"
#include <queue>
#include <string>

namespace controller {
	class BlockIndex;
}

namespace model {

	class NodeTransactionEntry;

	namespace files {

		class IBlockIndexReceiver 
		{
		public:
			virtual bool addIndicesForTransaction(const std::string& coinGroupId, uint16_t year, uint8_t month, uint64_t transactionNr, int32_t fileCursor, const uint32_t* addressIndices, uint8_t addressIndiceCount) = 0;
		};

		/*!
		 * @author Dario Rekowski
		 * @date 2020-02-11
		 * @brief Store Block Index Data in binary file
		 * 
		 * Store block by block: TransactionNr, address indices
		 * make groups with month and year 
		 * three types of blocks: year, month, data
		 * and last block: file hash
		*/		

		class BlockIndex : public FileBase
		{
		public:
			//! create filename from path and blocknr
			BlockIndex(const Poco::Path& groupFolderPath, Poco::UInt32 blockNr);
			//! use full filename which includes also the block nr
			BlockIndex(const Poco::Path& filename);
			~BlockIndex();

			inline void addMonthBlock(uint8_t month) {
				mDataBlocks.push(new MonthBlock(month)); 
				mDataBlockSumSize += mDataBlocks.back()->size();
			}
			inline void addYearBlock(uint16_t year) { 
				mDataBlocks.push(new YearBlock(year)); 
				mDataBlockSumSize += mDataBlocks.back()->size();
			}
			inline void addDataBlock(uint64_t transactionNr, int32_t fileCursor, const std::string& coinGroupId, const std::vector<uint32_t>& addressIndices) {
				mDataBlocks.push(new DataBlock(transactionNr, fileCursor, coinGroupId, addressIndices));
				mDataBlockSumSize += mDataBlocks.back()->size();
			}
	
			//! \brief read from file, put content into receiver
			//! \return true if hash in file and calculated hash are the same
			//! \return false if file not found or couldn't opened
			bool readFromFile(IBlockIndexReceiver* receiver);

			//! called from SerializeBlockIndexTask once in a while (after hdd write timeout)
			//! put blocks into binary file format
			std::unique_ptr<VirtualFile> serialize();

			bool BlockIndex::writeToFile();

			//! \brief clear data blocks
			void reset();

			inline const Poco::Path& getFileName() { return mFileName; }

		protected:
			//! \brief replace Index File with new one, clear blocks after writing into file
			
			

			enum BlockTypes {
				YEAR_BLOCK = 0xad,
				MONTH_BLOCK = 0x50,
				DATA_BLOCK = 0xdd,
				HASH_BLOCK = 0x17,
				NONE_BLOCK = 0
			};
			struct Block {
				Block(uint8_t _type) : type(_type) {}
				virtual ~Block() {};
				uint8_t type;
				virtual size_t size() = 0;

				virtual void writeIntoFile(VirtualFile* vFile) {
					//vFile->write(this, size());
					vFile->write(&type, sizeof(uint8_t));
				};
				//! return false if reading failed, file to short?
				virtual bool readFromFile(VirtualFile* vFile) {
					//return vFile->read(this, size());
					return vFile->read(&type, sizeof(uint8_t));
				}

				virtual void updateHash(crypto_generichash_state* state) {
					//crypto_generichash_update(state, (const unsigned char*)this, size());
					crypto_generichash_update(state, (const unsigned char*)&type, sizeof(uint8_t));
				}
			};
			struct YearBlock : public Block {
				YearBlock(uint16_t _year) : Block(YEAR_BLOCK), year(_year) {}
				YearBlock() : Block(YEAR_BLOCK), year(0) {}
				uint16_t year;
				size_t size() { return sizeof(uint8_t) + sizeof(uint16_t); }

				bool readFromFile(VirtualFile* vFile) {
					return vFile->read(&year, sizeof(uint16_t));
				}

				void writeIntoFile(VirtualFile* vFile) {
					Block::writeIntoFile(vFile);
					vFile->write(&year, sizeof(uint16_t));
				}

				void updateHash(crypto_generichash_state* state) {
					//crypto_generichash_update(state, (const unsigned char*)this, size());
					Block::updateHash(state);
					crypto_generichash_update(state, (const unsigned char*)&year, sizeof(uint16_t));
				}
			};
			struct MonthBlock: public Block {
				MonthBlock(uint8_t _month) : Block(MONTH_BLOCK), month(_month) {}
				MonthBlock() : Block(MONTH_BLOCK), month(0) {}
				uint8_t month;
				size_t size() { return sizeof(uint8_t) * 2; }

				bool readFromFile(VirtualFile* vFile) {
					return vFile->read(&month, sizeof(uint8_t));
				}

				void writeIntoFile(VirtualFile* vFile) {
					Block::writeIntoFile(vFile);
					vFile->write(&month, sizeof(uint8_t));
				}

				void updateHash(crypto_generichash_state* state) {
					//crypto_generichash_update(state, (const unsigned char*)this, size());
					Block::updateHash(state);
					crypto_generichash_update(state, (const unsigned char*)&month, sizeof(uint8_t));
				}
			};
			struct DataBlock: public Block {
				DataBlock(uint64_t _transactionNr, int32_t _fileCursor, const std::string& _coinGroupId, const std::vector<uint32_t>& _addressIndices)
					: Block(DATA_BLOCK), transactionNr(_transactionNr), fileCursor(_fileCursor), coinGroupId(_coinGroupId), addressIndices(nullptr), addressIndicesCount(_addressIndices.size())
				{
					addressIndices = (uint32_t*)malloc(addressIndicesCount * sizeof(uint32_t));
					memcpy(addressIndices, _addressIndices.data(), addressIndicesCount * sizeof(uint32_t));
				}
				DataBlock()
					: Block(DATA_BLOCK), transactionNr(0), fileCursor(-10), addressIndices(nullptr), addressIndicesCount(0)
				{

				}

				~DataBlock()
				{
					free(addressIndices);
					addressIndices = nullptr;
					addressIndicesCount = 0;
					fileCursor = 0;
				}
				uint64_t transactionNr;
				int32_t fileCursor;
				std::string coinGroupId;
				uint8_t  addressIndicesCount;
				uint32_t* addressIndices;
				size_t size() { 
					return sizeof(uint8_t) + sizeof(uint64_t) + sizeof(int32_t) + sizeof(uint16_t) + coinGroupId.size() + sizeof(uint8_t) + sizeof(uint32_t) * addressIndicesCount; 
				}

				virtual void writeIntoFile(VirtualFile* vFile);
				virtual bool readFromFile(VirtualFile* vFile);
				virtual void updateHash(crypto_generichash_state* state);

				Poco::SharedPtr<NodeTransactionEntry> createTransactionEntry(uint8_t month, uint16_t year);
			};

			Poco::Path  mFileName;
			std::queue<Block*> mDataBlocks;
			size_t mDataBlockSumSize;
		};
	}
}



#endif //__GRADIDO_NODE_MODEL_FILES_BLOCK_INDEX_H