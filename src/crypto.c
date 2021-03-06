#include "crypto.h"
#include "errorcodes.h"
#include "wordstreams.h"
#include "strconv.h"
#include "byteswap.h"
#include "string.h"
#include "byteswap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/rc4.h>


char* WordVersion;     //Word-Version
char* EncryptionAlgo;  //Encryption-Algorithm


int create_crypto_ctx(cfb_t *pCfb, crypto_ctx_t*ctx, char*pw) {

  // retrieve the stream with the encryption header information */
  cfb_directoryentry_t *pEncInfoEntry = 
          CFB_GET_DIRENTRY_BY_NAME (pCfb, cfb_direntry_EncryptionInfo);
  cfb_directoryentry_t* pTableEntry;
  // load old 1Table stream if it is not a word 2007 format
  if (!pEncInfoEntry) { pTableEntry = getTableStream(pCfb); }

  if ((pEncInfoEntry==0) && (pTableEntry == 0)) {
    fprintf(stderr, "EncryptionInfo Stream does not exist");
    fprintf(stderr, "Table stream could not be found\n");
    exit(INVALID_DOCUMENT_STRUCTURE);
  }

  uint8_t* encInfoStream = 0;
  if (pEncInfoEntry) { // 	Word 2007
    fprintf (stderr, "Word-Format             = Office Word 2007\n");
    // make the ministream physically contiguous
    uint32_t miniSectorSize = 1 << htoles (pCfb->fileHeader->MiniSectorShift);
    uint32_t cur = pEncInfoEntry->StartingSectorLocation;
    encInfoStream = (uint8_t*)malloc(sizeof(uint8_t)*
               (pEncInfoEntry->StreamSizeLow + miniSectorSize));
    for (int i=0; cur!=ENDOFCHAIN; i++) {
      memcpy (encInfoStream+miniSectorSize*i, 
              &pCfb->ministream[cur*miniSectorSize], miniSectorSize);
      cur = iter_ministream_next (pCfb, cur);
    }
   // HD("EncryptionInfo Stream", encInfoStream, pEncInfoEntry->StreamSizeLow);
  } else {
    fprintf (stderr, "Word-Format             = Word 97 - 2003\n");
    if ((getFibbase(pCfb)->Flags & FIBBASE_FLAG_ENCRYPTED) == 0) {
      printf("File is not encrypted!\n");

	encInfoStream = &pCfb->data[sector_offset(pCfb, 
                     htolel(pTableEntry->StartingSectorLocation))];
  	uint16_t nVerBuiltInNamesWhenSaved =  htoles( *((uint16_t*)(encInfoStream
						 + 6*(sizeof(uint16_t)))) );

	     switch(nVerBuiltInNamesWhenSaved) {
		    case 2: {
		       WordVersion = "Word 97";
		    }
		    break;
		    case 3: {
		       WordVersion= "Word 2000/2002";
		    }
		    break;
		    case 4: {
			WordVersion= "Office Word 2003";
		    }
		    break;
		    case 7: {
		       WordVersion= "Office Word 2007/2010";
		    }
		    break;
		    default:
		       WordVersion= "not found";
		  }

		  printf("Last saved with         = %s\n\n", WordVersion);

      return -1;
    }
    encInfoStream = &pCfb->data[sector_offset(pCfb, 
                     htolel(pTableEntry->StartingSectorLocation))];
  }
  
  
  // Table Stream: Try to parse the EncryptionHeader at the beginning of the stream.
  uint16_t encVersionMajor =  htoles( *((uint16_t*)encInfoStream) );
  uint16_t encVersionMinor =  htoles( *((uint16_t*)(encInfoStream
                                                    + sizeof(uint16_t))) );

  if ((RC4_VERSION_MAJOR == encVersionMajor) &&
      (RC4_VERSION_MINOR == encVersionMinor)) {
    parse_rc4_encryption_header(encInfoStream, ctx);
  } else if (( (CAPI_RC4_VERSION_MAJOR2 == encVersionMajor) 
               || (CAPI_RC4_VERSION_MAJOR3 == encVersionMajor))
             && (CAPI_RC4_VERSION_MINOR == encVersionMinor)) {
    parse_capi_encryption_header(encInfoStream, ctx);
  }

  if (pEncInfoEntry) { free(encInfoStream); }
  return 0;
}

