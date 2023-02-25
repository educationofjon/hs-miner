#define SPH_C64(x)    ((ulong)(x))
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

typedef struct {
	unsigned char buf[1024];    /* first field, for alignment. We set  uf to some ungodly large # that isnt needed */
	int wordCapacity;
	ulong saved;
	int wordIndex;
	union {
		ulong wide[25];
		uint narrow[50];
	} u;
} sha3_context;

constant ulong RC[] = {
	SPH_C64(0x0000000000000001), SPH_C64(0x0000000000008082),
	SPH_C64(0x800000000000808A), SPH_C64(0x8000000080008000),
	SPH_C64(0x000000000000808B), SPH_C64(0x0000000080000001),
	SPH_C64(0x8000000080008081), SPH_C64(0x8000000000008009),
	SPH_C64(0x000000000000008A), SPH_C64(0x0000000000000088),
	SPH_C64(0x0000000080008009), SPH_C64(0x000000008000000A),
	SPH_C64(0x000000008000808B), SPH_C64(0x800000000000008B),
	SPH_C64(0x8000000000008089), SPH_C64(0x8000000000008003),
	SPH_C64(0x8000000000008002), SPH_C64(0x8000000000000080),
	SPH_C64(0x000000000000800A), SPH_C64(0x800000008000000A),
	SPH_C64(0x8000000080008081), SPH_C64(0x8000000000008080),
	SPH_C64(0x0000000080000001), SPH_C64(0x8000000080008008)
};

#define a00   (sha3Ctx->u.wide[ 0])
#define a10   (sha3Ctx->u.wide[ 1])
#define a20   (sha3Ctx->u.wide[ 2])
#define a30   (sha3Ctx->u.wide[ 3])
#define a40   (sha3Ctx->u.wide[ 4])
#define a01   (sha3Ctx->u.wide[ 5])
#define a11   (sha3Ctx->u.wide[ 6])
#define a21   (sha3Ctx->u.wide[ 7])
#define a31   (sha3Ctx->u.wide[ 8])
#define a41   (sha3Ctx->u.wide[ 9])
#define a02   (sha3Ctx->u.wide[10])
#define a12   (sha3Ctx->u.wide[11])
#define a22   (sha3Ctx->u.wide[12])
#define a32   (sha3Ctx->u.wide[13])
#define a42   (sha3Ctx->u.wide[14])
#define a03   (sha3Ctx->u.wide[15])
#define a13   (sha3Ctx->u.wide[16])
#define a23   (sha3Ctx->u.wide[17])
#define a33   (sha3Ctx->u.wide[18])
#define a43   (sha3Ctx->u.wide[19])
#define a04   (sha3Ctx->u.wide[20])
#define a14   (sha3Ctx->u.wide[21])
#define a24   (sha3Ctx->u.wide[22])
#define a34   (sha3Ctx->u.wide[23])
#define a44   (sha3Ctx->u.wide[24])

ulong
dec64le_aligned(const void *src)
{
	return (ulong)(((const unsigned char *)src)[0])
		| ((ulong)(((const unsigned char *)src)[1]) << 8)
		| ((ulong)(((const unsigned char *)src)[2]) << 16)
		| ((ulong)(((const unsigned char *)src)[3]) << 24)
		| ((ulong)(((const unsigned char *)src)[4]) << 32)
		| ((ulong)(((const unsigned char *)src)[5]) << 40)
		| ((ulong)(((const unsigned char *)src)[6]) << 48)
		| ((ulong)(((const unsigned char *)src)[7]) << 56);
}

#define enc64le_aligned(dst, val) (*((ulong*)(dst)) = (val))

#define SPH_T64(x)    ((x) & SPH_C64(0xFFFFFFFFFFFFFFFF))
#define SPH_ROTL64(x, n)   SPH_T64(((x) << (n)) | ((x) >> (64 - (n))))
#define SPH_ROTR64(x, n)   SPH_ROTL64(x, (64 - (n)))

#define DECL64(x)        ulong x
#define MOV64(d, s)      (d = s)
#define XOR64(d, a, b)   (d = a ^ b)
#define AND64(d, a, b)   (d = a & b)
#define OR64(d, a, b)    (d = a | b)
#define NOT64(d, s)      (d = SPH_T64(~s))
#define ROL64(d, v, n)   (d = SPH_ROTL64(v, n))
#define XOR64_IOTA       XOR64


#define TH_ELT(t, c0, c1, c2, c3, c4, d0, d1, d2, d3, d4)   { \
		DECL64(tt0); \
		DECL64(tt1); \
		DECL64(tt2); \
		DECL64(tt3); \
		XOR64(tt0, d0, d1); \
		XOR64(tt1, d2, d3); \
		XOR64(tt0, tt0, d4); \
		XOR64(tt0, tt0, tt1); \
		ROL64(tt0, tt0, 1); \
		XOR64(tt2, c0, c1); \
		XOR64(tt3, c2, c3); \
		XOR64(tt0, tt0, c4); \
		XOR64(tt2, tt2, tt3); \
		XOR64(t, tt0, tt2); \
	}

#define THETA(b00, b01, b02, b03, b04, b10, b11, b12, b13, b14, \
	b20, b21, b22, b23, b24, b30, b31, b32, b33, b34, \
	b40, b41, b42, b43, b44) \
	{ \
		DECL64(t0); \
		DECL64(t1); \
		DECL64(t2); \
		DECL64(t3); \
		DECL64(t4); \
		TH_ELT(t0, b40, b41, b42, b43, b44, b10, b11, b12, b13, b14); \
		TH_ELT(t1, b00, b01, b02, b03, b04, b20, b21, b22, b23, b24); \
		TH_ELT(t2, b10, b11, b12, b13, b14, b30, b31, b32, b33, b34); \
		TH_ELT(t3, b20, b21, b22, b23, b24, b40, b41, b42, b43, b44); \
		TH_ELT(t4, b30, b31, b32, b33, b34, b00, b01, b02, b03, b04); \
		XOR64(b00, b00, t0); \
		XOR64(b01, b01, t0); \
		XOR64(b02, b02, t0); \
		XOR64(b03, b03, t0); \
		XOR64(b04, b04, t0); \
		XOR64(b10, b10, t1); \
		XOR64(b11, b11, t1); \
		XOR64(b12, b12, t1); \
		XOR64(b13, b13, t1); \
		XOR64(b14, b14, t1); \
		XOR64(b20, b20, t2); \
		XOR64(b21, b21, t2); \
		XOR64(b22, b22, t2); \
		XOR64(b23, b23, t2); \
		XOR64(b24, b24, t2); \
		XOR64(b30, b30, t3); \
		XOR64(b31, b31, t3); \
		XOR64(b32, b32, t3); \
		XOR64(b33, b33, t3); \
		XOR64(b34, b34, t3); \
		XOR64(b40, b40, t4); \
		XOR64(b41, b41, t4); \
		XOR64(b42, b42, t4); \
		XOR64(b43, b43, t4); \
		XOR64(b44, b44, t4); \
	}

