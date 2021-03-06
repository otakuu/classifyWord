#include "parse.h"
#include <string.h>

#ifndef DEBUG
#define DEBUG
#endif

#include "strconv.h"


uint8_t cfb_header_signature[] =
	{ 0xd0, 0xcf, 0x11, 0xe0, 0xa1, 0xb1, 0x1a, 0xe1 };

uint16_t cfb_direntry_1table[] =
	{ htoles(0x0031), htoles(0x0054), htoles(0x0061),
	  htoles(0x0062), htoles(0x006c), htoles(0x0065),
	  htoles(0x0000) };	// "1Table\0" in UTF16, LE

uint16_t cfb_direntry_0table[] =
	{ htoles(0x0030), htoles(0x0054), htoles(0x0061),
	  htoles(0x0062), htoles(0x006c), htoles(0x0065),
	  htoles(0x0000) };	// "0Table\0" in UTF16, LE

uint16_t cfb_direntry_data[] =
	{ htoles(0x0044), htoles(0x0061), htoles(0x0074),
	  htoles(0x0061), htoles(0x0000) };	// "Data\0" in UTF16, LE

uint16_t cfb_direntry_worddoc[] =
	{ htoles(0x0057), htoles(0x006f), htoles(0x0072),
	  htoles(0x0064), htoles(0x0044), htoles(0x006f),
	  htoles(0x0063), htoles(0x0075), htoles(0x006d),
	  htoles(0x0065), htoles(0x006e), htoles(0x0074),
	  htoles(0x0000) };	// "WordDocument\0" in UTF16, LE
typedef struct {
  uint8_t  direntry_name_len;
  uint16_t* direntry_name;
  uint32_t absolute_file_offset;
} stream_information_t;

/* streams of interest */
stream_information_t streams[4] = {
    { sizeof(cfb_direntry_0table)/sizeof(uint16_t),cfb_direntry_0table, 0 },
    { sizeof(cfb_direntry_1table)/sizeof(uint16_t),cfb_direntry_1table, 0 },
    { sizeof(cfb_direntry_data)/sizeof(uint16_t),cfb_direntry_data, 0 },
    { sizeof(cfb_direntry_worddoc)/sizeof(uint16_t),cfb_direntry_worddoc, 0 }
};

typedef struct
{
	uint32_t  MaxKeySize;
	uint8_t  *pCspName;
	uint32_t  CspNameLength;
} csp_name_to_keysize_map_t;

uint8_t csp_name_ms_base_csp[] =
	{
		0x4d, 0x00, 0x69, 0x00, 0x63, 0x00, 0x72, 0x00,
		0x6f, 0x00, 0x73, 0x00, 0x6f, 0x00, 0x66, 0x00,
		0x74, 0x00, 0x20, 0x00, 0x42, 0x00, 0x61, 0x00,
		0x73, 0x00, 0x65, 0x00, 0x20, 0x00, 0x43, 0x00,
		0x72, 0x00, 0x79, 0x00, 0x70, 0x00, 0x74, 0x00,
		0x6f, 0x00, 0x67, 0x00, 0x72, 0x00, 0x61, 0x00,
		0x70, 0x00, 0x68, 0x00, 0x69, 0x00, 0x63, 0x00,
		0x20, 0x00, 0x50, 0x00, 0x72, 0x00, 0x6f, 0x00,
		0x76, 0x00, 0x69, 0x00, 0x64, 0x00, 0x65, 0x00,
		0x72, 0x00, 0x20, 0x00, 0x76, 0x00, 0x31, 0x00,
		0x2e, 0x00, 0x30, 0x00, 0x00, 0x00
	};

uint8_t csp_name_ms_enhanced_csp[] =
	{
		0x4d, 0x00, 0x69, 0x00, 0x63, 0x00, 0x72, 0x00,
		0x6f, 0x00, 0x73, 0x00, 0x6f, 0x00, 0x66, 0x00,
		0x74, 0x00, 0x20, 0x00, 0x45, 0x00, 0x6e, 0x00,
		0x68, 0x00, 0x61, 0x00, 0x6e, 0x00, 0x63, 0x00,
		0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x43, 0x00,
		0x72, 0x00, 0x79, 0x00, 0x70, 0x00, 0x74, 0x00,
		0x6f, 0x00, 0x67, 0x00, 0x72, 0x00, 0x61, 0x00,
		0x70, 0x00, 0x68, 0x00, 0x69, 0x00, 0x63, 0x00,
		0x20, 0x00, 0x50, 0x00, 0x72, 0x00, 0x6f, 0x00,
		0x76, 0x00, 0x69, 0x00, 0x64, 0x00, 0x65, 0x00,
		0x72, 0x00, 0x20, 0x00, 0x76, 0x00, 0x31, 0x00,
		0x2e, 0x00, 0x30, 0x00, 0x00, 0x00
	};

