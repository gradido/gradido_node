/*! \mainpage Architecture Overview
*
* @tableofcontents
* \section intro_sec Introduction
*
* The gradido Node is inspired by bitcoin. \n
* The target is getting a node powerful enough to handle in future every transaction between every human on earth. \n
* That means approx. 14 transaction... per ms. \n
* Currently approx. 3.5 billion transactions per day (digital and analog together) exist. \n
* \n
* The world population is divided in regional groups approx. 100 per group. \n
* Groups can have other groups as parent, recursive.\n
* The Node Server has a separate block chain for every group he is listening. \n
* 14 transactions per ms is only valid for a Node Server listening to all groups worldwide. \n
* Most should listening only at some groups but should also operate resource efficiency to run on small and cheap servers \n
* and nevertheless withstanding hacking-attacks. \n
* \n
* Gradido Whitepaper: https://docs.google.com/document/d/1jZp-DiiMPI9ZPNXmjsvOQ1BtnfDFfx8BX7CDmA8KKjY/edit
*
* \section protocol_sec Transaction Protocol
*
* For Gradido transactions google protobuf messages are used, inspired by hedera hashgraph message format. \n 
* It all starting with a transaction. This will later be exchanged with hedera consensus message. \n
* \n
* <code>message Transaction {\n
* &nbsp;uint64 	id = 1; // unique per group, starting with 1, counting upwards\n
* &nbsp;TimestampSeconds received = 2; // world time, received at ordering service, currently community server, later hedera hashgraph\n
* &nbsp;SignatureMap sigMap = 3; // containing signature - pubkey pairs Byte\n
* &nbsp;bytes txHash = 4; // hash for complete transaction\n
* &nbsp;bytes bodyBytes = 5; // containing TransactionBody but serialized, like hedera do it\n
* }</code>\n
* \n
* \n
* <code>message TransactionBody {\n 
* &nbsp;string memo = 1; // max 150 chars, mainly encrypted with shared keys \n 
* &nbsp;TimestampSeconds created = 2; // world time, creation date of transaction\n 
* // specific transactions, not yet complete. \n
* &nbsp;oneof data {\n
* &nbsp;&nbsp;StateCreateGroup createGroup = 6;// the first transaction for a new group\n
* &nbsp;&nbsp;StateGroupChangeParent groupChangeParent = 7;// change group parent\n
* &nbsp;&nbsp;Transfer transfer = 8;// transaction type for sending gradidos from one to another\n
* &nbsp;&nbsp;TransactionCreation creation = 9;// transation type for the gradido creation \n
* &nbsp;}\n
* }</code>\n
*
* Mainly it exist two transaction types: State Changes and Gradido Transaction.\n
* <h3>State Changes:</h3>
* <ul>
* <li>creating or moving a group</li>
* <li>changing group council member</li>
* <li>creating new gradido account</li>
* <li>moving gradido account to another group</li>
* <li>adding hash of email or nickname to gradido account</li>
* <li>removing hash of email or nickname to gradido account</li>
* </ul>
* \n
* <h3>Gradido Transactions:</h3>
* <ul>
* <li>Transfer gradidos from one to another</li>
* <li>Create gradidos through group founder</li>
* <li>Create gradidos through group council members</li>
* </ul>
*
* \section file_formats_sec File Formats
* 
* root folder is ~/.gradido like with bitcoin or other crypto currencies\n 
* first file is group.index\n 
* txt format, created by user\n 
* containing in this format: \n 
* &lt;group public key&gt; = &lt;folder name&gt;\n 
* The Gradido Node create the folder with this name and store every transaction belonging to this group in there\n 
* in the group folder:\n 
* &nbsp;<b>.state: leveldb format, containing:</b>\n 
* <ul>
*  <li>current group states</li>
*  <li>counter for account index (lastKtoIndex)</li>
*  <li>counter for last block id (lastBlockNr)</li>
* </ul>\n 
* &nbsp;<b>pubkeys_&lt;first hash byte hex&gt;</b>\n 
* &nbsp;&nbsp;<b>_&lt;second hash byte hex&gt;.index: binary, unordered, containing:</b>\n 
* <ul>
*  <li>account hash, kto index uint32</li>
*  <li>file hash at end of file</li>
* </ul>
* &nbsp;&nbsp;file is unordered, sorting in memory\n 
* \n
* &nbsp;<b>blk0000000X.index, binary, sorted by kto index, containing:</b>\n 
* <ul>
*  <li>lowest kto index, highest kto index</li>
*  <li>kto index, count, [count X file positions where transaction which contain kto starts]</li>
*  <li>file hash at end of file</li>
* </ul>
* &nbsp;write out after block finished, until then exist only in memory\n
* \n
* &nbsp;<b>blk0000000X.data, binary, sorted by transaction id, containing: </b>\n
* <ul>
*  <li>size, serialized transaction (protobuf format)</li>
*  <li>file hash at end of file</li>
* </ul>
* 
*
* \subsection step1 Step 1: Opening the box
*
* etc...
*/