#define RHO(b00, b01, b02, b03, b04, b10, b11, b12, b13, b14, \
	b20, b21, b22, b23, b24, b30, b31, b32, b33, b34, \
	b40, b41, b42, b43, b44) \
	{ \
		/* ROL64(b00, b00,  0); */ \
		ROL64(b01, b01, 36); \
		ROL64(b02, b02,  3); \
		ROL64(b03, b03, 41); \
		ROL64(b04, b04, 18); \
		ROL64(b10, b10,  1); \
		ROL64(b11, b11, 44); \
		ROL64(b12, b12, 10); \
		ROL64(b13, b13, 45); \
		ROL64(b14, b14,  2); \
		ROL64(b20, b20, 62); \
		ROL64(b21, b21,  6); \
		ROL64(b22, b22, 43); \
		ROL64(b23, b23, 15); \
		ROL64(b24, b24, 61); \
		ROL64(b30, b30, 28); \
		ROL64(b31, b31, 55); \
		ROL64(b32, b32, 25); \
		ROL64(b33, b33, 21); \
		ROL64(b34, b34, 56); \
		ROL64(b40, b40, 27); \
		ROL64(b41, b41, 20); \
		ROL64(b42, b42, 39); \
		ROL64(b43, b43,  8); \
		ROL64(b44, b44, 14); \
	}

/*
 * The KHI macro integrates the "lane complement" optimization. On input,
 * some words are complemented:
 *    a00 a01 a02 a04 a13 a20 a21 a22 a30 a33 a34 a43
 * On output, the following words are complemented:
 *    a04 a10 a20 a22 a23 a31
 *
 * The (implicit) permutation and the theta expansion will bring back
 * the input mask for the next round.
 */

#define KHI_XO(d, a, b, c)   { \
		DECL64(kt); \
		OR64(kt, b, c); \
		XOR64(d, a, kt); \
	}

#define KHI_XA(d, a, b, c)   { \
		DECL64(kt); \
		AND64(kt, b, c); \
		XOR64(d, a, kt); \
	}

#define KHI(b00, b01, b02, b03, b04, b10, b11, b12, b13, b14, \
	b20, b21, b22, b23, b24, b30, b31, b32, b33, b34, \
	b40, b41, b42, b43, b44) \
	{ \
		DECL64(c0); \
		DECL64(c1); \
		DECL64(c2); \
		DECL64(c3); \
		DECL64(c4); \
		DECL64(bnn); \
		NOT64(bnn, b20); \
		KHI_XO(c0, b00, b10, b20); \
		KHI_XO(c1, b10, bnn, b30); \
		KHI_XA(c2, b20, b30, b40); \
		KHI_XO(c3, b30, b40, b00); \
		KHI_XA(c4, b40, b00, b10); \
		MOV64(b00, c0); \
		MOV64(b10, c1); \
		MOV64(b20, c2); \
		MOV64(b30, c3); \
		MOV64(b40, c4); \
		NOT64(bnn, b41); \
		KHI_XO(c0, b01, b11, b21); \
		KHI_XA(c1, b11, b21, b31); \
		KHI_XO(c2, b21, b31, bnn); \
		KHI_XO(c3, b31, b41, b01); \
		KHI_XA(c4, b41, b01, b11); \
		MOV64(b01, c0); \
		MOV64(b11, c1); \
		MOV64(b21, c2); \
		MOV64(b31, c3); \
		MOV64(b41, c4); \
		NOT64(bnn, b32); \
		KHI_XO(c0, b02, b12, b22); \
		KHI_XA(c1, b12, b22, b32); \
		KHI_XA(c2, b22, bnn, b42); \
		KHI_XO(c3, bnn, b42, b02); \
		KHI_XA(c4, b42, b02, b12); \
		MOV64(b02, c0); \
		MOV64(b12, c1); \
		MOV64(b22, c2); \
		MOV64(b32, c3); \
		MOV64(b42, c4); \
		NOT64(bnn, b33); \
		KHI_XA(c0, b03, b13, b23); \
		KHI_XO(c1, b13, b23, b33); \
		KHI_XO(c2, b23, bnn, b43); \
		KHI_XA(c3, bnn, b43, b03); \
		KHI_XO(c4, b43, b03, b13); \
		MOV64(b03, c0); \
		MOV64(b13, c1); \
		MOV64(b23, c2); \
		MOV64(b33, c3); \
		MOV64(b43, c4); \
		NOT64(bnn, b14); \
		KHI_XA(c0, b04, bnn, b24); \
		KHI_XO(c1, bnn, b24, b34); \
		KHI_XA(c2, b24, b34, b44); \
		KHI_XO(c3, b34, b44, b04); \
		KHI_XA(c4, b44, b04, b14); \
		MOV64(b04, c0); \
		MOV64(b14, c1); \
		MOV64(b24, c2); \
		MOV64(b34, c3); \
		MOV64(b44, c4); \
	}

#define IOTA(r)   XOR64_IOTA(a00, a00, r)

#define KF_ELT01(k)   { \
		THETA ( a00, a01, a02, a03, a04, a10, a11, a12, a13, a14, a20, a21, \
	              a22, a23, a24, a30, a31, a32, a33, a34, a40, a41, a42, a43, a44 ); \
		RHO ( a00, a01, a02, a03, a04, a10, a11, a12, a13, a14, a20, a21, \
	              a22, a23, a24, a30, a31, a32, a33, a34, a40, a41, a42, a43, a44 ); \
		KHI ( a00, a30, a10, a40, a20, a11, a41, a21, a01, a31, a22, a02, \
	              a32, a12, a42, a33, a13, a43, a23, a03, a44, a24, a04, a34, a14 ); \
		IOTA(k); \
	}

#define P1_TO_P0   { \
		DECL64(t); \
		MOV64(t, a01); \
		MOV64(a01, a30); \
		MOV64(a30, a33); \
		MOV64(a33, a23); \
		MOV64(a23, a12); \
		MOV64(a12, a21); \
		MOV64(a21, a02); \
		MOV64(a02, a10); \
		MOV64(a10, a11); \
		MOV64(a11, a41); \
		MOV64(a41, a24); \
		MOV64(a24, a42); \
		MOV64(a42, a04); \
		MOV64(a04, a20); \
		MOV64(a20, a22); \
		MOV64(a22, a32); \
		MOV64(a32, a43); \
		MOV64(a43, a34); \
		MOV64(a34, a03); \
		MOV64(a03, a40); \
		MOV64(a40, a44); \
		MOV64(a44, a14); \
		MOV64(a14, a31); \
		MOV64(a31, a13); \
		MOV64(a13, t); \
	}