uint8_t csp_name_ms_strong_csp[] =
	{
		0x4d, 0x00, 0x69, 0x00, 0x63, 0x00, 0x72, 0x00,
		0x6f, 0x00, 0x73, 0x00, 0x6f, 0x00, 0x66, 0x00,
		0x74, 0x00, 0x20, 0x00, 0x53, 0x00, 0x74, 0x00,
		0x72, 0x00, 0x6f, 0x00, 0x6e, 0x00, 0x67, 0x00,
		0x20, 0x00, 0x43, 0x00, 0x72, 0x00, 0x79, 0x00,
		0x70, 0x00, 0x74, 0x00, 0x6f, 0x00, 0x67, 0x00,
		0x72, 0x00, 0x61, 0x00, 0x70, 0x00, 0x68, 0x00,
		0x69, 0x00, 0x63, 0x00, 0x20, 0x00, 0x50, 0x00,
		0x72, 0x00, 0x6f, 0x00, 0x76, 0x00, 0x69, 0x00,
		0x64, 0x00, 0x65, 0x00, 0x72, 0x00, 0x00, 0x00
	};

csp_name_to_keysize_map_t csp_table[] =
	{
		{ 56, csp_name_ms_base_csp, sizeof(csp_name_ms_base_csp) },
		{ 128, csp_name_ms_enhanced_csp, sizeof(csp_name_ms_enhanced_csp) },
		{ 128, csp_name_ms_strong_csp, sizeof(csp_name_ms_strong_csp) },
	};


