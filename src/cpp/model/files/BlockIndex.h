#ifndef __GRADIDO_NODE_MODEL_FILES_BLOCK_INDEX_H
#define __GRADIDO_NODE_MODEL_FILES_BLOCK_INDEX_H

#include "FileBase.h"
#include "Poco/FileStream.h"

namespace controller {
	class BlockIndex;
}

namespace model {
	namespace files {

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
			BlockIndex(const std::string& filename);
			~BlockIndex();

			inline void addMonthBlock(uint8_t month) {mDataBlocks.push(new MonthBlock(month));}
			inline void addYearBlock(uint16_t year) { mDataBlocks.push(new YearBlock(year)); }
			inline void addDataBlock(uint64_t transactionNr, const std::vector<uint32_t>& addressIndices) {
				mDataBlocks.push(new DataBlock(transactionNr, addressIndices));
			}

			bool writeToFile();
			bool readFromFile(controller::BlockIndex* receiver);

		protected:
			enum BlockTypes {
				YEAR_BLOCK = 0xad,
				MONTH_BLOCK = 0x50,
				DATA_BLOCK = 0xdd
			};
			struct Block {
				Block(uint8_t _type) : type(_type) {}
				virtual ~Block();
				uint8_t type;
				virtual size_t size() = 0;
			};
			struct YearBlock : public Block {
				YearBlock(uint16_t _year) : Block(YEAR_BLOCK), year(_year) {}
				uint16_t year;
				size_t size() { return sizeof(uint8_t) + sizeof(uint16_t); }
			};
			struct MonthBlock: public Block {
				MonthBlock(uint8_t _month) : Block(MONTH_BLOCK), month(_month) {}
				uint8_t month;
				size_t size() { return sizeof(uint8_t) * 2; }
			};
			struct DataBlock: public Block {
				DataBlock(uint64_t _transactionNr, const std::vector<uint32_t>& _addressIndices)
					: Block(DATA_BLOCK), transactionNr(_transactionNr), addressIndices(nullptr), addressIndicesCount(_addressIndices.size())
				{
					addressIndices = (uint32_t*)malloc(addressIndicesCount * sizeof(uint32_t));
					memcpy(addressIndices, _addressIndices.data(), addressIndicesCount * sizeof(uint32_t));
				}
				~DataBlock()
				{
					free(addressIndices);
					addressIndices = nullptr;
					addressIndicesCount = 0;
				}
				uint64_t transactionNr;
				uint8_t  addressIndicesCount;
				uint32_t* addressIndices;
				size_t size() { return sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint32_t) * addressIndicesCount; }
			};

			std::string mFilename;
			std::queue<Block*> mDataBlocks;
		};
	}
}

#endif //__GRADIDO_NODE_MODEL_FILES_BLOCK_INDEX_H