void
sha3_init(sha3_context *sha3Ctx)
{
	int i;

#pragma unroll
	for (i = 0; i < 25; i ++)
		sha3Ctx->u.wide[i] = 0;
	/*
	 * Initialization for the "lane complement".
	 */
	sha3Ctx->u.wide[ 1] = SPH_C64(0xFFFFFFFFFFFFFFFFL);
	sha3Ctx->u.wide[ 2] = SPH_C64(0xFFFFFFFFFFFFFFFFL);
	sha3Ctx->u.wide[ 8] = SPH_C64(0xFFFFFFFFFFFFFFFFL);
	sha3Ctx->u.wide[12] = SPH_C64(0xFFFFFFFFFFFFFFFFL);
	sha3Ctx->u.wide[17] = SPH_C64(0xFFFFFFFFFFFFFFFFL);
	sha3Ctx->u.wide[20] = SPH_C64(0xFFFFFFFFFFFFFFFFL);

	//set word capacity. To roll with SHA3-512,384,??? you'll change this.
	//256:: // sha3Ctx->wordCapacity =  2 * 256 / (8 * (int) sizeof(ulong));
	sha3Ctx->wordCapacity =  512 / (8 * (int) sizeof(ulong));
}
ulong swapLong(void *X) {
	ulong x = (ulong) X;
	x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
	x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
	x = (x & 0x00FF00FF00FF00FF) << 8  | (x & 0xFF00FF00FF00FF00) >> 8;
	return x;
}
ulong swapLongAMD(ulong X) {
	ulong x = (ulong) X;
	ulong xb = (ulong) X;
	x = (x & 0x00000000FFFFFFFF) << 32 | (xb & 0xFFFFFFFF00000000) >> 32;
	x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
	x = (x & 0x00FF00FF00FF00FF) << 8  | (x & 0xFF00FF00FF00FF00) >> 8;
	return x;
}

void sha3_update(sha3_context *sha3Ctx, const void *data, int inputLen, int bitCapacity, ulong delimitedSuffix)
{

	unsigned char *buf;
	buf = sha3Ctx->buf;

	size_t words = (size_t) inputLen / sizeof(ulong);
	size_t tail = (size_t) inputLen - (words * sizeof(ulong));

	size_t wordIndex = 0;
	size_t byteIndex = 0;
	int caps = bitCapacity / (8 * (int) sizeof(ulong)); //sha3Ctx->wordCapacity;

	int clen = (int) inputLen;
	//set buffer from input data

	#pragma unroll
		for (int i = 0; i < clen; i++) buf[i] = ((const unsigned char *)data)[i];
		data = (const unsigned char *)data + clen;


	#pragma unroll
	for(size_t j = 0; j < words; j++, buf += sizeof(ulong)) {
		ulong t = (ulong) (buf[0]) |
			((ulong) (buf[1]) << 8 * 1) |
			((ulong) (buf[2]) << 8 * 2) |
			((ulong) (buf[3]) << 8 * 3) |
			((ulong) (buf[4]) << 8 * 4) |
			((ulong) (buf[5]) << 8 * 5) |
			((ulong) (buf[6]) << 8 * 6) |
			((ulong) (buf[7]) << 8 * 7);
		sha3Ctx->u.wide[wordIndex] ^= t;

		int lim = (int) 24 - caps;
		if(++wordIndex == (size_t) lim) {
			//first run f1600
			#pragma unroll
			for (int jj = 0; jj < 24; jj ++) {
				KF_ELT01(RC[jj + 0]);
				P1_TO_P0;
			}
			wordIndex = 0;
		}
	}
  #pragma unroll
	  ulong derps = 0x0; //we'll overwrite sha3Ctx->saved with derps since it doesnt zero out here anyway
	  while(tail--){
	  	ulong adds = (ulong) (*(buf++)) << ((byteIndex++) * 8);
      derps |= adds;

	  }
	sha3Ctx->saved = derps; //like so

	//ok, now the part that nobody remembers in other openCL implementations out there:
	//add the proper effing padding to determine fips sha3 vs keccak here

	//ulong pad = (sha3Ctx->saved ^ ((ulong) ((ulong) (0x02 | (1 << 2)) << ((byteIndex) * 8))));

	//ulong delimeter = 0x06; //sha3 - 256
	//ulong delimeter = 0x1f; //shake256
	//ulong delimeter = 1; //keccak256, not sha3-fips202

	ulong pad = (sha3Ctx->saved ^ ((ulong) (delimitedSuffix << (byteIndex * 8))));
	sha3Ctx->u.wide[wordIndex] ^= pad;

	int lim = 24 - caps;
	sha3Ctx->u.wide[lim] ^= 0x8000000000000000UL;

	//ok now run F1600:
	#pragma unroll

	for (int jj = 0; jj < 24; jj ++) {
		KF_ELT01(RC[jj + 0]);
		P1_TO_P0;
	}


}

void cshake_update(sha3_context *sha3Ctx, const void *data, int inputLen, int bitCapacity, ulong delimitedSuffix, int padPos)
{

	unsigned char *buf;
	buf = sha3Ctx->buf;

	size_t words = (size_t) inputLen / sizeof(ulong);
	size_t tail = (size_t) inputLen - (words * sizeof(ulong));

	size_t wordIndex = 0;
	size_t byteIndex = 0;
	int caps = bitCapacity / (8 * (int) sizeof(ulong)); //sha3Ctx->wordCapacity;

	int clen = (int) inputLen;
	//set buffer from input data

	uint gid = get_global_id(0);
	//printf("\nclen %i %i",clen,sizeof(ulong));
	#pragma unroll
		for (int i = 0; i < clen; i++){
		  buf[i] = ((const unsigned char *)data)[i];
		  //printf("\nset buf char from data %016llx %i",buf[i],i);
		}


	if(padPos >= 0){
		buf[clen] ^= 0x0000000000000004UL;
	}
	if(padPos == -5){
		buf[136] ^= 0x0000000000000004UL;
	}
	if(padPos == -3){
		buf[136] ^= 0x0000000000000004UL;
	}

	//data = (const unsigned char *)data + clen;
	data = (ulong*)data;
	/*if(padPos == -2)
		printf("\n words len %i %i",words,clen);*/

	#pragma unroll
		for(size_t j = 0; j < words; j++, buf += sizeof(ulong)) {
			ulong t;
			if(padPos != -2){
			t = (ulong) (buf[0]) |
	      ((ulong) (buf[1]) << 8 * 1) |
	      ((ulong) (buf[2]) << 8 * 2) |
	      ((ulong) (buf[3]) << 8 * 3) |
	      ((ulong) (buf[4]) << 8 * 4) |
	      ((ulong) (buf[5]) << 8 * 5) |
	      ((ulong) (buf[6]) << 8 * 6) |
	      ((ulong) (buf[7]) << 8 * 7);
	    }
			if(padPos == -2){
				t = ((ulong*) data)[j];
				/*if((int) j > words-8){
					printf("\n resetting end char %i %i",j*8,clen);
					t = (ulong) 0x0;
				}
				else{
					printf("\n not resetting end char %i %i",j,clen,t);
				}*/
			}
			sha3Ctx->u.wide[wordIndex] ^= t;
			/*if(padPos == -2)
				printf("\n word was set in cshake update %016llx %016llx %i",sha3Ctx->u.wide[wordIndex],t,wordIndex);*/
			int lim = (int) 24 - caps;
			lim = caps; //reset to 17 (=136/8 aka (1600-512)/8*8);

			if(++wordIndex == (size_t) lim) {
				//first run f1600

				#pragma unroll
				for (int jj = 0; jj < 24; jj ++) {
					KF_ELT01(RC[jj + 0]);
					P1_TO_P0;
				}
				/*printf("\n permutation was done here");
				for(int i=0;i<16;i++){
					printf("\n word was set in f1600 %016llx %i",sha3Ctx->u.wide[i],i);

				}*/
				wordIndex = 0;


			}
		}

}