crypto_algo_e
parse_word_headers(FILE *file,
                   size_t *pKeySize, size_t *pMaxKeySize,
                   uint8_t *pSalt, size_t cbSalt,
                   uint8_t *pEncryptedVerifier,
                   size_t cbEncryptedVerifier,
                   uint8_t *pEncryptedVerifierHash,
                   size_t cbEncryptedVerifierHash)
{
	assert(NULL != file);
	assert(NULL != pKeySize);
	assert(NULL != pMaxKeySize);
	assert(NULL != pSalt);
	assert(cbSalt > 0);
	assert(NULL != pEncryptedVerifier);
	assert(cbEncryptedVerifier > 0);
	assert(NULL != pEncryptedVerifierHash);
	assert(cbEncryptedVerifierHash > 0);

	cfb_fileheader_t *pCfbFileHeader = NULL;
	crypto_data_t     cryptoData = { NULL, 0, NULL, 0, NULL, 0};
	uint32_t          ulFirstDirSectorOffset = 0;
	size_t            cbFileSize = 0;
	size_t            cbRead = 0;
	crypto_algo_e     result = unknown;
	int               whichTable = -1; // is 0 for '0Table' and 1 for '1Table' and -1 if FIB header has not been read yet
	
	word_file_streams_t* streams = (word_file_streams_t*)malloc(sizeof(word_file_streams_t));
	streams->tableStream = 0;
	streams->documentStream = 0;
	streams->dataStream = 0;

	cryptoData.pSalt = pSalt;
	cryptoData.cbSalt = cbSalt;
	cryptoData.pEncryptedVerifier = pEncryptedVerifier;
	cryptoData.cbEncryptedVerifier = cbEncryptedVerifier;
	cryptoData.pEncryptedVerifierHash = pEncryptedVerifierHash;
	cryptoData.cbEncryptedVerifierHash = cbEncryptedVerifierHash;

	memset(pSalt, 0, cbSalt);
	memset(pEncryptedVerifier, 0, cbEncryptedVerifier);
	memset(pEncryptedVerifierHash, 0, cbEncryptedVerifierHash);

	fseek(file, 0, SEEK_END);
	cbFileSize = ftell(file);

	// Read the CFB file header at the very beginning of the file.
	pCfbFileHeader = (cfb_fileheader_t*)malloc(sizeof(cfb_fileheader_t));
	assert(NULL != pCfbFileHeader);

	fseek(file, 0, SEEK_SET);
	cbRead = fread(pCfbFileHeader, 1, sizeof(cfb_fileheader_t), file);
	assert(sizeof(cfb_fileheader_t) == cbRead);

	// Find the offset of the first directory sector so we can parse it.
	ulFirstDirSectorOffset = SECTOR_OFFSET(
		htolel(pCfbFileHeader->FirstDirectorySectorLocation),
		htoles(pCfbFileHeader->SectorShift));

	// Verify the signature in the CFB header.
	if (0 != memcmp(cfb_header_signature, pCfbFileHeader, sizeof(cfb_header_signature)))
	{
		DPRINTF("The file is not a CFB.\n");
		return result;
	}

	if (CFB_FILEHEADER_BYTEORDER_LE != htoles(pCfbFileHeader->ByteOrder))
	{
		// The translated byte order is invalid. We cannot continue.
		assert(0);
		return result;
	}

	DPRINTF("Version                 = %d.%d\n", htoles(pCfbFileHeader->MajorVersion), htoles(pCfbFileHeader->MinorVersion));
	DPRINTF("FirstDirSectorOffset    = 0x%08x\n", ulFirstDirSectorOffset);
	DPRINTF("Number of DIFAT sectors = %d\n", htoles(pCfbFileHeader->NumberOfDifatSectors));
	

	// Read the root directory to find the '0Table' or '1Table' and 'WordDocument' streams.
	for (int entry = 0; (ulFirstDirSectorOffset + sizeof(cfb_directoryentry_t) * entry) < cbFileSize; entry++)
	{
		if (!parse_directory_entry(file, difat, streams, ulFirstDirSectorOffset,
		                           htoles(pCfbFileHeader->SectorShift),
		                           entry, &cryptoData, &result, pKeySize, pMaxKeySize, &whichTable))
		{
			// No more entries found, stop processing.
			break;
		}
	}
	
			
	if (NULL != pCfbFileHeader)
	{
		free(pCfbFileHeader);
		pCfbFileHeader = NULL;
	}
	
	free(difat);
	
	free(streams->tableStream);
	free(streams->documentStream);
	free(streams->dataStream);		
	free(streams);

	return result;
}

stream_info_t* create_stream_info(cfb_directoryentry_t* dirEntry) {
	stream_info_t* info = (stream_info_t*)malloc(sizeof(stream_info_t));
	info->size = dirEntry->StreamSizeLow; // TODO what about sizeHigh?
	info->firstSector = dirEntry->StartingSectorLocation;
	return info;
}

