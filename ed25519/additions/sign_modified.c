#include <string.h>
#include "crypto_sign.h"
#include "crypto_hash_sha512.h"
#include "ge.h"
#include "sc.h"
#include "zeroize.h"

/* NEW: Compare to pristine crypto_sign() 
   Uses explicit private key for nonce derivation and as scalar,
   instead of deriving both from a master key.
*/
int crypto_sign_modified(
  unsigned char *sm,unsigned long long *smlen,
  const unsigned char *m,unsigned long long mlen,
  const unsigned char *sk, const unsigned char* pk,
  const unsigned char* random
)
{
  unsigned char nonce[64];
  unsigned char hram[64];
  ge_p3 R;

  *smlen = mlen + 64;
  memmove(sm + 64,m,mlen);
  memmove(sm + 32,sk,32); /* NEW: Use privkey directly for nonce derivation */
  crypto_hash_sha512(nonce,sm + 32,mlen + 32);
  memmove(sm + 32,pk,32);

  /* NEW: XOR random into nonce */
  for (int count=0; count < 32; count++)
    nonce[count] ^= random[count];

  sc_reduce(nonce);
  ge_scalarmult_base(&R,nonce);
  ge_p3_tobytes(sm,&R);

  crypto_hash_sha512(hram,sm,mlen + 64);
  sc_reduce(hram);
  sc_muladd(sm + 32,hram,sk,nonce); /* NEW: Use privkey directly */

  volatile unsigned char* p = sm+64;
  sc_muladd(sm+64,hram,hram,hram);
  /* Dummy call to hopefully erase any traces of privkey or nonce
     left in the stack from prev call to this func */

  zeroize(nonce, 64);
  return 0;
}
