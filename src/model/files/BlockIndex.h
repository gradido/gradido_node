#ifndef __GRADIDO_NODE_MODEL_FILES_BLOCK_INDEX_H
#define __GRADIDO_NODE_MODEL_FILES_BLOCK_INDEX_H

#include "gradido_blockchain/data/TransactionType.h"

#include "../../lib/VirtualFile.h"

#include "date/date.h"

#include <queue>
#include <string>

namespace cache {
	class BlockIndex;
}

namespace gradido {
	namespace blockchain {
		class NodeTransactionEntry;
	}
}
namespace model {

	namespace files {

		class IBlockIndexReceiver 
		{
		public:
			virtual bool addIndicesForTransaction(
				gradido::data::TransactionType transactionType,
				uint32_t coinCommunityIdIndex, 
				date::year year,
				date::month month,
				uint64_t transactionNr, 
				int32_t fileCursor, 
				const uint32_t* addressIndices, 
				uint8_t addressIndiceCount
			) = 0;
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

		class BlockIndex
		{
		public:
			//! create filename from path and blocknr
			BlockIndex(std::string_view groupFolderPath, uint32_t blockNr);
			//! use full filename which includes also the block nr
			BlockIndex(std::string_view filename);
			~BlockIndex();

			inline void addMonthBlock(date::month month) {
				mDataBlocks.push(new MonthBlock(month)); 
				mDataBlockSumSize += mDataBlocks.back()->size();
			}
			inline void addYearBlock(date::year year) { 
				mDataBlocks.push(new YearBlock(year)); 
				mDataBlockSumSize += mDataBlocks.back()->size();
			}
			inline void addDataBlock(
				uint64_t transactionNr,
				int32_t fileCursor,
				gradido::data::TransactionType transactionType,
				uint32_t coinCommunityIdIndex,
				const std::vector<uint32_t>& addressIndices
			) {
				mDataBlocks.push(new DataBlock(transactionNr, fileCursor, transactionType, coinCommunityIdIndex, addressIndices));
				mDataBlockSumSize += mDataBlocks.back()->size();
			}
	
			//! \brief read from file, put content into receiver
			//! \return true if hash in file and calculated hash are the same
			//! \return false if file not found or couldn't opened
			bool readFromFile(IBlockIndexReceiver* receiver);

			//! called from SerializeBlockIndexTask once in a while (after hdd write timeout)
			//! put blocks into binary file format
			std::unique_ptr<VirtualFile> serialize();

			void writeToFile();

			//! \brief clear data blocks
			void exit();
			//! remove index file
			void reset();

			inline const std::string& getFileName() { return mFileName; }

			static void removeAllBlockIndexFiles(std::string_view groupFolderPath);

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
				YearBlock(date::year _year) : Block(YEAR_BLOCK), year(_year) {}
				YearBlock() : Block(YEAR_BLOCK), year(0) {}
				date::year year;
				size_t size() { return sizeof(uint8_t) + sizeof(date::year); }

				bool readFromFile(VirtualFile* vFile) {
					return vFile->read(&year, sizeof(date::year));
				}

				void writeIntoFile(VirtualFile* vFile) {
					Block::writeIntoFile(vFile);
					vFile->write(&year, sizeof(date::year));
				}

				void updateHash(crypto_generichash_state* state) {
					//crypto_generichash_update(state, (const unsigned char*)this, size());
					Block::updateHash(state);
					crypto_generichash_update(state, (const unsigned char*)&year, sizeof(date::year));
				}
			};
			struct MonthBlock: public Block {
				MonthBlock(date::month _month) : Block(MONTH_BLOCK), month(_month) {}
				MonthBlock() : Block(MONTH_BLOCK), month(0) {}
				date::month month;
				size_t size() { return sizeof(uint8_t) + sizeof(date::month); }

				bool readFromFile(VirtualFile* vFile) {
					return vFile->read(&month, sizeof(date::month));
				}

				void writeIntoFile(VirtualFile* vFile) {
					Block::writeIntoFile(vFile);
					vFile->write(&month, sizeof(date::month));
				}

				void updateHash(crypto_generichash_state* state) {
					//crypto_generichash_update(state, (const unsigned char*)this, size());
					Block::updateHash(state);
					crypto_generichash_update(state, (const unsigned char*)&month, sizeof(date::month));
				}
			};
			struct DataBlock: public Block {
				DataBlock(
					uint64_t _transactionNr, 
					int32_t _fileCursor, 
					gradido::data::TransactionType _transactionType,
					uint32_t _coinCommunityIdIndex,
					const std::vector<uint32_t>& _addressIndices
				) : 
					Block(DATA_BLOCK), 
					transactionNr(_transactionNr), 
					fileCursor(_fileCursor), 
					transactionType(_transactionType),
					coinCommunityIdIndex(_coinCommunityIdIndex),
					addressIndices(nullptr), 
					addressIndicesCount(_addressIndices.size())
				{
					addressIndices = (uint32_t*)malloc(addressIndicesCount * sizeof(uint32_t));
					assert(addressIndices);
					memcpy(addressIndices, _addressIndices.data(), addressIndicesCount * sizeof(uint32_t));
				}
				DataBlock()
					: 
					Block(DATA_BLOCK), 
					transactionNr(0), 
					fileCursor(-10), 
					transactionType(gradido::data::TransactionType::NONE), 
					coinCommunityIdIndex(0),
					addressIndices(nullptr), 
					addressIndicesCount(0)
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
				gradido::data::TransactionType transactionType;
				uint32_t coinCommunityIdIndex;
				uint8_t  addressIndicesCount;
				uint32_t* addressIndices;
				size_t size() { 
					return 
						  sizeof(uint8_t)  // Block Type
						+ sizeof(uint64_t) // transaction nr
						+ sizeof(int32_t)  // fileCursor
						+ sizeof(gradido::data::TransactionType) // transaction type
						+ sizeof(uint32_t) // coin community id index size
						+ sizeof(uint8_t) + sizeof(uint32_t) * addressIndicesCount; // address index count, address indices array
				}

				virtual void writeIntoFile(VirtualFile* vFile);
				virtual bool readFromFile(VirtualFile* vFile);
				virtual void updateHash(crypto_generichash_state* state);

				std::shared_ptr<gradido::blockchain::NodeTransactionEntry> createTransactionEntry(date::month month, date::year year);
			};

			std::string mFileName;
			std::queue<Block*> mDataBlocks;
			size_t mDataBlockSumSize;
		};
	}
}



#endif //__GRADIDO_NODE_MODEL_FILES_BLOCK_INDEX_H