int
parse_directory_entry(FILE *file,
                      difat_t *difat,
		      word_file_streams_t* streams,
                      uint32_t ulSectorOffset,
                      uint16_t sectorShift,
                      uint32_t entryId,
                      crypto_data_t *pData,
                      crypto_algo_e *pAlgo,
                      size_t *pKeySize,
                      size_t *pMaxKeySize,
		      int *whichTable)
{
	assert(NULL != file);
	assert(NULL != pData);
	assert(NULL != pAlgo);
	assert(NULL != pKeySize);

	int                             result = -1;
	cfb_directoryentry_t           *pCfbDirEntry = NULL;
	crypt_rc4_encryption_header_t  *pRC4Header = NULL;
	crypt_capi_encryption_header_t *pCapiHeader = NULL;
	doc_fibbase_t                  *pFibBase = NULL;
	size_t                          cbRead = 0;

	pCfbDirEntry = (cfb_directoryentry_t*)malloc(sizeof(cfb_directoryentry_t));
	assert(NULL != pCfbDirEntry);

	fseek(file, ulSectorOffset + (entryId * sizeof(cfb_directoryentry_t)), SEEK_SET);
	cbRead = fread(pCfbDirEntry, 1, sizeof(cfb_directoryentry_t), file);
	assert(sizeof(cfb_directoryentry_t) == cbRead);

	if (CFB_DIRECTORYENTRY_OBJECTTYPE_UNKNOWN == pCfbDirEntry->ObjectType)
	{
		// We found an unallocated entry, i.e. the end of the directory.
		result = 0;
		goto Exit;
	}
	
	//DPRINTF("Entry %d is of type %d and has length %d\n", entryId, pCfbDirEntry->ObjectType, htolel(pCfbDirEntry->DirectoryEntryNameLength));
	
	if ((pCfbDirEntry->ObjectType != CFB_DIRECTORYENTRY_OBJECTTYPE_STREAM) && 
	    (pCfbDirEntry->ObjectType != CFB_DIRECTORYENTRY_OBJECTTYPE_ROOT)) {
		goto Exit;
	}
	assert(pCfbDirEntry->DirectoryEntryNameLength <= 64);
	
	wchar_t pwszEntryName[32];
	
	ucs2_to_wcs(pCfbDirEntry->DirectoryEntryName,
	            htoles(pCfbDirEntry->DirectoryEntryNameLength),
	            &pwszEntryName[0]);

	DPRINTF("Entry %d ('%ls') starts at 0x%08x and has 0x%08x bytes.\n", entryId, pwszEntryName,
		SECTOR_OFFSET(
			htolel(pCfbDirEntry->StartingSectorLocation),
			sectorShift),
		htolel(pCfbDirEntry->StreamSizeLow));
	
	if ((*whichTable == 1) && is_same_name(pCfbDirEntry->DirectoryEntryName,
				htoles(pCfbDirEntry->DirectoryEntryNameLength) / 2,
				cfb_direntry_1table,
				sizeof(cfb_direntry_1table) / sizeof(uint16_t)))
	{
	
		if (streams->tableStream == 0) {
			streams->tableStream = create_stream_info(pCfbDirEntry);
		}

		// '1Table' Stream -> Try to parse the EncryptionHeader at beginning of the stream.

		//DPRINTF("Entry %d is '1Table', size = %d bytes.\n",
		//	entryId, htolel(pCfbDirEntry->StreamSizeLow));

		pRC4Header = (crypt_rc4_encryption_header_t*)malloc(sizeof(crypt_rc4_encryption_header_t));
		assert(NULL != pRC4Header);

		pCapiHeader = (crypt_capi_encryption_header_t*)malloc(sizeof(crypt_capi_encryption_header_t));
		assert(NULL != pCapiHeader);

		fseek(file, SECTOR_OFFSET(
				htolel(pCfbDirEntry->StartingSectorLocation),
				sectorShift),
			SEEK_SET);

		cbRead = fread(pRC4Header, 1, sizeof(crypt_rc4_encryption_header_t), file);
		assert(sizeof(crypt_rc4_encryption_header_t) == cbRead);

		fseek(file, SECTOR_OFFSET(
				htolel(pCfbDirEntry->StartingSectorLocation),
				sectorShift),
			SEEK_SET);

		cbRead = fread(pCapiHeader, 1, sizeof(crypt_capi_encryption_header_t), file);
		assert(sizeof(crypt_capi_encryption_header_t) == cbRead);

		if ((RC4_VERSION_MAJOR == htoles(pRC4Header->VersionMajor)) &&
		    (RC4_VERSION_MINOR == htoles(pRC4Header->VersionMinor)))
		{
			parse_rc4_encryption_header(pRC4Header, pData);
			*pAlgo = rc4_basic;
		}
		else
		{
			if (parse_capi_encryption_header(pCapiHeader, pData, file,
				SECTOR_OFFSET(
					htolel(pCfbDirEntry->StartingSectorLocation),
					sectorShift),
				pKeySize, pMaxKeySize))
			{
				*pAlgo = rc4_capi;
			}
			else
			{
				DPRINTF("Unsupported Encryption Header format.\n");
			}
		}
	}
        else if ((*whichTable == 0) && is_same_name(pCfbDirEntry->DirectoryEntryName,
				htoles(pCfbDirEntry->DirectoryEntryNameLength) / 2,
				cfb_direntry_0table,
				sizeof(cfb_direntry_0table) / sizeof(uint16_t)))
	{
	
		if (streams->tableStream == 0) {
			streams->tableStream = create_stream_info(pCfbDirEntry);
		}

		// '0Table' Stream -> Try to parse the EncryptionHeader at the beginning of the stream.

		//DPRINTF("Entry %d is '0Table', size = %d bytes.\n",
		//	entryId, htolel(pCfbDirEntry->StreamSizeLow));

		pRC4Header = (crypt_rc4_encryption_header_t*)malloc(sizeof(crypt_rc4_encryption_header_t));
		assert(NULL != pRC4Header);

		pCapiHeader = (crypt_capi_encryption_header_t*)malloc(sizeof(crypt_capi_encryption_header_t));
		assert(NULL != pCapiHeader);

		fseek(file, SECTOR_OFFSET(
				htolel(pCfbDirEntry->StartingSectorLocation),
				sectorShift),
			SEEK_SET);

		cbRead = fread(pRC4Header, 1, sizeof(crypt_rc4_encryption_header_t), file);
		assert(sizeof(crypt_rc4_encryption_header_t) == cbRead);

		fseek(file, SECTOR_OFFSET(
				htolel(pCfbDirEntry->StartingSectorLocation),
				sectorShift),
			SEEK_SET);

		cbRead = fread(pCapiHeader, 1, sizeof(crypt_capi_encryption_header_t), file);
		assert(sizeof(crypt_capi_encryption_header_t) == cbRead);

		if ((RC4_VERSION_MAJOR == htoles(pRC4Header->VersionMajor)) &&
		    (RC4_VERSION_MINOR == htoles(pRC4Header->VersionMinor)))
		{
			parse_rc4_encryption_header(pRC4Header, pData);
			*pAlgo = rc4_basic;
		}
		else
		{
			if (parse_capi_encryption_header(pCapiHeader, pData, file,
				SECTOR_OFFSET(
					htolel(pCfbDirEntry->StartingSectorLocation),
					sectorShift),
				pKeySize, pMaxKeySize))
			{
				*pAlgo = rc4_capi;
			}
			else
			{
				DPRINTF("Unsupported Encryption Header format.\n");
			}
		}
	}
	else if (is_same_name(pCfbDirEntry->DirectoryEntryName,
		              htoles(pCfbDirEntry->DirectoryEntryNameLength) / 2,
		              cfb_direntry_data,
		              sizeof(cfb_direntry_data) / sizeof(uint16_t)))
	{
	
		if (streams->dataStream == 0) {
			streams->dataStream = create_stream_info(pCfbDirEntry);
		}

	}
	else if (is_same_name(pCfbDirEntry->DirectoryEntryName,
		              htoles(pCfbDirEntry->DirectoryEntryNameLength) / 2,
		              cfb_direntry_worddoc,
		              sizeof(cfb_direntry_worddoc) / sizeof(uint16_t)))
	{
	
		if (streams->documentStream == 0) {
			streams->documentStream = create_stream_info(pCfbDirEntry);
		}

		// 'WordDocument' Stream -> Try to parse the FIBBase header at the beginning of the stream.

		//DPRINTF("Entry %d is 'WordDocument', size = %d bytes.\n",
		//	entryId, htolel(pCfbDirEntry->StreamSizeLow));

		fseek(file,
			SECTOR_OFFSET(
				htolel(pCfbDirEntry->StartingSectorLocation),
				sectorShift),
			SEEK_SET);

		if (!parse_fibbase(file, &pFibBase))
		{
			DPRINTF("The FIB Base could not be parsed.\n");
		}
		
		if (( htoles(pFibBase->Flags) & FIBBASE_FLAG_WHICHTBLSTM) == 0)
		{
		       *whichTable = 0;
		}
		else
		{
		       *whichTable = 1;
		}

	}

Exit:
	if (NULL != pCfbDirEntry)
	{
		free(pCfbDirEntry);
		pCfbDirEntry = NULL;
	}

	if (NULL != pRC4Header)
	{
		free(pRC4Header);
		pRC4Header = NULL;
	}

	if (NULL != pFibBase)
	{
		free(pFibBase);
		pFibBase = NULL;
	}

	return result;
}

