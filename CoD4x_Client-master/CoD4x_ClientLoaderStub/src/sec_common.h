/*
===========================================================================
    Copyright (C) 2010-2013  Ninja and TheKelm of the IceOps-Team

    This file is part of CoD4X17a-Server source code.

    CoD4X17a-Server source code is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    CoD4X17a-Server source code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
===========================================================================
*/



//#define Sec_Malloc(x) malloc(x)

void *Sec_Malloc(size_t size);
#define Sec_MemInit(n) void *_secmem[n]
#define Sec_GMalloc(type,n) (type *)Sec_Malloc(sizeof(type)*n)
#define Sec_SMalloc(n) Sec_GMalloc(char,n)

#define Sec_Free(x) if(x!=NULL) free(x); x=NULL

extern int SecCryptErr;
char *Sec_CryptErrStr(int);

void Sec_Init( void );
qboolean Sec_Initialized();

#define cod4xpem "-----BEGIN PUBLIC KEY-----\n\
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwu8nEsLD4sTP+Py30Fnq\n\
UOlgZZrGb7aIiQhn8iXAXXuhLC0pKOQ2drq3KWMbHeiNSAaxI2TGRirYCiZETnkX\n\
WCt0NxvrGtbvbsDHBaVju/5X9CiyJBFr+YFhZ8RK/UH8KxMqIAlvN5f3H30rPqwB\n\
QlI+scIXp5ZrFt97zaYw4czpWod4iZVm4O8fNJJAFq9qR2yxVyKaP7DZr3wZEt1+\n\
WJrOmkWPYkNC/YC1qnY35ubDAS7vZPvPtmw4oeJKSsTFwR5ddKMiLvPzRW3KgpT1\n\
B4zHBTO1xOKTYvEQqJqspz1ELUeSPemEYmZEZdakVLDKyzPZ5+a0WR4q3pDtmrZG\n\
KwIDAQAB\n\
-----END PUBLIC KEY-----"