doc_fibbase_t* getFibbase(cfb_t* pCfb) {
  cfb_directoryentry_t *pDirentry = CFB_GET_DIRENTRY_BY_NAME (pCfb, cfb_direntry_WordDocument);
  if (pDirentry==0) {
    fprintf(stderr, "WordDocument stream could not be found\n");
    exit(INVALID_DOCUMENT_STRUCTURE);
  }
  
  uint32_t offset = sector_offset(pCfb, pDirentry->StartingSectorLocation);
  doc_fibbase_t *pFibbase = (doc_fibbase_t*) &(pCfb->data[offset]);
  
  if (htoles(pFibbase->wIdent) != FIBBASE_IDENTIFIER) {
    fprintf(stderr, "Fibbase has wrong identifier\n");
    exit(INVALID_DOCUMENT_STRUCTURE);
  }
  
  return pFibbase; 
}

cfb_directoryentry_t*
getTableStream(cfb_t *pCfb) {

  doc_fibbase_t* pFibbase = getFibbase(pCfb);
  
  if ((htoles(pFibbase->Flags) & FIBBASE_FLAG_WHICHTBLSTM) == 0) {
    // whichTable == 0
    return CFB_GET_DIRENTRY_BY_NAME (pCfb, cfb_direntry_0Table);
  } else {
    // whichTable == 1
    return CFB_GET_DIRENTRY_BY_NAME (pCfb, cfb_direntry_1Table);
  }

}

void
parse_rc4_encryption_header(uint8_t* pData, crypto_ctx_t* pCryptoCtx) {

  crypt_rc4_encryption_header_t* pRc4EncHeader = (crypt_rc4_encryption_header_t*)pData;

  pCryptoCtx->algo = rc4_basic;
  pCryptoCtx->keybits = 40;
  pCryptoCtx->maxkeybits = 40;
  memcpy(pCryptoCtx->salt, pRc4EncHeader->Salt, 16);
  memcpy(pCryptoCtx->ev, pRc4EncHeader->EncryptedVerifier, 16);
  memcpy(pCryptoCtx->evh, pRc4EncHeader->EncryptedVerifierHash, 16);
  
  printf("Encryption-Algorithm    = RC4/Basic (w/o Crypto API)\n");
  printf("Hash-Algorithm          = MD5\n");  
  printf("Keysize                 = 40-bit\n\n");
  //HD("Salt", pCryptoCtx->salt, 16);
  //HD("EncryptedVerifier", pCryptoCtx->ev, 16);
  //HD("EncryptedVerifierHash", pCryptoCtx->evh, 16);
    
}

