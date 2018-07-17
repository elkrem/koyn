#ifndef Utils_h
#define Utils_h





inline void hex2bin(uint8_t * cont,const char * data, uint32_t len)
{
  if (len%2==0)
  {
    uint32_t count;
    uint8_t val[len / 2];
    for (count = 0; count < sizeof(val) / sizeof(val[0]); count++) {
      char buf[5] = {'0', 'x', data[0], data[1], '\0'};
      val[count] = strtol(buf, NULL, 0);
      data += 2;
    }
    memcpy(cont,val,len/2);
  }
}


inline void bin2hex(char * cont,uint8_t * data, uint32_t len)
{
  int pos =0;
  char val[(len*2)+1];
  for (unsigned int i=0; i<len; i++) {
    val[pos]="0123456789abcdef"[data[i]>>4];
    val[pos+1]="0123456789abcdef"[data[i]&0xf];
    pos+=2;
  }
  memcpy(cont,val,len*2);
}


inline void reverseBin(uint8_t * cont, uint32_t len)
{
  for (unsigned int i = 0, j = len - 1; i < len / 2; i++, j--)
  {
    int temp = cont[i];
    cont[i] = cont[j];
    cont[j] = temp;
  }
}

inline long long my_atoll(char *instr)
{
  long long retval;

  retval = 0;
  for (; *instr; instr++) {
    retval = 10*retval + (*instr - '0');
  }
  return retval;
}
inline String uint64ToString(uint64_t input) {
  String result = "";
  uint8_t base = 10;

  do {
    char c = input % base;
    input /= base;

    if (c < 10)
      c +='0';
    else
      c += 'A' - 10;
    result = c + result;
  } while (input);
  return result;
}

inline bool convertFileToHexString(File *file1, File *file2)
{
  if(file1->isOpen()&&file2->isOpen())
  {
    while(file1->available())
    {
      uint32_t remainder = file1->size()-file1->curPosition();
      if(remainder>=10)
      {
        uint8_t binData[10];
        char hexData[20];
        for(int i=0;i<10;i++){binData[i]=file1->read();}
        bin2hex(hexData,binData,10);
        file2->write(hexData,20);
      }else
      {
        uint8_t binData[remainder];
        char hexData[remainder*2];
        for(unsigned int i=0;i<remainder;i++){binData[i]=file1->read();}
        bin2hex(hexData,binData,remainder);
        file2->write(hexData,remainder*2);
      }
    }
    file2->close();
    return true;
  }else
  {
    return false;
  }
}
#endif