static void sha3_finalize(sha3_context *sha3Ctx, void *dst, int inputLen)
	{
		union {
			unsigned char tmp[200 + 1]; //again, more chars than we need but thats ok
			ulong dummy;   /* for alignment */
		} u;
		size_t j;
		/* Finalize the "lane complement" */
		sha3Ctx->u.wide[ 1] = ~sha3Ctx->u.wide[ 1];
		sha3Ctx->u.wide[ 2] = ~sha3Ctx->u.wide[ 2];
		sha3Ctx->u.wide[ 8] = ~sha3Ctx->u.wide[ 8];
		sha3Ctx->u.wide[12] = ~sha3Ctx->u.wide[12];
		sha3Ctx->u.wide[17] = ~sha3Ctx->u.wide[17];
		sha3Ctx->u.wide[20] = ~sha3Ctx->u.wide[20];


		for (j = 0; j < 64; j += 8){
			enc64le_aligned(((uchar*)dst) + j, sha3Ctx->u.wide[j >> 3]);
		}
	}


void SHA3FIPS202(__global char* in, __global ulong* out, __global int* inputLen, __global int* capacity, __global ulong* delimitedSuffix) {
	/*most of this sha3 stuff predates cBlake. Leaving for futurism initiatives.*/
	uint id = get_global_id(0);

	int iLen = (int) inputLen[id];
	sha3_context	 ctx_sha3;
	char data[200]; //doesnt matter RE this len since we cant do dynamic, we just set to something large. update someday i suppose....
	ulong hash[8];

	for (int i = 0; i < iLen; i++) {
		data[i] = in[i];
	}
	ulong delSuf = delimitedSuffix[id];
	int caps = capacity[id];
	sha3_init(&ctx_sha3);
	sha3_update(&ctx_sha3, data,iLen, caps, delSuf);
	sha3_finalize(&ctx_sha3, hash,iLen);

	for (int i = 0; i < 8; i++) {
		out[(id * 8)+i] = hash[i];
	}
}

static unsigned int left_encode( unsigned char * encbuf, int value )
{
    unsigned int n, i;
    int v;

    for ( v = value, n = 0; v && (n < 8); ++n, v >>= 8 )
        ; /* empty */
    if (n == 0)
        n = 1;

    for ( i = 1; i <= n; ++i )
    {
    	encbuf[i] = (unsigned char)(value >> (8 * (n-i)));
    }
    encbuf[0] = (unsigned char)n;
    return n + 1;
}

static unsigned int right_encode( unsigned char * encbuf, size_t value )
{
    unsigned int n, i;
    size_t v;

    for ( v = value, n = 0; v && (n < sizeof(size_t)); ++n, v >>= 8 )
        ; /* empty */
    if (n == 0)
        n = 1;
    for ( i = 1; i <= n; ++i )
    {
        encbuf[i-1] = (unsigned char)(value >> (8 * (n-i)));
    }
    encbuf[n] = (unsigned char)n;
    return n + 1;
}
static void kmac_permute(sha3_context* sha3Ctx){

	#pragma unroll

	for (int jj = 0; jj < 24; jj ++) {
		KF_ELT01(RC[jj + 0]);
		P1_TO_P0;
	}

}
void KMAC(unsigned char* in, ulong* out, int* inputLen, int* outputLen, unsigned char* key, int* keyLen, unsigned char* customization, int* customizationLen) {
	uint id = get_global_id(0);
	unsigned char encbuf[8+1];
	#pragma unroll
	for(int i=0;i<8+1;i++){
		encbuf[i] = 0;
	}
	int iLen = (int) inputLen;
	sha3_context	 ctx_sha3;
	ulong hash[16];

	ulong delSuf = 0x1f;
	int caps = 512;
	int rateInBytes = (1600 - caps) / 8;


	sha3_init(&ctx_sha3);
	int keyLength = (int) keyLen;

	ulong newinlen = (ulong) ( (1600-caps)/8 );
	unsigned char initBlock[((1600-512)/8)];
	unsigned char t3[(((1600-512)/8)) * 2]; //riffraff hardcoding this to 2... 2 = 208 (416 chars of block header)

	unsigned char t[(1600-512)/8];//136 chars

	#pragma unroll
	for(int i=0;i<((1600-512)/8);i++){
		initBlock[i] = 0;
		t[i] = 0;
	}
	#pragma unroll
	for(int i=0;i<136*2;i++){
		t3[i] = 0;
	}
	int outLength = (int) outputLen;
	ulong outbits = (ulong) outLength;
	ulong keybits = (ulong) keyLength*8;

	int eb0 = left_encode(encbuf,rateInBytes);


	ulong cstm = 0x0;
	//the real kmac init happens here
	initBlock[0]  = 0x01;
	initBlock[1]  = 0x88;
	initBlock[2]  = 0x01;
	initBlock[3]  = 32;
	initBlock[4]  = 0x4b;  // "KMAC"
	initBlock[5]  = 0x4d;
	initBlock[6]  = 0x41;
	initBlock[7]  = 0x43;
	initBlock[8]  = 0x01;
	initBlock[9]  = 0x0;//16;    // fixed bitlen of cstm
	initBlock[10] = (ulong) cstm & 0xff;
	initBlock[11] = (ulong) cstm >> 8;

	//now our block of data
	t[0] = 0x01;
	t[1] = 0x88;
	int inBitsLength = keyLength* 8;
	int eb2 = left_encode(encbuf,inBitsLength); //length in bits of in
	t[2] = encbuf[0];
	t[3] = encbuf[1];
	t[4] = 0x0;

	#pragma unroll
	for(int i=0;i<iLen;i++){
		t3[i] = in[i];
	}
	int padPlacement = 4;
	if(keyLength >= 32){
		padPlacement = 5;
		//for some reason if I have a tiny key we need an extra char of padding
		//not a problem in our case unless the nonces go to like 1-3 chars vs the current 64, ha
	}
	#pragma unroll
	for(int i=0;i<keyLength;i++){
		t[(padPlacement+i)] = key[i];
	}

	int eb3 = left_encode(encbuf,512);

	t3[(iLen)] = 0x1;//OMFG set this to 0x02 for effing CSHAKE512!?!?!?!?!?!
	t3[(iLen+1)] = 0x0;//encbuf[0];
	t3[(iLen+2)] = encbuf[0];//512 & 0xff;//encbuf[1];
	t3[(iLen+3)] = 0x4;


	//unsigned char fbuf[136*2]; //if header sizes go up past 136*2 increase me


	//unsigned char* f = (unsigned char*) fbuf;
	/*unsigned char f[(((1600-512)/8)) * 2];

	ulong* f0[1];
	#pragma unroll
	for(int i=0;i<iLen;i++){

		f[i] = t3[i];

	}

	f[iLen+0] = t3[(iLen+0)];//512 & 0xff;//encbuf[1];
	f[iLen+1] = t3[(iLen+1)];//(ulong*) encbuf[1];
	f[iLen+2] = t3[iLen+2] ;
	f[iLen+3] = t3[iLen+3] ;
	#pragma unroll
	for(int i=iLen+4;i<136*2;i++){
		f[i] = (unsigned char*) 0x00;
	}

	f[(1600-512)/8*2-1] = 128; //last bit of padding
	*/

	t3[(1600-512)/8*2-1] = 128; //last bit of padding

	//absorb init block
	cshake_update(&ctx_sha3, (ulong*) initBlock,136, 1600-512, 0x04, 1 );
	/*for(int i=0;i<8;i++){
		printf("\nfinish cshake init char b0 %i %016llx\n",i,ctx_sha3.u.wide[i]);
	}*/

	//absorb input block (data)
	ulong* t2Block = (ulong*) t;

	cshake_update(&ctx_sha3, t2Block, 136, 1600-512, 0x04, -3);
	/*for(int i=0;i<8;i++){
		printf("\nfinish cshake update data b1 %i %016llx\n",i,ctx_sha3.u.wide[i]);
	}*/

	ulong* fBlock = (ulong*) t3;
	cshake_update(&ctx_sha3, fBlock,136*2, 1600-512,  0x04, -2 );

	/*for(int i=0;i<8;i++){
		printf("\nfinish cshake update data b2 %i %016llx\n",i,ctx_sha3.u.wide[i]);
	}*/

	//only would need kmac_permute after blocks with an input len > 136 chars block
	//kmac_permute(&ctx_sha3);

	sha3_finalize(&ctx_sha3, hash,newinlen);
	#pragma unroll
	for (int i = 0; i < 16; i++) {
		out[i] = hash[i];
		//printf("\nkmac out char %i %016llx",i,hash[i]);
	}
	return;
}