int
is_same_name(const uint16_t *name1, size_t size1,
             const uint16_t *name2, size_t size2)
{
	if (size1 != size2)
	{
		return 0;
	}

	if (0 == memcmp(name1, name2, size1 * sizeof(uint16_t)))
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

int
parse_fibbase(FILE *file, doc_fibbase_t **ppFibBase)
{
	assert(NULL != file);
	assert(NULL != ppFibBase);
	assert(NULL == *ppFibBase);

	doc_fibbase_t *pFibBase = NULL;
	int            result = -1;
	size_t         cbRead;

	pFibBase = (doc_fibbase_t*)malloc(sizeof(doc_fibbase_t));
	assert(NULL != pFibBase);

	cbRead = fread(pFibBase, 1, sizeof(doc_fibbase_t), file);
	assert(sizeof(doc_fibbase_t) == cbRead);

	if (FIBBASE_IDENTIFIER != htoles(pFibBase->wIdent))
	{
		DPRINTF("Unknown identifier in the FibBase: %04X expected, %04X found.\n",
			FIBBASE_IDENTIFIER, htoles(pFibBase->wIdent));
		assert(0);
		result = 0;
		goto Exit;
	}

	if ((FIBBASE_FLAG_ENCRYPTED != (htoles(pFibBase->Flags) & FIBBASE_FLAG_ENCRYPTED)))
	{
		DPRINTF("The file is not encrypted'.\n");
		result = 0;
		goto Exit;
	}

	DPRINTF("nFibBack              = 0x%04x\n", htoles(pFibBase->nFibBack));
	DPRINTF("lid                   = 0x%04x\n", htoles(pFibBase->lid));
	DPRINTF("pnNext                = 0x%04x\n", htoles(pFibBase->pnNext));
	DPRINTF("nFib                  = 0x%04x\n", htoles(pFibBase->nFib));
	DPRINTF("EncryptionHeader size = %d bytes.\n", htolel(pFibBase->lKey));

	*ppFibBase = pFibBase;
	pFibBase = NULL;

Exit:
	if (NULL != pFibBase)
	{
		free(pFibBase);
		pFibBase = NULL;
	}

	return result;
}

void
parse_rc4_encryption_header(crypt_rc4_encryption_header_t *pHeader, crypto_data_t *pData)
{
	assert(NULL != pHeader);
	assert(NULL != pData);
	assert(NULL != pData->pSalt);
	assert(16 <= pData->cbSalt);
	assert(NULL != pData->pEncryptedVerifier);
	assert(16 <= pData->cbEncryptedVerifier);
	assert(NULL != pData->pEncryptedVerifierHash);
	assert(16 <= pData->cbEncryptedVerifierHash);
	assert(RC4_VERSION_MAJOR == htoles(pHeader->VersionMajor));
	assert(RC4_VERSION_MINOR == htoles(pHeader->VersionMinor));

	memcpy(pData->pSalt, pHeader->Salt, 16);
	memcpy(pData->pEncryptedVerifier, pHeader->EncryptedVerifier, 16);
	memcpy(pData->pEncryptedVerifierHash, pHeader->EncryptedVerifierHash, 16);
}

uint32_t
get_max_key_size(uint16_t *pCspName, size_t cbCspName, uint32_t keySize)
{
	assert(NULL != pCspName);

	uint32_t maxKeySize = 0;

	for (int i = 0; i < sizeof(csp_table) / sizeof(csp_name_to_keysize_map_t); i++)
	{
		if (csp_table[i].CspNameLength != cbCspName)
		{
			continue;
		}

		if (!memcmp(csp_name_ms_base_csp, pCspName, cbCspName))
		{
			// 'Microsoft Base Cryptographic Provider':
			// This CSP seems to have a variable max
			// key size, depending on the actual key
			// size. Account for this here.
			if (keySize == 40)
			{
				maxKeySize = 128;
			}
			else
			{
				maxKeySize = keySize;
			}
			break;
		}

		if (!memcmp(csp_table[i].pCspName, pCspName, cbCspName))
		{
			maxKeySize = csp_table[i].MaxKeySize;
			DPRINTF("Found the CSP; max key size = %d bits.\n", keySize);
			break;
		}
	}

	return maxKeySize;
}

int
parse_capi_encryption_header(crypt_capi_encryption_header_t *pHeader,
                             crypto_data_t *pData, FILE *file,
                             uint32_t ulStreamBase,
                             size_t *pKeySize, size_t *pMaxKeySize)
{
	assert(NULL != pHeader);
	assert(NULL != pData);
	assert(NULL != pData->pSalt);
	assert(NULL != pData->pEncryptedVerifier);
	assert(NULL != pData->pEncryptedVerifierHash);
	assert(NULL != file);
	assert(NULL != pKeySize);

	size_t cbRead = 0;

	//DPRINTF("CAPI EH.Version      = %d.%d\n", htoles(pHeader->VersionMajor), htoles(pHeader->VersionMinor));
	//DPRINTF("CAPI EH.Flags        = %08x\n", htolel(pHeader->Flags));
	DPRINTF("HeaderSize   = %d\n", htolel(pHeader->HeaderSize));

	if (((CAPI_RC4_VERSION_MAJOR2 == htoles(pHeader->VersionMajor)) ||
             (CAPI_RC4_VERSION_MAJOR3 == htoles(pHeader->VersionMajor))) &&
            (CAPI_RC4_VERSION_MINOR == htoles(pHeader->VersionMinor)))
	{
		crypt_capi_rc4_encryption_header_t *pRC4Header = NULL;

		pRC4Header = (crypt_capi_rc4_encryption_header_t*)malloc(htolel(pHeader->HeaderSize));
		assert(NULL != pRC4Header);

		fseek(file, ulStreamBase + sizeof(crypt_capi_encryption_header_t), SEEK_SET);
		cbRead = fread(pRC4Header, 1, htolel(pHeader->HeaderSize), file);
		assert(cbRead == htolel(pHeader->HeaderSize));
	
		DPRINTF("\n");
		//DPRINTF("CAPI EH.Flags        = %08x\n", htolel(pRC4Header->Flags));
		//DPRINTF("CAPI EH.SizeExtra    = %08x\n", htolel(pRC4Header->SizeExtra));
		DPRINTF("CAPI EH.AlgID        = %08x\n", htolel(pRC4Header->AlgID));
		DPRINTF("CAPI EH.AlgIDHash    = %08x\n", htolel(pRC4Header->AlgIDHash));
		DPRINTF("KeySize      = %d\n", htolel(pRC4Header->KeySize));
		DPRINTF("CAPI EH.ProviderType = %08x\n", htolel(pRC4Header->ProviderType));

		if (CAPI_ALGO_RC4 != htolel(pRC4Header->AlgID) ||
		    CAPI_ALGO_SHA1 != htolel(pRC4Header->AlgIDHash) ||
		    CAPI_CSP_RC4 != htolel(pRC4Header->ProviderType))
		{
			DPRINTF("Unknown Encryption Algorithm.\n");
			return unknown;
		}

		*pKeySize = htolel(pRC4Header->KeySize);

		cbRead = fread(&pData->cbSalt, 1, sizeof(uint32_t), file);
		pData->cbSalt = htolel(pData->cbSalt);
		assert(sizeof(uint32_t) == cbRead);

		DPRINTF("\n");
		DPRINTF("SaltSize = %d\n", pData->cbSalt);
		assert(16 == pData->cbSalt); // Only 16 byte salts supported at this time.

		cbRead = fread(pData->pSalt, 1, pData->cbSalt, file);
		assert(cbRead = pData->cbSalt);
		HD("SALT", pData->pSalt, pData->cbSalt);

		pData->cbEncryptedVerifier = 16;
		cbRead = fread(pData->pEncryptedVerifier, 1, 16, file);
		assert(cbRead == pData->cbEncryptedVerifier);

		cbRead = fread(&pData->cbEncryptedVerifierHash, 1, sizeof(uint32_t), file);
		pData->cbEncryptedVerifierHash = htolel(pData->cbEncryptedVerifierHash);
		assert(sizeof(uint32_t) == cbRead);

		DPRINTF("VerifierHashSize = %d\n", pData->cbEncryptedVerifierHash);
		cbRead = fread(pData->pEncryptedVerifierHash, 1, pData->cbEncryptedVerifierHash, file);
		assert(cbRead == pData->cbEncryptedVerifierHash);

		*pMaxKeySize = get_max_key_size(
			&pRC4Header->CSPName,
			htolel(pHeader->HeaderSize) - sizeof(crypt_capi_rc4_encryption_header_t) + sizeof(uint16_t),
			htolel(pRC4Header->KeySize));
		DPRINTF("MaxKeySize = %d\n", *pMaxKeySize);

		return rc4_capi;
	}

	return unknown;
}