void 
parse_capi_encryption_header(uint8_t* pData, crypto_ctx_t* pCryptoCtx) {

  /* pointer to first position of StreamHeader, EncryptionHeader, Verifier */
  crypt_capi_encryption_header_t* pStreamHeader = 
       (crypt_capi_encryption_header_t*)pData;
  crypt_capi_rc4_encryption_header_t *pEncHeader = 
     (crypt_capi_rc4_encryption_header_t*)
        (pData + sizeof(crypt_capi_encryption_header_t));
  uint32_t* verifierData = (uint32_t*)
       (pData + sizeof(crypt_capi_encryption_header_t) 
              + htolel(pStreamHeader->HeaderSize));

  /* dump some information */
  //printf("CAPI EH.Version      = %d.%d\n", 
  //  htoles(pStreamHeader->VersionMajor), htoles(pStreamHeader->VersionMinor));
  //printf("CAPI EH.Flags        = %08x\n", htolel(pStreamHeader->Flags));
  //printf("\nHeaderSize             = %d Byte\n", htolel(pStreamHeader->HeaderSize));
  //printf("CAPI EH.Flags        = %08x\n", htolel(pEncHeader->Flags));
  //printf("CAPI EH.SizeExtra    = %d\n", htolel(pEncHeader->SizeExtra));

  switch(htolel(pEncHeader->AlgID)) {
		    case 26625: {
		       EncryptionAlgo = "RC4";
		    }
		    break;
		    case 26126: {
		       EncryptionAlgo= "128-bit AES";
		    }
		    break;
		    case 26127: {
			EncryptionAlgo= "192-bit AES";
		    }
		    break;
		    case 26128: {
			EncryptionAlgo= "256-bit AES";
		    }
		    break;
		    case 0: {
  			if ((pEncHeader->Flags & 0x0400) != 0) //AES-Flag 0000 0100
			  EncryptionAlgo= "AES";
			else   
				if ((pEncHeader->Flags & 0x0800) == 0) //0000 1000
				  EncryptionAlgo = "RC4";
				else
				EncryptionAlgo = "External";

		    }
		    break;
		    default:
		       EncryptionAlgo= "not found";
		  }



  printf("Encryption-Algorithm    = %s (with Crypto API)\n", EncryptionAlgo);
  //printf("CAPI EH.AlgIDHash    = %08x\n", htolel(pEncHeader->AlgIDHash)); //MUST BE SHA-1 (8004)
  printf("Hash-Algorithm          = SHA-1\n");  
  printf("Keysize                 = %d-bit\n", htolel(pEncHeader->KeySize));
  //printf("CAPI EH.ProviderType   = %08x\n\n", htolel(pEncHeader->ProviderType));

  
  int len_CSPName = 0;
  uint16_t *pCSPName = (uint16_t *)&(pEncHeader->CSPName);
  while (pCSPName[len_CSPName] != 0) len_CSPName++;
  
  printf("Crypto Service Provider = ");
  for (int ind_txt = 0; ind_txt < len_CSPName; ind_txt++) {
    fprintf(stdout, "%c", (char)pCSPName[ind_txt]);
  }

  printf("\n");


  // check which version
  uint32_t flags = htolel(pStreamHeader->Flags);
  //fprintf (stderr, "Flags=%x\n", flags);
  if (flags & ENCHEADER_FLAGS_EXTERNAL) {
    fprintf (stderr, "External Encryption currently not supported\n");
    exit(UNKOWN_ENCRYPTION_ALGORITHM);
  } else if (flags & ENCHEADER_FLAGS_AES) {
    //fprintf (stderr, "AES encryption of ECMA-376 documents\n");
    pCryptoCtx->algo = aes;
    pCryptoCtx->keybits = htolel(pEncHeader->KeySize);
    //fprintf (stderr, "TODO, not finished yet\n");
  } else {
    // old format 

    if (CAPI_ALGO_RC4 != htolel(pEncHeader->AlgID) ||
		    CAPI_ALGO_SHA1 != htolel(pEncHeader->AlgIDHash) ||
		    CAPI_CSP_RC4 != htolel(pEncHeader->ProviderType))
    {
      printf("Unknown Encryption Algorithm.\n");
      exit(UNKOWN_ENCRYPTION_ALGORITHM);
    }

    pCryptoCtx->algo = rc4_capi;
    pCryptoCtx->keybits = htolel(pEncHeader->KeySize);
  
    if (pCryptoCtx->keybits == 40) {
      pCryptoCtx->maxkeybits = 128;
    } else {
      pCryptoCtx->maxkeybits = pCryptoCtx->keybits;
    }
  }

  pCryptoCtx->salt_size = verifierData[0];

  printf("\n");
  //printf("SaltSize = %d\n", pCryptoCtx->salt_size);
  //assert(16 == pCryptoCtx->salt_size); // Only 16 byte salts supported at this time.
  
  pCryptoCtx->ev_size = 16;

  memcpy(pCryptoCtx->salt, &verifierData[1], pCryptoCtx->salt_size);
  //HD("Salt", pCryptoCtx->salt, pCryptoCtx->salt_size);

  memcpy(pCryptoCtx->ev, &verifierData[5], pCryptoCtx->salt_size);
  //HD("Verifier", pCryptoCtx->ev, pCryptoCtx->salt_size);

  uint32_t evhSize = verifierData[9];
  //assert(20 == evhSize);  
  pCryptoCtx->evh_size = evhSize;

  memcpy(pCryptoCtx->evh, &verifierData[10], 32);

  //printf("VerifierHashSize = %d\n", evhSize);
  //HD("VerifierHash", pCryptoCtx->evh, evhSize);
}