/***********
*blake 2b things
***********/

#if __ENDIAN_LITTLE__
  #define SPH_LITTLE_ENDIAN 1
#else
  #define SPH_BIG_ENDIAN 1
#endif

#define SPH_UPTR sph_u64

typedef unsigned int sph_u32;
typedef int sph_s32;
#ifndef __OPENCL_VERSION__
  typedef unsigned long long sph_u64;
  typedef long long sph_s64;
#else
  typedef unsigned long sph_u64;
  typedef long sph_s64;
#endif

#define SPH_64 1
#define SPH_64_TRUE 1

#define SWAP4(x) as_uint(as_uchar4(x).wzyx)
#define SWAP8(x) as_ulong(as_uchar8(x).s76543210)

#if SPH_BIG_ENDIAN
  #define DEC64E(x) (x)
  #define DEC64BE(x) (*(const sph_u64 *) (x));
  #define DEC32LE(x) SWAP4(*(const sph_u32 *) (x));
#else
  #define DEC64E(x) SWAP8(x)
  #define DEC64BE(x) SWAP8(*(const sph_u64 *) (x));
  #define DEC64LE(x) (*(const sph_u64 *) (x));
  #define DEC32LE(x) (*(const sph_u32 *) (x));
#endif

inline static uint2 ror64(const uint2 x, const uint y)
{
    return (uint2)(((x).x>>y)^((x).y<<(32-y)),((x).y>>y)^((x).x<<(32-y)));
}

inline static ulong ror64_3(const ulong x, const uint y)
{
	return (ulong)  ((x) << (y)) | ((x) >> (64U - (y)));
}
inline static uint2 ror64_2(const uint2 x, const uint y)
{
    return (uint2)(((x).y>>(y-32))^((x).x<<(64-y)),((x).x>>(y-32))^((x).y<<(64-y)));
}

__constant static const uchar blake2b_sigma[12][16] = {
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
  { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } ,
  { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 } ,
  {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 } ,
  {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 } ,
  {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 } ,
  { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 } ,
  { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 } ,
  {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 } ,
  { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13 , 0 } ,
  {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 } ,
  { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 } };

