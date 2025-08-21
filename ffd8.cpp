// Embarcadero C++ 7.30 for Win32 Copyright (c) 2012-2017 Embarcadero Technologies, Inc.

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h> // Para strcmp en Linux

// --- Includes específicos del sistema operativo ---
#ifdef _WIN32
    #include <io.h>
    #include <windows.h>
#else
    #include <dirent.h> // Para manejo de directorios en Linux
    #include <unistd.h>
#endif

// --- Prototipos de funciones ---
int b_itoa(int value, char *sp, int radix=10);
int b_atoi(const char *num);
int image(int fd);
int guardar(int fd, int aomitir, char *nom);
void procesarArchivo(const char* rutaArchivo, int index);
void procesarDirectorio(const char* rutaDirectorio, int* pIndex);

int main(int argc, char* argv[] )
{
  int fileIndex = 1;

  if (argc < 2)
  {
      fprintf(stderr, "Uso: %s [archivo1] [directorio1] ...\n", argv[0]);
      return 1;
  }

  for(int i=1; i< argc; i++)
  {
    #ifdef _WIN32
        DWORD attribs = GetFileAttributesA(argv[i]);
        if (attribs == INVALID_FILE_ATTRIBUTES)
        {
            fprintf(stderr, "Advertencia: No se pudo acceder a '%s'\n", argv[i]);
            continue;
        }
        if (attribs & FILE_ATTRIBUTE_DIRECTORY)
        {
            procesarDirectorio(argv[i], &fileIndex);
        }
        else
        {
            procesarArchivo(argv[i], fileIndex++);
        }
    #else // Implementación para Linux/POSIX
        struct stat path_stat;
        if (stat(argv[i], &path_stat) != 0)
        {
            fprintf(stderr, "Advertencia: No se pudo acceder a '%s'\n", argv[i]);
            continue;
        }
        if (S_ISDIR(path_stat.st_mode))
        {
            procesarDirectorio(argv[i], &fileIndex);
        }
        else if (S_ISREG(path_stat.st_mode))
        {
            procesarArchivo(argv[i], fileIndex++);
        }
    #endif
  }
  return 0;
}

void procesarArchivo(const char* rutaArchivo, int index)
{
    // Esta función es multiplataforma tal como está.
    int fd = open(rutaArchivo, O_RDONLY);
    if (fd > 0)
    {
        int prInicio = image(fd);
        prInicio -= 4;
        if (prInicio >= 0)
        {
            lseek(fd, 0, SEEK_SET);
            char nombre[12];
            b_itoa(index, nombre);
            guardar(fd, prInicio, nombre);
            printf("%s %s\n", nombre, rutaArchivo);
        }
        close(fd);
    }
}

#ifdef _WIN32
void procesarDirectorio(const char* rutaDirectorio, int* pIndex)
{
    char rutaBusqueda[MAX_PATH];
    snprintf(rutaBusqueda, MAX_PATH, "%s\\*.*", rutaDirectorio);

    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(rutaBusqueda, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            char rutaCompleta[MAX_PATH];
            snprintf(rutaCompleta, MAX_PATH, "%s\\%s", rutaDirectorio, findFileData.cFileName);
            procesarArchivo(rutaCompleta, (*pIndex)++);
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
}
#else // Implementación de procesarDirectorio para Linux/POSIX
void procesarDirectorio(const char* rutaDirectorio, int* pIndex)
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(rutaDirectorio)))
    {
        fprintf(stderr, "Advertencia: No se pudo abrir el directorio '%s'\n", rutaDirectorio);
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char rutaCompleta[1024]; // PATH_MAX sería más robusto
        snprintf(rutaCompleta, sizeof(rutaCompleta), "%s/%s", rutaDirectorio, entry->d_name);

        struct stat path_stat;
        if (stat(rutaCompleta, &path_stat) != 0) {
            continue;
        }

        if (S_ISREG(path_stat.st_mode))
        {
            procesarArchivo(rutaCompleta, (*pIndex)++);
        }
    }
    closedir(dir);
}
#endif


int image(int fd)
{
  char c;
  int cc;
  int nread=0;
  int edo=0;
  char bu[1024];
  int encontrado=0;
  int ixread=0;
  int nocharRx=0;

  while ( (nread=read(fd,&bu,1023)) >0 )
  {
    ixread=0;

    while(ixread<nread)
    {
      c= bu[ixread];
      nocharRx++;
      cc= c & 0x000000FF;

      switch(edo)
      {
        case 0: 
        case 4:
          edo=( 0xff==cc 
              ? 1 : 0 );
          break;
        case 1:
          edo=( 0xd8==cc 
              ? 2 : 0 );
          break;
        case 2:
          edo=( 0xff==cc 
              ? 3 : 0 );
          break;
        case 3:
          edo=( 0xe0==cc or 0xe1==cc
              ? 4 : 0 );
          break;
        default:
          edo=0;
      }

      if( 4==edo)
        encontrado++;

      if(encontrado>0) break;

      ixread++;

    } /*ciclo de lectura x cantidad*/
    if(encontrado>0) break;

  } /*ciclo de lectura del archiv*/
  
  if( encontrado==0 ) nocharRx=0;

  return nocharRx;
}

int b_itoa(int value, char *sp, int radix)
{
  char tmp[16];// be careful with the length of the buffer
  char *tp = tmp;
  char *xp= sp;
  int i;
  unsigned v;
  int sign;
  int numTam=0;

  sign = (radix == 10 && value < 0);
  if (sign)
    v = -value;
  else
    v = (unsigned)value;

  while (v || tp == tmp)
  {
    numTam++;
    i = v % radix;
    v /= radix; // v/=radix uses less CPU clocks than v=v/radix does
    if (i < 10)
      *tp++ = i+'0';
    else
      *tp++ = i + 'a' - 10;
  }

  if (sign)
    *sp++ = '-';
  while (tp > tmp)
    *sp++ = *--tp;
  xp[numTam]=0;
  return numTam;
}


int b_atoi(const char *num)
{
  unsigned long value = 0;
  while (*num)
    value = value * 10 + *num++  - '0';
  return value;
}

int guardar(int fd, int aomitir, char *nom)
{
  char bu[1024];
  int leidos=0;
  int fdrw=0;
  if(aomitir>1023)
    aomitir=1023;
  leidos=read(fd,&bu, aomitir);

  if( leidos==aomitir)
  {
    leidos=0;
    #ifdef _WIN32
        fdrw = _sopen(nom, _O_WRONLY | _O_CREAT | _O_TRUNC,  _S_IREAD | _S_IWRITE);
    #else
        fdrw = open(nom, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    #endif

    if(fdrw>0 )
    {
      while ( (leidos=read(fd,&bu,1023)) >0 )
      {
        write(fdrw, &bu, leidos);
      }
    }
    close(fdrw);
  }
 return 0;  
}
