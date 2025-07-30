unsigned char* ELFHlp_LoadAndAllocImage(unsigned char* filedata, unsigned int filesize);
void ELFHlp_FreeImage(unsigned char* image);
void* ELFHlp_GetProcAddress(const unsigned char* image, const char *procname);