void blake2b(unsigned char* block, ulong* output, unsigned int* inputLen, ulong* inputKey, unsigned char* keyLen) {
	sph_u32 gid = get_global_id(0);
	ulong m[16];
	const int debugBlake = 0;
	ulong inLength = (ulong) inputLen;
	int keyLength = (int) keyLen;
	ulong* ins = (ulong*) inputKey;
	#pragma unroll
	for(int i=0;i<16;i++){

		if(i <= 3){
			ulong keyinp = inputKey[i];//DEC64LE((inputKey+(i*8)));//dec64le_aligned((unsigned char*)(inputKey+(i*8)));
			if(debugBlake == 1) {
				printf("\nog input is %i %016llx\n",i,keyinp);
			}
			m[i] = keyinp;

		}
		if(i > 3){
			m[i] = 0x0;
		}
	}



	ulong dynV[16] = { 0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
	                0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179,
	                0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
	                0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0xe07c265404be4294, 0x5be0cd19137e2179 };
	ulong ttt[16];
	ulong* v = (ulong*)ttt;
	v[0] = (ulong) 0x6a09e667f3bcc908;
	v[1] = (ulong) 0xbb67ae8584caa73b;
	v[2] = (ulong) 0x3c6ef372fe94f82b;
	v[3] = (ulong) 0xa54ff53a5f1d36f1;
	v[4] = (ulong) 0x510e527fade682d1;
	v[5] = (ulong) 0x9b05688c2b3e6c1f;
	v[6] = (ulong) 0x1f83d9abfb41bd6b;
	v[7] = (ulong) 0x5be0cd19137e2179;
	v[8] = (ulong) 0x6a09e667f3bcc908;
	v[9] = (ulong) 0xbb67ae8584caa73b;
	v[10] = (ulong) 0x3c6ef372fe94f82b;
	v[11] = (ulong) 0xa54ff53a5f1d36f1;
	v[12] = (ulong) 0x510e527fade682d1;
	v[13] = (ulong) 0x9b05688c2b3e6c1f;
	v[14] = (ulong) 0xe07c265404be4294;
	v[15] = (ulong) 0x5be0cd19137e2179;


	unsigned int* word0Padding[8];
	word0Padding[0] = 32; //target out length, pop this val to 64 to have blake2b-512 oooOOOOooO
	word0Padding[1] = keyLength; //key length
	word0Padding[2] = 1;
	word0Padding[3] = 1;
	word0Padding[4] = 0;
	word0Padding[5] = 0;
	word0Padding[6] = 0;
	word0Padding[7] = 0;

	const ulong *p = ( const ulong * )word0Padding;
	/*ulong barf = (ulong) (( ulong )( p[0] & 0x00000000000000FF ) <<  0) |
     (( ulong )( p[1] & 0x00000000000000FF ) <<  8) |
     (( ulong )( p[2] & 0x00000000000000FF ) << 16) |
     (( ulong )( p[3] & 0x00000000000000FF ) << 24) |
     (( ulong )( p[4] & 0x00000000000000FF ) << 32) |
     (( ulong )( p[5] & 0x00000000000000FF ) << 40) |
     (( ulong )( p[6] & 0x00000000000000FF ) << 48) |
     (( ulong )( p[7] & 0x00000000000000FF ) << 56) ;*/
  ulong tp = (ulong)(p[0] >>24 & 0x000000000000FF00 | p[0] & 0x00000000000000FF | p[1] >> 16 & 0x0000000000FF0000 | p[1] << 24 & 0x00000000FF000000);
  ulong t = tp; //padding on first 64 bit word
	ulong barf = t;
	v[0] ^= barf;
	v[12] ^= (ulong) 128; //total key len
	v[14] ^= 0xFFFFFFFFFFFFFFFFUL; //pad for end of block already, were antsy
	/*ulong ogV[16] = { 0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
	                0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179,
	                0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
	                0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0xe07c265404be4294, 0x5be0cd19137e2179 };*/
	ulong tttt[16];
	ulong* ogV = (ulong*)tttt;
	ogV[0] = (ulong) 0x6a09e667f3bcc908;
	ogV[1] = (ulong) 0xbb67ae8584caa73b;
	ogV[2] = (ulong) 0x3c6ef372fe94f82b;
	ogV[3] = (ulong) 0xa54ff53a5f1d36f1;
	ogV[4] = (ulong) 0x510e527fade682d1;
	ogV[5] = (ulong) 0x9b05688c2b3e6c1f;
	ogV[6] = (ulong) 0x1f83d9abfb41bd6b;
	ogV[7] = (ulong) 0x5be0cd19137e2179;
	ogV[8] = (ulong) 0x6a09e667f3bcc908;
	ogV[9] = (ulong) 0xbb67ae8584caa73b;
	ogV[10] = (ulong) 0x3c6ef372fe94f82b;
	ogV[11] = (ulong) 0xa54ff53a5f1d36f1;
	ogV[12] = (ulong) 0x510e527fade682d1;
	ogV[13] = (ulong) 0x9b05688c2b3e6c1f;
	ogV[14] = (ulong) 0xe07c265404be4294;
	ogV[15] = (ulong) 0x5be0cd19137e2179;
	ogV[0] ^= barf;

	//printf("\n barf components %016llx %016llx %016llu",tp,v[2],v[2]);
	//printf("\n before round b %016llx",v[0]);
	//printf("\n before round b %016llx",v[1]);
	//printf("\n before round b %016llx",v[2]);


#define G(r,i,a,b,c,d) \
	a = a + b + m[ blake2b_sigma[r][2*i] ]; \
	((uint2*)&d)[0] = ((uint2*)&d)[0].yx ^ ((uint2*)&a)[0].yx; \
	c = c + d; \
	((uint2*)&b)[0] = ror64( ((uint2*)&b)[0] ^ ((uint2*)&c)[0], 24U); \
	a = a + b + m[ blake2b_sigma[r][2*i+1] ]; \
	((uint2*)&d)[0] = ror64( ((uint2*)&d)[0] ^ ((uint2*)&a)[0], 16U); \
	c = c + d; \
    ((uint2*)&b)[0] = ror64_2( ((uint2*)&b)[0] ^ ((uint2*)&c)[0], 63U);

#define ROUND(r)                    \
	G(r,0,v[ 0],v[ 4],v[ 8],v[12]); \
    G(r,1,v[ 1],v[ 5],v[ 9],v[13]); \
    G(r,2,v[ 2],v[ 6],v[10],v[14]); \
    G(r,3,v[ 3],v[ 7],v[11],v[15]); \
    G(r,4,v[ 0],v[ 5],v[10],v[15]); \
    G(r,5,v[ 1],v[ 6],v[11],v[12]); \
    G(r,6,v[ 2],v[ 7],v[ 8],v[13]); \
    G(r,7,v[ 3],v[ 4],v[ 9],v[14]); \

  if(debugBlake == 1) {
    for(int i=0;i<16;i++){
			printf("\n before round 0 %016llu",v[i]);
		}

	}
	int bytesRolled = 0;

	ROUND( 0 );
	ROUND( 1 );
	ROUND( 2 );
	ROUND( 3 );
	ROUND( 4 );
	ROUND( 5 );
	ROUND( 6 );
	ROUND( 7 );
	ROUND( 8 );
	ROUND( 9 );
	ROUND( 10 );
	ROUND( 11 );

	if(debugBlake == 1) {
		for(int i=0;i<8;i++){
			printf("\n after round 0 %016llu",v[i]);
		}
	}

	#pragma unroll
	for(int i=0;i<8;i++){
		v[i] = ogV[i] ^ v[i] ^ v[i+8];
	}

	v[ 8] = ogV[0] ^ barf;
	v[ 9] = ogV[1];
	v[10] = ogV[2];
	v[11] = ogV[3];
	v[12] = ogV[4] ^ (ulong) (128+128);
	v[13] = ogV[5] ^ (ulong) 0x0;
	v[14] = ogV[6] ^ (ulong) 0x0;
	v[15] = ogV[7] ^ (ulong) 0x0;



	ulong lastV[16];
	#pragma unroll
	for(int i=0;i<16;i++){
		ulong inp = DEC64LE((block+(bytesRolled*8)));
		m[i] = inp;
		lastV[i] = v[i];
		bytesRolled += 1;
	} //first 128 bytes
	if(debugBlake == 1) {
		for(int i=0;i<8;i++){
			printf("\n before round 128 1 %016llu",v[i]);
		}
	}

	ROUND( 0 );
	ROUND( 1 );
	ROUND( 2 );
	ROUND( 3 );
	ROUND( 4 );
	ROUND( 5 );
	ROUND( 6 );
	ROUND( 7 );
	ROUND( 8 );
	ROUND( 9 );
	ROUND( 10 );
	ROUND( 11 );

	if(debugBlake == 1) {
		for(int i=0;i<8;i++){
			//v[i] = lastV[i] ^ v[i] ^ v[i+8];
			printf("\n done round 128 1 %016llu",v[i]);
		}
	}

	#pragma unroll
	for(int i=0;i<8;i++){
		v[i] = lastV[i] ^ v[i] ^ v[i+8];
	}

	v[ 8] = ogV[0] ^ barf;
	v[ 9] = ogV[1];
	v[10] = ogV[2];
	v[11] = ogV[3];
	v[12] = ogV[4] ^ (ulong) (128+208);//208 being block + length
	v[13] = ogV[5] ^ (ulong) 0x0;
	v[14] = ogV[6] ^ (ulong) -1;
	//v[14] = ogV[6] ^ (ulong) 0x0; //normal block gets 0x0, last gets -1
	v[15] = ogV[7] ^ (ulong) 0x0;

	#pragma unroll
	for(int i=0;i<16;i++){
		ulong inp = DEC64LE((block+(bytesRolled*8)));
		m[i] = inp;
		lastV[i] = v[i];
		bytesRolled += 1;
	} //second 128 bytes
	for(int i=10;i<16;i++){
		m[i] = (ulong) 0x0; //wtf amd!?
	}

	if(debugBlake == 1) {
		for(int i=0;i<8;i++){
			//v[i] = lastV[i] ^ v[i] ^ v[i+8];
			printf("\n before round 128 2 %016llu %016llu",v[i],lastV[i]);
		}
	}

	ROUND( 0 );
	ROUND( 1 );
	ROUND( 2 );
	ROUND( 3 );
	ROUND( 4 );
	ROUND( 5 );
	ROUND( 6 );
	ROUND( 7 );
	ROUND( 8 );
	ROUND( 9 );
	ROUND( 10 );
	ROUND( 11 );

	if(debugBlake == 1) {
		for(int i=0;i<8;i++){
			printf("\n after round 2 %016llu",v[i]);
		}
	}

/*********
should the block size ever increase just fill into these blocks,
leaving them here for now just in case things change anytime soon...
:::
add the -1 padding to the last block like
v[14] = ogV[6] ^ (ulong) -1;

******/
/*begin round 3:::
	#pragma unroll
	for(int i=0;i<8;i++){
		v[i] = lastV[i] ^ v[i] ^ v[i+8];
	}

	v[ 8] = ogV[0] ^ barf;
	v[ 9] = ogV[1];
	v[10] = ogV[2];
	v[11] = ogV[3];
	v[12] = ogV[4] ^ (ulong) (128+384);
	v[13] = ogV[5] ^ (ulong) 0x0;
	v[14] = ogV[6] ^ (ulong) 0x0;
	v[15] = ogV[7] ^ (ulong) 0x0;

	#pragma unroll
	for(int i=0;i<16;i++){
		ulong inp = DEC64LE((block+(bytesRolled*8)));
		//printf("\nmessage input two %i %016llx",i,inp);
		m[i] = inp;
		lastV[i] = v[i];
		bytesRolled += 1;
	} //third 128 bytes
*/

	/*for(int i=0;i<8;i++){
		printf("\n before round 3 %016llu",v[i]);
	}*/

	/*ROUND( 0 );
	ROUND( 1 );
	ROUND( 2 );
	ROUND( 3 );
	ROUND( 4 );
	ROUND( 5 );
	ROUND( 6 );
	ROUND( 7 );
	ROUND( 8 );
	ROUND( 9 );
	ROUND( 10 );
	ROUND( 11 );*/

	/*for(int i=0;i<8;i++){
		printf("\n after round 3 %016llu",v[i]);
	}*/

/*end round 3:::::::
	#pragma unroll
	for(int i=0;i<8;i++){
		v[i] = lastV[i] ^ v[i] ^ v[i+8];
	}
*/

/*final round::
	v[ 8] = ogV[0] ^ barf;
	v[ 9] = ogV[1];
	v[10] = ogV[2];
	v[11] = ogV[3];
	v[12] = ogV[4] ^ (ulong) (128+512-96); //-96 being length from 512 our 416 chars are
	v[13] = ogV[5] ^ (ulong) 0x0;
	v[14] = ogV[6] ^ (ulong) -1;
	v[15] = ogV[7] ^ (ulong) 0x0;

	#pragma unroll
	for(int i=0;i<16;i++){
		ulong inp = DEC64LE((block+(bytesRolled*8)));
		//printf("\nmessage input three %i %016llx %016llu",i,inp);
		m[i] = inp;
		lastV[i] = v[i];
		bytesRolled += 1;
	} //fourth 128 bytes
*/
	/*printf("\n bytes rolled %i\n",bytesRolled);
	for(int i=0;i<16;i++){
		printf("\n before round 4 %016llu",v[i]);
	}*/
/*
	ROUND( 0 );
	ROUND( 1 );
	ROUND( 2 );
	ROUND( 3 );
	ROUND( 4 );
	ROUND( 5 );
	ROUND( 6 );
	ROUND( 7 );
	ROUND( 8 );
	ROUND( 9 );
	ROUND( 10 );
	ROUND( 11 );
*/
	/*for(int i=0;i<8;i++){
		printf("\n after final round %016llu %016llu",v[i],lastV[i] ^ v[i] ^ v[i+8]);
	}*/

/****END EXTRA BLOCK STUFF FOR POSSIBLE FUTURISM******/

#undef G
#undef ROUND
	#pragma unroll
	for(int i=0;i<8;i++){
		output[i] = lastV[i] ^ v[i] ^ v[i+8];
		//printf("\nfinal output char %i %016llx\n",i,output[i]);
	}

}


/*****
and now the cBlake goodies
*******/

kernel void cBlake(__constant unsigned char* in, __global ulong* out, __constant int* inputLen, __constant int* outputLen, __constant unsigned char* key, __constant int* keyLen, __constant ulong* hashTarget, __global int* hasFoundNonce, __global ulong* nonceOutput, __constant ulong* nonceBaseAsLong){

	int gid = (int) get_global_id(0);
	int lid = (int) get_local_id(0);
	const int debugKMAC = 0;
	const int clMinerThreads = 1;
	const int localThreads = 128;
	unsigned char inputBuf[512];
	unsigned char* input = inputBuf;
	unsigned char keyBuf[32];
	unsigned char* keyBytes = keyBuf;

	int* inLength = (int*) inputLen[0];
	int* keyLength = (int*) keyLen[0];
	int* outLength = (int*) outputLen[0];
	int doBlake = 0;
	int il = inputLen[0];
	int kl = (int)keyLen[0];
	ulong nonceInBuf[8];
	ulong* nonceIn = nonceInBuf;
	ulong htIn[8];
	ulong* hashTarget64 = htIn;

	ulong htz = (ulong*) hashTarget[0];
	hashTarget64[0] = (ulong*) htz;
	//hashTarget64[0] = swapLong(htz);

	#pragma unroll
	for(int i=0;i<32;i++){
		if(i < kl){
			keyBytes[i] = (unsigned char*) key[i];
			keyBytes[0] += i;
		}
		else{
			keyBytes[i] = 0;
		}

		if(i < 8){
			nonceIn[i] = nonceBaseAsLong[i];
			nonceIn[i] = swapLongAMD(nonceIn[i]);
			//nonceIn[i] = swapLong(nonceIn[i]);
		}
	}

	if(debugKMAC == 1){
		printf("\n noncein as long %016llx %016llx %016llx %016llx",nonceIn[0],nonceIn[1],nonceIn[2],nonceIn[7]);
	}
	ulong nonceLong = (ulong) nonceIn[0];//0x0000000000576193; //hard code here to test things too
	//printf("\nnoncelong is init %016llx",nonceLong);
	int nonceOffset = 0;
	if(nonceLong == (ulong) 0xFFFFFFFFFFFFFFFF){
		nonceLong = (ulong*) nonceIn[1];
		nonceOffset = 4;
		if(nonceLong == (ulong) 0xFFFFFFFFFFFFFFFF){
			nonceLong = (ulong*) nonceIn[2];
			nonceOffset = 8;
		}
	}

	//AMD might notlike this?
	unsigned char * p = (unsigned char*)&nonceLong;
	keyBytes[0+nonceOffset] = p[0];
	keyBytes[1+nonceOffset] = p[1];
	keyBytes[2+nonceOffset] = p[2];
	keyBytes[3+nonceOffset] = p[3];
	keyBytes[4+nonceOffset] = p[4];
	keyBytes[5+nonceOffset] = p[5];
	keyBytes[6+nonceOffset] = p[6];
	keyBytes[7+nonceOffset] = p[7];

	//printf("\n noncelong post step 1 %016llx",nonceLong);

	#pragma unroll
	for(int i=0;i<il;i++){
		input[i] = in[i];
	}

	unsigned char fakeC[0];
	unsigned char* customization = fakeC;
	int* blakeInLength = (int*) 64;
	ulong output[8];
	ulong* outputBuf = (ulong*) output;
	int* clen = (int*) 0;
	int hasFound = 0;

	int jj = 0;
	//trying loop inside here to get extra local perf
	//#pragma unroll
	//for(int jj=0;jj<clMinerThreads;jj++){
		//if(hasFound == 0){
			ulong blakeBuf[16];
			char blakeChars[64];
			char* blakeBuffer = blakeChars;
			ulong* blakeIn = (ulong*) blakeBuf;

			/*if(nonceLong == 0xFFFFFFFFFFFFFFFF){
				//alright we should be using the next long
				if(nonceOffset == 0){
					nonceOffset = 4;
					nonceLong = (ulong*) nonceIn[1];
				}
				if(nonceOffset == 4){
					//i dont think the nonces go this long but why waste cycles to compare here anyway..
					nonceOffset = 8;
					nonceLong = (ulong*) nonceIn[2];
				}
			}
			else{
				nonceOffset = 0;
				nonceLong = (ulong*) nonceIn[0];
			}*/
			//ulong* tt = swapLong((ulong*) nonceLong);
			//printf("\n noncelong pre add %016llx",nonceLong);
			ulong nonceAdd = /*(ulong)jj + */(ulong*)((int)gid * clMinerThreads);
			nonceLong = (ulong) ( nonceLong + (ulong) (0x0000000000000001 * nonceAdd)  );

			//nonceLong = 0x0000000000576193; //hard code a nonce if you want to debug, just make sure to stop incrementing on next line.

			//printf("\n nonce in %016llx noncelong %016llx nonceAdd %016llx gid %i lid %i jj %i",nonceBaseAsLong[0],nonceLong,nonceAdd,gid,lid,jj);
			//printf("\n noncelong was incremented %016llx %016llx",nonceLong,nonceAdd);

			//nonceLong = 0x0000000000fe58c5;
			p = (unsigned char*)&nonceLong;
			keyBytes[0+nonceOffset] = (unsigned char) p[0] & 0x00000000000000FF;
			keyBytes[1+nonceOffset] = (unsigned char) p[1] & 0x00000000000000FF;
			keyBytes[2+nonceOffset] = (unsigned char) p[2] & 0x00000000000000FF;
			keyBytes[3+nonceOffset] = (unsigned char) p[3] & 0x00000000000000FF;
			keyBytes[4+nonceOffset] = (unsigned char) p[4] & 0x00000000000000FF;
			keyBytes[5+nonceOffset] = (unsigned char) p[5] & 0x00000000000000FF;
			keyBytes[6+nonceOffset] = (unsigned char) p[6] & 0x00000000000000FF;
			keyBytes[7+nonceOffset] = (unsigned char) p[7] & 0x00000000000000FF;
			//printf("\n and p??? %016llx %016llx %02x %02x %02x %i",nonceLong,nonceIn[0],p[0],p[1],keyBytes[nonceOffset],nonceOffset);

			if(debugKMAC == 1){
				printf("\nnonce isset before kmac %016llx %02x%02x%02x%02x%02x%02x%02x%02x",nonceLong,keyBytes[0],keyBytes[1],keyBytes[2],keyBytes[3],keyBytes[4],keyBytes[5],keyBytes[6],keyBytes[7]);
			}


			KMAC(input,blakeIn,(int*) il, outLength, keyBytes, keyLength, customization, clen);

			ulong blakeEmpty[8];
			ulong* blakeInput = blakeEmpty;

			#pragma unroll
			for(int i=0;i<4;i++){
				blakeBuffer[i] = 0x0;
				ulong enc = blakeIn[i];
				blakeInput[i] = enc;

				if(debugKMAC == 1){
					printf("\n kmac out character gid %i lid %i algo iteration %i char %i:: %016llx",gid,lid,jj,i,enc);
				}

			}

			blakeBuffer = (ulong*) blakeInput;
			//blakeBuffer += 0;

			blake2b(input,outputBuf,(int*) 512, (ulong*) blakeIn,(ulong) 32); //64 being # of bytes incoming from blakeIn
			//printf("\n blake output 0 %016llx %016llx",(ulong*) outputBuf[0],outputBuf[1]);
			ulong bufOutCompare = outputBuf[0];//swapLongAMD((ulong*) outputBuf[0]);//swapLongAMD((ulong*) outputBuf[0]);//swapLong((ulong*) outputBuf[0]);
			ulong ht64Compare = hashTarget[0];//swapLongAMD(hashTarget[0]);
			ht64Compare >>= 8;
			//ht64Compare = swapLongAMD(ht64Compare);
			bufOutCompare = swapLongAMD(bufOutCompare);
			//printf("\n ht64 target %016llx %016llx %016llx",ht64Compare,bufOutCompare,outputBuf[0]);
			if( bufOutCompare <= ht64Compare){
				//printf("\n ht64 target %016llx %016llx %016llx",ht64Compare,bufOutCompare,outputBuf[0]);
				//printf("\n WE FOUND BUFFER");
				hasFoundNonce[0] = (int) 1;
				hasFound = 1;

				#pragma unroll
				for(int i=0;i<8;i++){
/*<<<<<<< HEAD*/
					out[i] = swapLongAMD(outputBuf[i]);
					nonceOutput[i] = 0x0;
/*=======
					out[i] = swapLong((ulong*) outputBuf[i]);
					nonceOutput[i] = (ulong) 0x0;
>>>>>>> 9b5daec941ec58c98a71546971bb966566d88bd9*/
				}
				if(nonceOffset == 0){
					nonceOutput[0] = swapLongAMD(nonceLong);
					nonceOutput[1] = 0x0;
					nonceOutput[2] = 0x0;
				}
				else if(nonceOffset == 4){
					nonceOutput[0] = nonceIn[0];
					nonceOutput[1] = swapLongAMD(nonceLong);
					nonceOutput[2] = 0x0;
				}
				else if(nonceOffset == 8){
					nonceOutput[0] = nonceIn[0];
					nonceOutput[1] = nonceIn[1];
					nonceOutput[2] = swapLongAMD(nonceLong);
				}
			}
		//}
	//}

	//in case one wants to choose the bestest nonce ever from this closed loop
	/*#pragma unroll
	for(int i=0;i<keyLength;i++){
		nonceOutput[(gid*32)+i] = keyBytes[i];
	}

	#pragma unroll
	for(int i=0;i<8;i++){
		out[(gid*8)+i] = swapLong((ulong*) outputBuf[i]);//outputBuf[i];
	}*/


}
