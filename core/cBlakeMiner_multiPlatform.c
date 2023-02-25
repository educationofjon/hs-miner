#include "amd.h"
#include "macamd.h"
#include "nvidia.h"

#ifdef WIN32
  #define PROGRAM_FILE "cBLAKE_AMD.cl"
#endif
#ifndef WIN32
  #define PROGRAM_FILE "cBLAKE.cl"
#endif
#define KERNEL_FUNC "cBlake"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#include <conio.h>
#endif

#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef MAC
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#define MAX_PATH 256

typedef struct {
   int gpuID;
   int platformID;
   int hasInit;
   int hasNewWork;
   int awaitingWork;
   int global_size_multiplier;
   char* headerString;
   char* nonceString;
   char* targetString;
   
   #ifdef WIN32
   ULONGLONG* lastWorkReadTime;
   #endif

   #ifndef WIN32
   time_t* lastWorkReadTime;
   #endif

   cl_int *hasNonce; 
   cl_ulong *nonceOutput;
   cl_ulong *output;
   
   cl_ulong* lastNonce;
   cl_device_id* device;
   cl_command_queue queue;
   cl_event *readQueue[3];
   
   cl_mem input_buffer;
   cl_mem cBlake_output_buffer;
   cl_mem input_len_buffer;
   cl_mem outputLen_buffer;
   cl_mem key_buffer;
   cl_mem keyLen_buffer;
   cl_mem target_buffer;
   cl_mem nonce_out_buffer;
   cl_mem has_nonce_buffer;
   cl_mem nonce_as_ulong_buffer;

   cl_ulong* cBlake_output_buffer_map;
   cl_ulong* nonce_out_buffer_map;
   cl_int* has_nonce_buffer_map;
} gpuMinerContext;

/* Find a GPU or CPU associated with the first available platform */
cl_device_id create_device(int targetGPUID,int targetPlatformID) {

   cl_device_id dev;
   int err;
   int platformID = 1;
   if(targetPlatformID != -1){
      platformID = targetPlatformID;
   }
   char* value;
   size_t valueSize;
   cl_uint platformCount;
   cl_platform_id* platforms;
   cl_uint deviceCount;
   cl_device_id* devices;

   /* Identify a platform */
   err = clGetPlatformIDs(0, NULL, &platformCount);
   platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * platformCount);
   err = clGetPlatformIDs(platformCount, platforms, NULL);
   if(err < 0) {
      perror("Couldn't identify a platform");
      exit(1);
   } 

   clGetDeviceIDs(platforms[platformID], CL_DEVICE_TYPE_GPU, 0, NULL, &deviceCount);
   devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
   clGetDeviceIDs(platforms[platformID], CL_DEVICE_TYPE_GPU, deviceCount, devices, NULL);

   /* Access a device */
   err = clGetDeviceIDs(platforms[platformID], CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
   if(err == CL_DEVICE_NOT_FOUND) {
      printf("\nNOO, EFFING GPU WASNT FOUND!? %s\n","");
      //err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
   }
   if(err < 0) {
      perror("Couldn't access any devices");
      exit(1);   
   }

   for (int j = 0; j < deviceCount; j++) {

      // print device name
      clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 0, NULL, &valueSize);
      value = (char*) malloc(valueSize);
      clGetDeviceInfo(devices[j], CL_DEVICE_NAME, valueSize, value, NULL);
      //printf("%d. Device: %s\n", j+1, value);
      if(targetGPUID == -1 || targetGPUID == j){
         fprintf(stdout, "{\"event\":\"registerDevice\",\"id\":\"%i\",\"name\":\"%s\",\"platform\":\"%i\"}\r\n",j,value,platformID);
         fflush(stdout);
      }
      
      free(value);
   }
   if(targetGPUID == -1){
     
      return devices[0];
   }
   else{
      return devices[targetGPUID];  
   }
}

/* Create program from a file and compile it */
cl_program build_program(cl_context ctx, cl_device_id dev[], const char* program_buffer /*filename*/,unsigned int* program_len) {
   cl_program program;
   FILE *program_handle;
   char *program_log;
   size_t program_size, log_size;
   int err;
  /*
   cl_program program;
   FILE *program_handle;
   char *program_buffer, *program_log;
   size_t program_size, log_size;
   int err;
   

   program_handle = fopen(filename, "r");
   if(program_handle == NULL) {
      perror("Couldn't find the program file");
      exit(1);
   }
   
   fseek(program_handle, 0, SEEK_END);
   program_size = ftell(program_handle);
   rewind(program_handle);
   program_buffer = (char*)malloc(program_size + 1);
   program_buffer[program_size] = '\0';
   fread(program_buffer, sizeof(char), program_size, program_handle);
   fclose(program_handle);
*/

   /* Create program from file */
   program_size = program_len/*strlen(program_buffer)*/;
   //printf("sizeof k %lu",program_size);

   program = clCreateProgramWithSource(ctx, 1, 
      (const char**)&program_buffer, &program_size, &err);
   if(err < 0) {
      perror("Couldn't create the program");
      exit(1);
   }

   //free(program_buffer);

   /* Build program */
   err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
   
   if(err < 0) {

      /* Find size of log and print to std output */
      clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 
            0, NULL, &log_size);
      program_log = (char*) malloc(log_size + 1);
      program_log[log_size] = '\0';
      
      clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 
            log_size + 1, program_log, NULL);
      printf("%s\n", program_log);
      free(program_log);
      
      exit(1);
   }

   return program;
}
char *mfgets(char * restrict s, int n, FILE * restrict stream){
    int ch, i=0;
    if(n<1)return NULL;
    if(n==1){
        *s = '\0';
        return s;
    }
    while(EOF!=(ch=fgetc(stream))){
        s[i++] = ch;
        if(i == n - 1) break;
        if(ch == '\n'){
            char next = fgetc(stream);
            if(next == '\n')
                break;
            else
                ungetc(next, stream);
        }
    }
    s[i] = '\0';
    return i == 0 ? NULL : s;
}

unsigned long convertWindowsTimeToUnixTime(long long int input){
    long long int temp;
    temp = input / 10000000; //convert from 100ns intervals to seconds;
    temp = temp - 11644473600LL;  //subtract number of seconds between epochs
    return (unsigned long) temp;
}

int main( int argc, char *argv[]) {
#ifdef WIN32
   ULONGLONG previousResult = 0x0;
#endif
#ifndef WIN32
   unsigned long previousResult = 0x0;
#endif

   int platformID = 0;
   //argc == 4 is a test vector on the cli
   gpuMinerContext miningContexts[24];
   int kernelsNeedInit = 0;
   char* gpu_mfg = 'nvidia';
   int miningMode = 0; //0 = solo, 1 = pool
   //if(argc < 4){
      //ok then, starting at argv[1]::
      //argv[1] : comma sep gpus
      if(argv[1] == NULL){
         printf("\n PROVIDE AT LEAST A COMMA SEPARATED LIST OF GPU");
         exit(1);
      }
      char *ptr;
      //printf("\n argv1 %s",argv[1]);
      if(argv[2] != NULL){
         platformID = (int) strtol(argv[2],&ptr,10);
         //printf("\n platform is overridden %i",platformID);
      }
      if(argv[3] != NULL){
        gpu_mfg = argv[3];
        
      }
      if(argv[4] != NULL){
        miningMode = (int) strtol(argv[2],&ptr,10);
      }
      
      char* str = argv[1];
      int i=0;
      char *p = strtok(str,",");
      int *gpus[24];
      
      while(p != NULL){
         gpus[i] = (int) strtol(p,&ptr,10);
         p = strtok(NULL,",");
         i++;
      }
      int *filtGPUs[i];
      //printf("\n sizeof gpus %i %i",i,sizeof(filtGPUs));
      for(int j=0;j<i;j++){
         filtGPUs[j] = (int) gpus[j];
      }
      int addedDevices = 0;
      for(int j=0;j<i;j++){
         //printf("\n gpus for days %i %i",j,filtGPUs[j]);
         //int instance = startGPUKernel(j,platformID);
         gpuMinerContext minerContext;
         minerContext.platformID = platformID;
         minerContext.gpuID = filtGPUs[j];
         minerContext.awaitingWork = 1;
         minerContext.hasNewWork = 0;
         
         miningContexts[(int) filtGPUs[j]] = minerContext;
         if(filtGPUs[j] != -1){
          cl_device_id dev = create_device(filtGPUs[j],platformID);
          miningContexts[(int) filtGPUs[j]].device = dev;
          addedDevices = 1;
         }
         //int instance = testGPULoop(minerContext);
         fprintf(stdout, "{\"event\":\"getDeviceWork\",\"id\":\"%i\",\"platform\":\"%i\"}\r\n",(int) filtGPUs[j],platformID);
         fflush(stdout);
         
         if(minerContext.gpuID == -1){
            printf("{\"event\":\"getAllDevices\",\"note\":\"might take a second\",\"action\":\"log\"}\r\n");
            cl_device_id device;
            device = create_device(minerContext.gpuID,platformID);
            //this will log all the devices
         } 

      }
      int gpuLength = i;  
      
        //printf("\n getc is here %s",fgetc(stdin));
      startGPUKernel(miningContexts,filtGPUs,gpuLength,gpu_mfg,miningMode);
      //}

      
      //printf("\n test gpu context lookup %i",miningContexts[1].gpuID);
   //}
  return 0;
}
char *get_minerWork(int gpuID, int platformID)
{
    char homedir[MAX_PATH];

#ifdef WIN32
    snprintf(homedir, MAX_PATH, "%s%s%s%i%s%i%s", getenv("HOMEDRIVE"), getenv("HOMEPATH"),"/.HandyMiner/",platformID,"_",gpuID,".work");
#else
    snprintf(homedir, MAX_PATH, "%s%s%i%s%i%s", getenv("HOME"),"/.HandyMiner/",platformID,"_",gpuID,".work");
    //printf("\n env home??? %s",getenv("HOME"));
#endif
    return strdup(homedir);
}
int setGPUWork(gpuMinerContext gpuMinerContexts[],int gpuID){
  char * buffer = 0;
  long length;
  char loc[MAX_PATH];
  char *workLoc = loc;
  
  gpuMinerContext mCtx = gpuMinerContexts[gpuID];
  #ifdef WIN32
    snprintf(workLoc, MAX_PATH, "%s%s%s%i%s%i%s", getenv("HOMEDRIVE"), getenv("HOMEPATH"),"/.HandyMiner/",mCtx.platformID,"_",mCtx.gpuID,".work");
  #else
    snprintf(workLoc, MAX_PATH, "%s%s%i%s%i%s", getenv("HOME"),"/.HandyMiner/",mCtx.platformID,"_",mCtx.gpuID,".work");
    //printf("\n env home??? %s",getenv("HOME"));
  #endif

  //printf("{\"gpuid_set_work\":\"all\",\"action\":\"log\"}\r\n");
  FILE * f = fopen (workLoc, "rb");
  

  if (f)
  {
    fseek (f, 0, SEEK_END);
    length = ftell (f);
    fseek (f, 0, SEEK_SET);
    buffer = malloc (length);
    if (buffer)
    {
      fread (buffer, 1, length, f);
    }
    fclose (f);
  }
  if (buffer)
  {
    //printf("\n read buffer \n %s",buffer);
    // start to process your data / extract strings here...
    int iterations=0;
    
    char *mptr;
    int messageGPUID = 0;
    int global_size_multiplier = 0;
    char* blockHeaderString;
    char* nonceString;
    char* targetString;
    char *lnp = strtok(buffer,"\n");
    //printf("\n lnp %s",lnp);
    while(lnp != NULL){
      iterations = 0;
      //
      //printf("\n lnp is now %s",lnp);
      char *inD;// = (char*) lnp;
      inD = lnp;
      //memcpy(inD,lnp,strlen(lnp)+1);
      char *mp = strtok(inD,"|");
      //while((mp = strtok_r(inD,"|",&inD))/*mp != NULL*/){
      while(/*(mp = strtok(inD,"|"))*/ mp != NULL){
        //printf("\n lnp??? %s",lnp);
        if(iterations == 0){
          //parse int, its field 0 or 1
          messageGPUID = (int) strtol(mp,&mptr,10);
        }
        else if(iterations == 1){
          global_size_multiplier = (int) strtol(mp,&mptr,10);
          
        }
        
        else if(iterations == 2){
          //header strng
          blockHeaderString = (char*) mp;
        }
        else if(iterations == 3){
          //nonce string
          nonceString = (char*) mp;
        }
        else if(iterations == 4){
          //target string
          targetString = (char*) mp;
        
        }
        
        mp = strtok(NULL,"|");
        //printf("\n strtok is null %s",lnp);
        iterations++;
      }
      
      

      //printf("\n lnp mid %s",lnp);
      //event type will always be zero
      //well re-purpose this field to be the global_size_multiplier that can be used
      //like intensity on the miner side..
      //if(eventType == 0){
        //new job is here, set a gpu status
        printf("{\"gpuid_set_work\":%i,\"action\":\"log\"}\r\n",messageGPUID);
        gpuMinerContext gpu = gpuMinerContexts[messageGPUID];
        
        gpuMinerContexts[messageGPUID].headerString = blockHeaderString;
        gpuMinerContexts[messageGPUID].targetString = targetString;
        gpuMinerContexts[messageGPUID].nonceString = nonceString;
        gpuMinerContexts[messageGPUID].hasNewWork = (int) 1; //will pick up in the kernel loop per that gpu context
        gpuMinerContexts[messageGPUID].global_size_multiplier = (int) global_size_multiplier;  //eventType gets repurposed here
      //}
      lnp = strtok(NULL,"\n");
      
    }//end
    return 1; //all good
  }
  else{
    //no buffer
    return 0;
  }

  //return 0;
}
#ifndef WIN32
int checkWorkInUnix(gpuMinerContext gpuMinerContexts[], time_t **previousResult, int gpuCount, int *gpuIDs[]){
  struct stat statbuf;
  //char path = "miner.work";
  char resp;
  //char *workLoc = (char*) get_minerWork();
  //char *workLoc = strcat(getenv("HOME"),"/.HandyMiner/miner.work");//get_minerWork();
  for(int i=0;i<gpuCount;i++){
    
    gpuMinerContext mCtx = gpuMinerContexts[(int) gpuIDs[i]];
    /*char *workLoc = (char*) get_minerWork(mCtx.gpuID,mCtx.platformID);*/
    char loc[MAX_PATH];
    char *workLoc = loc;
    #ifdef WIN32
      snprintf(workLoc, MAX_PATH, "%s%s%s%i%s%i%s", getenv("HOMEDRIVE"), getenv("HOMEPATH"),"/.HandyMiner/",mCtx.platformID,"_",mCtx.gpuID,".work");
    #else
      snprintf(workLoc, MAX_PATH, "%s%s%i%s%i%s", getenv("HOME"),"/.HandyMiner/",mCtx.platformID,"_",mCtx.gpuID,".work");
      //printf("\n env home??? %s",getenv("HOME"));
    #endif

    if (stat(workLoc, &statbuf) == -1) {
        perror(workLoc);
        exit(1);
    }
    time_t mtime = statbuf.st_mtime;
    unsigned long mSeconds = (unsigned long) mtime;
    unsigned long prevSeconds = (unsigned long) mCtx.lastWorkReadTime;
    
    
    if(prevSeconds != mSeconds){
      
      int didRead = setGPUWork(gpuMinerContexts,gpuIDs[i]);
      if(didRead == 1){
        gpuMinerContexts[(int) gpuIDs[i]].lastWorkReadTime = mtime;
      }

    }
  }
  
  
  //free(workLoc);
  return 0;
}
#endif

#ifdef WIN32

int checkWorkIn(gpuMinerContext gpuMinerContexts[], ULONGLONG **previousResult, int gpuCount, int gpuIDs[]){
  ULONGLONG qwResult;
  FILETIME CreateTime;
  FILETIME AccessTime;
  FILETIME WriteTime;
  SYSTEMTIME stUTC, stLocal, stUTC1, stLocal1, stUTC2, stLocal2;
  for(int i=0;i<gpuCount;i++){
    gpuMinerContext mCtx = gpuMinerContexts[(int) gpuIDs[i]];
    LPCWSTR fileName = (LWCWSTR) (char* get_minerWork(mCtx.gpuID,mCtx.platformID)); //"./miner.work";
    //printf("\n trying open file %S",fileName);
    
    HANDLE hFile = CreateFile(fileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    
    BOOL gft = GetFileTime(hFile,&CreateTime,&AccessTime,&WriteTime);
    CloseHandle(hFile);
    if(gft){
      qwResult = convertWindowsTimeToUnixTime( (((ULONGLONG) WriteTime.dwHighDateTime) << 32) + WriteTime.dwLowDateTime );
      if(qwResult != (ULONGLONG) mCtx.lastWorkReadTime){
        printf("\n set gpu work on checkWorkIn Windows");
        //*previousResult = (ULONGLONG*) qwResult;
        //setGPUWork(gpuMinerContexts,gpuCount,gpuIDs);
        int didRead = setGPUWork(gpuMinerContexts,gpuIDs[i]);
        if(didRead == 1){
          gpuMinerContexts[(int) gpuIDs[i]].lastWorkReadTime = (ULONGLONG*) qwResult;
        }
      }
    }  
  }
  
  
  fflush(stdout);
  return 0;
}
#endif 
//end windows method


int startGPUKernel(gpuMinerContext gpuMinerContexts[],int *gpuIDs[],int gpuCount, char* gpuManufacturer, int miningMode){
   cl_ulong nonce = 0;
   
#ifdef WIN32
   ULONGLONG *previousResult = 0x0;
#endif
#ifndef WIN32
   time_t *previousResult;
#endif
   cl_ulong output[8];
   /* OpenCL structures */
   //cl_device_id device;
   cl_device_id devices[(int) gpuCount];
   for(int i=0;i<gpuCount;i++){
    //devices[i] = create_device((int)gpuIDs[i],0);//
    devices[i] = (cl_device_id) gpuMinerContexts[(int)gpuIDs[i]].device;
    //printf("\n device long id %016llu",devices[i]);
   }
#ifdef WIN32
   checkWorkIn(gpuMinerContexts,&previousResult,gpuCount,gpuIDs);
#endif

#ifndef WIN32
   checkWorkInUnix(gpuMinerContexts,&previousResult,gpuCount,gpuIDs);
#endif

   //printf("\n post work check %016llu",previousResult);
   cl_context context;
   cl_program program;
   cl_kernel kernel;
   //cl_command_queue queue;
   cl_int i, err;

   size_t local_size, global_size;
   int clMinerThreads = 1;

   /* Data and buffers */
   //cl_mem input_buffer, cBlake_output_buffer, input_len_buffer, outputLen_buffer, key_buffer, keyLen_buffer, target_buffer;
   //cl_mem nonce_out_buffer, has_nonce_buffer, nonce_as_ulong_buffer;
   /* Create device and context */
   //device = create_device(minerContext.gpuID,minerContext.platformID);
   //TODO:::: TRY CONTEXT/PROGRAM/KERNEL/QUEUE PER DEVICE?
   printf("{\"status\":\"building kernel\",\"action\":\"log\"}\r\n");
   fflush(stdout);

   context = clCreateContext(NULL, gpuCount, &devices, NULL, NULL, &err);
   if(err < 0) {
      perror("Couldn't create a context");
      exit(1);   
   }
   //printf("\n created context");
   //return;
   /* Build program */
   //char* prFile = PROGRAM_FILE;
   const char* progBuffer;
   unsigned int progLen;
   if(strcmp(gpuManufacturer , "amd") == 0 ){
    //prFile = "cBLAKE_AMD.cl";
      progBuffer = cBLAKE_AMD_cl;
      progLen = cBLAKE_AMD_cl_len;
    #ifdef MAC
      //prFile = "cBLAKE_MAC_AMD.cl";
      progBuffer = cBLAKE_MAC_AMD_cl;
      progLen = cBLAKE_MAC_AMD_cl_len;
    #endif
   }
   else if(strcmp(gpuManufacturer , "nvidia") == 0 ){
    //prFile = "cBLAKE_NVIDIA.cl";
    progBuffer = cBLAKE_NVIDIA_cl;
    progLen = cBLAKE_NVIDIA_cl_len;
   }
   //printf("{\"action\":\"log\", \"clExec\":\"%s\"}\r\n",program_buffer);
   program = build_program(context, devices, progBuffer /*prFile*//*PROGRAM_FILE*/,progLen);
   //printf("\n build program");
   /* Create data buffer */
   #ifdef MAC
      global_size = 4194304;//16384;
      local_size = 64;
   #else
      global_size = (size_t) (4194304); //vega64, I think 1070 is half this
      local_size = 64;
   #endif
   printf("{\"action\":\"log\", \"globalSize\":\"%i\"}\r\n",global_size);
   /*global_size = 1;
   local_size = 1;*/
   

   /* Create a command queue */
  for(int wtf=0;wtf<gpuCount;wtf++){
    //cl_device_id dev = gpuMinerContexts[(int) gpuIDs[wtf]].device;
    cl_device_id dev = devices[wtf];//create_device((int)gpuIDs[wtf],1);
    //printf("\n and cl device id %i isset %i %d actual %i %d",gpuIDs[wtf],dev,dev);
    //cl_command_queue queue = clCreateCommandQueue(context,dev,0,NULL);
    gpuMinerContexts[(int) gpuIDs[wtf]].queue = clCreateCommandQueue(context,dev,0,NULL);
  
  }
  //printf("\n created queues");
  //return;
   //queue = clCreateCommandQueue(context, device, 0, &err);
   
   if(err < 0) {
      perror("Couldn't create a command queue");
      exit(1);   
   };

   /* Create a kernel */
   kernel = clCreateKernel(program, KERNEL_FUNC, &err);
   if(err < 0) {
      perror("Couldn't create a kernel");
      exit(1);
   };
   
   cl_int *inputLength = (cl_int*)malloc(sizeof(cl_int));
   cl_char *testInput;
   
   cl_int *outputLen = (cl_int*)malloc(sizeof(cl_int));
   outputLen[0] = (int) 512;
   
   cl_int *keyLen = (cl_int*)malloc(sizeof(cl_int));
   //cl_int *hasNonce = (cl_int*)malloc(sizeof(cl_int)); //1 || 0 we found a nonce that satisfies target
#ifndef WIN32
    for(int wtf=0;wtf<gpuCount;wtf++){
     gpuMinerContexts[(int) gpuIDs[wtf]].hasNonce = (cl_int*)malloc(sizeof(cl_int)); //1 || 0 we found a nonce that satisfies target
     
     //cl_ulong* nonceOutput = (uint8_t*)malloc(8*sizeof(cl_ulong)); //only need to return the best one
     gpuMinerContexts[(int) gpuIDs[wtf]].nonceOutput = (uint8_t*)malloc(8*sizeof(cl_ulong)); //only need to return the best one
     gpuMinerContexts[(int) gpuIDs[wtf]].output = (uint8_t*)malloc(8*sizeof(cl_ulong)); //only need to return the best one
   }
#endif
#ifdef WIN32
   for(int wtf=0;wtf<gpuCount;wtf++){
     gpuMinerContexts[(int) gpuIDs[wtf]].hasNonce = (cl_int*)_aligned_malloc(sizeof(cl_int),4096); //1 || 0 we found a nonce that satisfies target
     
     //cl_ulong* nonceOutput = (uint8_t*)malloc(8*sizeof(cl_ulong)); //only need to return the best one
     gpuMinerContexts[(int) gpuIDs[wtf]].nonceOutput = (uint8_t*)_aligned_malloc(8*sizeof(cl_ulong),4096); //only need to return the best one
     gpuMinerContexts[(int) gpuIDs[wtf]].output = (uint8_t*)_aligned_malloc(8*sizeof(cl_ulong),4096); //only need to return the best one
   }
#endif

   cl_ulong lastTime = (cl_ulong)time(NULL);
   cl_ulong timeDiff;
   cl_ulong lastNonce = 0;
   cl_ulong nonceDiff;
   

   char *outputData = (char*) malloc(64*sizeof(char));
   char *outputNonceData = (char*) malloc(64*sizeof(char));
   
   //inputLength[0] = strlen(minerContext.headerString) / 2;
   //int inputLen = (int) strlen(minerContext.headerString) / 2;
   int inputLen;
   //testInput = (cl_char*)malloc(416);

   uint8_t* inputBytes;
   //inputBytes = (uint8_t*)malloc(strlen(argv[1])/2);
   
   char tmp[3];
   tmp[2] = '\0';
   
   char tmp2[9];
   tmp2[8] = '\0';

   int rB,kkLen,krB, nonceUlPos;

   uint8_t* keyBytes;
   //keyBytes = (uint8_t*)malloc(strlen(argv[2])/2);

   

   //only set inputBytes on init, and todo: socket update
   
   
   


   

   /*cl_ulong lastTime = (cl_ulong)time(NULL);
   cl_ulong timeDiff;
   cl_ulong lastNonce = 0;
   cl_ulong nonceDiff;*/

   cl_ulong nonceOffset = 0x0;
   cl_ulong minerIterations = 0x0;
   cl_ulong iterationsLast = 0x0;

   //keyLen[0] = strlen(minerContext.nonceString) / 2;
   //kkLen = (int) strlen(minerContext.nonceString) / 2;

   //int len = strlen(minerContext.targetString) || 1;
   cl_ulong* targetBytes;
   cl_ulong* nonceAsUlong;

   
   //nonceAsUlong = (cl_ulong*)malloc(8);
   //argv2 = noncestring, 
   //argv3 = targetstring
   //argv1 = headerstring
   
   int tB = 0;
   /*key = (cl_char*)malloc(strlen(minerContext.nonceString));
   for(int i=0;i<strlen(minerContext.nonceString);i++){
      key[i] = minerContext.nonceString[i];
   }*/
printf("{\"status\":\"mining commenced\",\"action\":\"log\"}\r\n");
fflush(stdout);
while(1/*hasSolved == 0*/){ //loop fordays

#ifdef WIN32
  checkWorkIn(gpuMinerContexts,&previousResult,gpuCount,gpuIDs); //get work
  printf(""); //really not sure why this is needed but on mac if not we get segfault?
#endif
#ifndef WIN32
  checkWorkInUnix(gpuMinerContexts,&previousResult,gpuCount,gpuIDs);
  printf(""); //really not sure why this is needed but on mac if not we get segfault?
#endif

  //checkStdIn(gpuMinerContexts);
  //printf("\n checkwork for %i %s",gpuMinerContexts[4].gpuID,gpuMinerContexts[4].headerString);
  int shouldWeSleep = 0;
  cl_event *readQueue[gpuCount*3];
  size_t global_size_modified = global_size;
  for(int minerI=0;minerI<gpuCount;minerI++){

    gpuMinerContext minerContext = (gpuMinerContext) gpuMinerContexts[(int)gpuIDs[minerI]];
    int gsModifier = (int) minerContext.global_size_multiplier;

    if(gsModifier >= 1 && gsModifier <= 10){
      //ok lets tune global size
      global_size_modified = (size_t) ((int)global_size >> (10 - gsModifier));
    }
    if(gsModifier == 11){
      //printf('{\"message\":\"ENABLE BEAST MODE\",\"action\":\"log\"}\r\n');
      global_size_modified = (size_t)((int) global_size * 2);
    }

    if(gsModifier == 161803){
      global_size_modified = (size_t)((int) global_size * 4);
    }
    if(gsModifier == 323606){
      global_size_modified = (size_t)((int) global_size * 16);
    }
    if(gsModifier == 485409){
      global_size_modified = (size_t)((int) global_size * 32);
    }
    if(gsModifier == 647212){
      global_size_modified = (size_t)((int) global_size * 64);
    }
    /*printf("{\"action\":\"log\", \"globalSize\":\"%i\"}\r\n",global_size_modified);
    fflush(stdout);*/
      //printf("\n miner context %i awaits %i",minerContext.gpuID,minerContext.awaitingWork);
  
     if(minerContext.awaitingWork == 1 && minerContext.hasNewWork == 0){
      //sleep(0.25);
      shouldWeSleep += 1;
      continue;
     }
     if(minerContext.awaitingWork == 1 && minerContext.hasNewWork == 1){
      //new work is here after the last job, now we can go again
      //printf("\n work is here!");
      minerIterations = 0x0;
      nonceOffset = 0x0;
      //printf("\nnew work was picked up in mining loop gpu %i",minerContext.gpuID);
      gpuMinerContexts[(int)gpuIDs[minerI]].awaitingWork = 0;
     }
     //printf("\n will loop now");
     //hasNonce = (cl_int*)malloc(sizeof(cl_int));
     //output = (cl_ulong*)malloc(8*sizeof(cl_ulong));
#ifdef WIN32
     keyBytes = (uint8_t*)_aligned_malloc(strlen(minerContext.nonceString)/2,4096);
     inputBytes = (uint8_t*)_aligned_malloc(strlen(minerContext.headerString)/2,4096);
     targetBytes = (cl_ulong*)_aligned_malloc(8*sizeof(cl_ulong),4096);
     nonceAsUlong = (cl_ulong*)_aligned_malloc(8*sizeof(cl_ulong),4096);
#endif
#ifndef WIN32
     keyBytes = (uint8_t*)malloc(strlen(minerContext.nonceString)/2);
     inputBytes = (uint8_t*)malloc(strlen(minerContext.headerString)/2);
     targetBytes = (cl_ulong*)malloc(8*sizeof(cl_ulong));
     nonceAsUlong = (cl_ulong*)malloc(8*sizeof(cl_ulong));
#endif
     //hasNonce = (cl_int*)malloc(sizeof(cl_int));
     //reset garbage again
     keyLen[0] = strlen(minerContext.nonceString) / 2;
     kkLen = (int) strlen(minerContext.nonceString) / 2;
     inputLength[0] = strlen(minerContext.headerString) / 2;
     inputLen = (int) strlen(minerContext.headerString) / 2;
     outputLen[0] = (int) 512;
     
     if(minerContext.hasNewWork == 1){
      minerIterations = 0x0; //todo add minerIterations to gpuMinerContext struct
      gpuMinerContexts[(int)gpuIDs[minerI]].hasNewWork = 0;
     }

     nonce = (cl_ulong) (minerIterations * (int) global_size_modified * clMinerThreads );
     lastNonce = minerContext.lastNonce;
     cl_ulong timeNow = (cl_ulong)time(NULL);
     timeDiff = timeNow - lastTime;
     nonceDiff = nonce - lastNonce;
     
     nonceUlPos = 0;
     
     for(int i=0;i<strlen(minerContext.nonceString);i+=8){
        tmp2[0] = minerContext.nonceString[i+0];
        tmp2[1] = minerContext.nonceString[i+1];
        tmp2[2] = minerContext.nonceString[i+2];
        tmp2[3] = minerContext.nonceString[i+3];
        tmp2[4] = minerContext.nonceString[i+4];
        tmp2[5] = minerContext.nonceString[i+5];
        tmp2[6] = minerContext.nonceString[i+6];
        tmp2[7] = minerContext.nonceString[i+7];
        nonceAsUlong[nonceUlPos] = strtoul(tmp2,NULL,16);
        nonceUlPos++;
     }
     //printf("\nnonceasulong 0 %016llx 1 %016llx",nonceAsUlong[0],nonceAsUlong[1]);
     //nonceAsUlong[0] = ((nonceAsUlong[0] << 32 & 0xFFFFFFFF00000000) | nonceAsUlong[1] & 0x00000000FFFFFFFF);
     //nonceAsUlong[0] = nonceAsUlong[0] >> 32 & 0x00000000FFFFFFFF;

     nonceAsUlong[1] = 0x0000000000000000;


     rB = 0;
     for(int i=0;i<strlen(minerContext.headerString);i+=2){
        tmp[0] = minerContext.headerString[i+0];
        tmp[1] = minerContext.headerString[i+1];
        inputBytes[rB] = strtol(tmp,NULL,16); //
        //printf("\nuint8_t char %i %02lx %02lu",rB,inputBytes[rB],inputBytes[rB]);
        rB++;
     }

     krB = 0;
     for(int i=0;i<strlen(minerContext.nonceString);i+=2){
        tmp[0] = minerContext.nonceString[i+0];
        tmp[1] = minerContext.nonceString[i+1];
        keyBytes[krB] = strtol(tmp,NULL,16);
        //printf("\n nonce char isset %i %02x",i,keyBytes[krB]);
        krB++;
     }

     tB = 0;

     for(int i=0;i<strlen(minerContext.targetString);i+=8){
        tmp2[0] = minerContext.targetString[i+0];
        tmp2[1] = minerContext.targetString[i+1];
        tmp2[2] = minerContext.targetString[i+2];
        tmp2[3] = minerContext.targetString[i+3];
        tmp2[4] = minerContext.targetString[i+4];
        //argv2 = noncestring, 
        //argv3 = targetstring
        //argv1 = headerstring
        tmp2[5] = minerContext.targetString[i+5];
        tmp2[6] = minerContext.targetString[i+6];
        tmp2[7] = minerContext.targetString[i+7];
        targetBytes[tB] = strtoul(tmp2,NULL,16) ;
        tB++;
     }

     //targetBytes[0] = ((targetBytes[0] << 32 & 0xFFFFFFFF00000000) | targetBytes[1] & 0x00000000FFFFFFFF);
     //targetBytes[0] <<= 8;
     //printf("\n target bytes in C land0 %016llx %016llx %016llx",targetBytes[0],targetBytes[1],targetBytes[2]);
     targetBytes[0] = (cl_ulong*) ((targetBytes[0] << 32) & 0xFFFFFFFF00000000 | /*targetBytes[1] << 8 & 0x00000000FF000000 |*/ targetBytes[1] << 0 & 0x00000000FFFFFFFF);
     if(targetBytes[2] != 0x0000000000000000){
      //we have a big target, lets pass that in too
      targetBytes[1] = (cl_ulong*) (targetBytes[2] << 32 & 0xFFFFFFFF00000000);
     }
     else{
      targetBytes[1] = (cl_ulong*) 0x0000000000000000;
     }
     //targetBytes[0] = (cl_ulong*) ((targetBytes[0] << 40) & 0xFFFFFFFF00000000 | targetBytes[1] << 8 & 0x000000FF00000000 | targetBytes[1] << 8 & 0x00000000FFFFFFFF);
     //targetBytes[1] = 0x0000000000000000;
     //printf("\n target bytes in C land1 %016llx %016llx %016llx",targetBytes[0],targetBytes[1],targetBytes[2]);
     
     /*exit(0);*/
     /*end sigh*/
     
     nonceOffset = (cl_ulong) (minerIterations * (cl_ulong)( (cl_ulong)((int)global_size_modified-1) * clMinerThreads));
     
     nonceAsUlong[0] += nonceOffset;
     int nonceOverflowed = 0;
     if(nonceAsUlong[0] >= 0x00000000FFFFFFFF){
        nonceOverflowed = 1;
     }
     nonceAsUlong[0] = (nonceAsUlong[0] << 32 & 0xFFFFFFFF00000000);
     if(nonceOverflowed == 1){
      //nonce did overflow whoaaaaaaaaaa
      printf("{\"event\":\"nonceFull\",\"gpuID\":\"%i\", \"platformID\":\"%i\", \"nonce\":\"%016llx\"}\r\n",minerContext.gpuID,minerContext.platformID,nonceAsUlong[0]);
      fflush(stdout);
     }
     //minerIterations += 0x0000000000000001;
     /*if(nonceAsUlong[0] >= 0xFFFFFF0000000000){
      //request a new nonce time
      printf("{\"event\":\"nonceFull\",\"gpuID\":\"%i\", \"platformID\":\"%i\", \"nonce\":\"%016llx\"}\r\n",minerContext.gpuID,minerContext.platformID,nonceAsUlong[0]);
      fflush(stdout);
     }*/
     if((int)timeDiff>= 10){

        //lastTime = timeNow;
        lastNonce = nonce;
        gpuMinerContexts[(int)gpuIDs[minerI]].lastNonce = lastNonce;
        unsigned long hashPS = nonceDiff / timeDiff;
        nonce += nonceAsUlong[0];
        //printf("\ntimediff %i noncediff %f hashps %i",nonceDiff,(float)nonceDiff,hashPS);
        updateStatus(minerContext.headerString, nonceAsUlong[0],hashPS,minerContext.gpuID,minerContext.platformID);
        //printResultsJSON(testInput,outputData,inputLen,key);
     }
     //printf("\n nonceas ullong %016llx minerIterations: %016llu global_size_modified: %i clMinerThreads: %i \n",nonceAsUlong[0],minerIterations,global_size,clMinerThreads);
     
     gpuMinerContexts[(int) gpuIDs[minerI]].input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY |
           CL_MEM_COPY_HOST_PTR, inputLen * sizeof(unsigned char), inputBytes, &err);
     gpuMinerContexts[(int) gpuIDs[minerI]].cBlake_output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE |
           CL_MEM_COPY_HOST_PTR, 8 * sizeof(cl_ulong), /*NULL*/ gpuMinerContexts[(int) gpuIDs[minerI]].output, &err);
     gpuMinerContexts[(int) gpuIDs[minerI]].input_len_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY |
           CL_MEM_COPY_HOST_PTR, sizeof(cl_int), inputLength, &err);
     gpuMinerContexts[(int) gpuIDs[minerI]].key_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | 
           CL_MEM_COPY_HOST_PTR, kkLen * sizeof(unsigned char), keyBytes, &err);
     gpuMinerContexts[(int) gpuIDs[minerI]].keyLen_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | 
           CL_MEM_COPY_HOST_PTR, sizeof(cl_int), keyLen, &err);
     gpuMinerContexts[(int) gpuIDs[minerI]].outputLen_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | 
           CL_MEM_COPY_HOST_PTR, sizeof(cl_int), outputLen, &err);
     gpuMinerContexts[(int) gpuIDs[minerI]].target_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY |
           CL_MEM_COPY_HOST_PTR, 8 * sizeof(cl_ulong), targetBytes, &err);
     gpuMinerContexts[(int) gpuIDs[minerI]].has_nonce_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE |
           CL_MEM_COPY_HOST_PTR, sizeof(cl_int), /*NULL*/ gpuMinerContexts[(int) gpuIDs[minerI]].hasNonce, &err);
     gpuMinerContexts[(int) gpuIDs[minerI]].nonce_out_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | 
           CL_MEM_COPY_HOST_PTR, 8 * sizeof(cl_ulong), /*NULL*/gpuMinerContexts[(int) gpuIDs[minerI]].nonceOutput, &err);
     gpuMinerContexts[(int) gpuIDs[minerI]].nonce_as_ulong_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY |
           CL_MEM_COPY_HOST_PTR, 8 * sizeof(cl_ulong), nonceAsUlong, &err);
     
                                                                                                    //command_queue[0], mPattern_obj, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, MAX_PATTERN_SIZE * MAX_PATTERN_NUM * sizeof(cl_char), 0, NULL, NULL, &ret
     /*gpuMinerContexts[(int) gpuIDs[minerI]].cBlake_output_buffer_map = (cl_ulong *) clEnqueueMapBuffer(gpuMinerContexts[(int) gpuIDs[minerI]].queue, gpuMinerContexts[(int) gpuIDs[minerI]].cBlake_output_buffer,CL_FALSE,CL_MAP_READ, 0, 8 * sizeof(cl_ulong), 0,NULL,&gpuMinerContexts[(int) gpuIDs[minerI]].readQueue[0],&err);
     gpuMinerContexts[(int) gpuIDs[minerI]].has_nonce_buffer_map = (cl_int *) clEnqueueMapBuffer(gpuMinerContexts[(int) gpuIDs[minerI]].queue, gpuMinerContexts[(int) gpuIDs[minerI]].has_nonce_buffer,CL_FALSE,CL_MAP_READ, 0, sizeof(cl_int), 0,NULL,&gpuMinerContexts[(int) gpuIDs[minerI]].readQueue[1], &err);
     gpuMinerContexts[(int) gpuIDs[minerI]].nonce_out_buffer_map = (cl_ulong *) clEnqueueMapBuffer(gpuMinerContexts[(int) gpuIDs[minerI]].queue, gpuMinerContexts[(int) gpuIDs[minerI]].nonce_out_buffer,CL_FALSE,CL_MAP_READ, 0, 8 * sizeof(cl_ulong), 0,NULL,&gpuMinerContexts[(int) gpuIDs[minerI]].readQueue[2], &err);
     */

     if(err < 0) {
        perror("Couldn't create a buffer");
        exit(1);   
     };
     /* Create kernel arguments */
     err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &gpuMinerContexts[(int) gpuIDs[minerI]].input_buffer);
     err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &gpuMinerContexts[(int) gpuIDs[minerI]].cBlake_output_buffer);
     err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &gpuMinerContexts[(int) gpuIDs[minerI]].input_len_buffer);
     err |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &gpuMinerContexts[(int) gpuIDs[minerI]].outputLen_buffer);
     err |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &gpuMinerContexts[(int) gpuIDs[minerI]].key_buffer);
     err |= clSetKernelArg(kernel, 5, sizeof(cl_mem), &gpuMinerContexts[(int) gpuIDs[minerI]].keyLen_buffer);
     err |= clSetKernelArg(kernel, 6, sizeof(cl_mem), &gpuMinerContexts[(int) gpuIDs[minerI]].target_buffer);
     err |= clSetKernelArg(kernel, 7, sizeof(cl_mem), &gpuMinerContexts[(int) gpuIDs[minerI]].has_nonce_buffer);
     err |= clSetKernelArg(kernel, 8, sizeof(cl_mem), &gpuMinerContexts[(int) gpuIDs[minerI]].nonce_out_buffer);
     err |= clSetKernelArg(kernel, 9, sizeof(cl_mem), &gpuMinerContexts[(int) gpuIDs[minerI]].nonce_as_ulong_buffer);
     

     /* Enqueue kernel */
     err = clEnqueueNDRangeKernel(gpuMinerContexts[(int) gpuIDs[minerI]].queue, kernel, 1, NULL, &global_size_modified, 
           &local_size, 0, NULL, NULL); 
     if(err < 0) {
        perror("Couldn't enqueue the kernel");
        exit(1);
     }

     /* Read the kernel's output */
     err = clEnqueueReadBuffer(gpuMinerContexts[(int) gpuIDs[minerI]].queue, gpuMinerContexts[(int) gpuIDs[minerI]].cBlake_output_buffer, CL_FALSE, 0, 
         sizeof(output), gpuMinerContexts[(int) gpuIDs[minerI]].output, 0, NULL, &gpuMinerContexts[(int) gpuIDs[minerI]].readQueue[0]);
     err |= clEnqueueReadBuffer(gpuMinerContexts[(int) gpuIDs[minerI]].queue, gpuMinerContexts[(int) gpuIDs[minerI]].nonce_out_buffer, CL_FALSE, 0, 
           sizeof(cl_ulong) * 8, gpuMinerContexts[(int) gpuIDs[minerI]].nonceOutput, 0, NULL, &gpuMinerContexts[(int) gpuIDs[minerI]].readQueue[1]);

     err |= clEnqueueReadBuffer(gpuMinerContexts[(int) gpuIDs[minerI]].queue, gpuMinerContexts[(int) gpuIDs[minerI]].has_nonce_buffer, CL_FALSE, 0, 
           sizeof(int), gpuMinerContexts[(int) gpuIDs[minerI]].hasNonce, 0, NULL, &gpuMinerContexts[(int) gpuIDs[minerI]].readQueue[2]);
     
     //clFinish(gpuMinerContexts[(int) gpuIDs[minerI]].queue);
     
     if(err < 0) {
        perror("Couldn't read the buffer");
        exit(1);
     }

     //clFinish(gpuMinerContexts[(int) gpuIDs[minerI]].queue);
#ifndef WIN32
    free(keyBytes);
    free(inputBytes);
    free(targetBytes);
    free(nonceAsUlong);
#endif
#ifdef WIN32
    _aligned_free(keyBytes);
    _aligned_free(inputBytes);
    _aligned_free(targetBytes);
    _aligned_free(nonceAsUlong);
#endif
  } //end for each gpu
  //clWaitForEvents(gpuCount*3,readQueue);

  for(int minerI=0;minerI<gpuCount;minerI++){
    //if(minerIterations % 100 == 0){
      //clFinish(gpuMinerContexts[(int) gpuIDs[minerI]].queue);
    /*gpuMinerContexts[(int) gpuIDs[minerI]].cBlake_output_buffer_map = (cl_ulong *) clEnqueueMapBuffer(gpuMinerContexts[(int) gpuIDs[minerI]].queue, gpuMinerContexts[(int) gpuIDs[minerI]].cBlake_output_buffer,CL_TRUE,CL_MAP_READ, 0, 8 * sizeof(cl_ulong), 0,NULL,&gpuMinerContexts[(int) gpuIDs[minerI]].readQueue[0],&err);
    gpuMinerContexts[(int) gpuIDs[minerI]].has_nonce_buffer_map = (cl_int *) clEnqueueMapBuffer(gpuMinerContexts[(int) gpuIDs[minerI]].queue, gpuMinerContexts[(int) gpuIDs[minerI]].has_nonce_buffer,CL_TRUE,CL_MAP_READ, 0, sizeof(cl_int), 0,NULL,&gpuMinerContexts[(int) gpuIDs[minerI]].readQueue[1], &err);
    gpuMinerContexts[(int) gpuIDs[minerI]].nonce_out_buffer_map = (cl_ulong *) clEnqueueMapBuffer(gpuMinerContexts[(int) gpuIDs[minerI]].queue, gpuMinerContexts[(int) gpuIDs[minerI]].nonce_out_buffer,CL_TRUE,CL_MAP_READ, 0, 8 * sizeof(cl_ulong), 0,NULL,&gpuMinerContexts[(int) gpuIDs[minerI]].readQueue[2], &err);
   */
    
    //}
    //clWaitForEvents(3,gpuMinerContexts[(int) gpuIDs[minerI]].readQueue); 
     //prob dont need this here unless we solve tho
    clWaitForEvents(3,gpuMinerContexts[(int) gpuIDs[minerI]].readQueue); 
    
    int clHasSolved = (int) gpuMinerContexts[(int) gpuIDs[minerI]].hasNonce[0];
     //clHasSolved = 1;
    if(clHasSolved == 1 && gpuMinerContexts[(int) gpuIDs[minerI]].awaitingWork == 0){
      //clFinish(gpuMinerContexts[(int) gpuIDs[minerI]].queue);

      cl_ulong *out = gpuMinerContexts[(int) gpuIDs[minerI]].output;
      cl_ulong *nonceOut = gpuMinerContexts[(int) gpuIDs[minerI]].nonceOutput; 
      for(i=0;i<8/2;i++){
        sprintf(outputData+i*16,"%016llx",out[i]);
      }

      for(i=0;i<8/2;i++){
        sprintf(outputNonceData+(i*16),"%016llx",nonceOut[i]);
      }
        //printf("\n SOLLLVVVEEEEEDD!");
      printResultsJSON(gpuMinerContexts[(int) gpuIDs[minerI]].headerString,outputData,inputLen,outputNonceData,1,gpuMinerContexts[(int) gpuIDs[minerI]].gpuID,gpuMinerContexts[(int) gpuIDs[minerI]].platformID);
      
      //nonceDiff = (unsigned long) nonceOut[0];
      //nonceDiff = gpuMinerContexts[(int)gpuIDs[minerI]].lastNonce;
      //unsigned long hashPS = nonceDiff / timeDiff;
      
      //updateStatus(gpuMinerContexts[(int) gpuIDs[minerI]].headerString, lastNonce,hashPS,gpuMinerContexts[(int) gpuIDs[minerI]].gpuID,gpuMinerContexts[(int) gpuIDs[minerI]].platformID);
      
      //gpuMinerContexts[(int)gpuIDs[minerI]].lastNonce = 0x0; //reset me

      //lastTime = (cl_ulong)time(NULL);

      //nonceDiff = 0x0;
      if(miningMode == 0){
        //we are solo so we know we can stop and await next work
        gpuMinerContexts[(int) gpuIDs[minerI]].awaitingWork = 1;
      }
      
      gpuMinerContexts[(int) gpuIDs[minerI]].hasNewWork = 0;
      clHasSolved = 0;
      gpuMinerContexts[(int) gpuIDs[minerI]].hasNonce[0] = 0;
        //hasSolved = 1; //!!!!!1!!!
    }

    /*if(clHasSolved == 11 && gpuMinerContexts[(int) gpuIDs[minerI]].awaitingWork == 0){
      //cl kernel overflowed the nonce, lets get a new header
      printf("{\"event\":\"nonceFull\",\"gpuID\":\"%i\", \"platformID\":\"%i\", \"nonce\":\"%016llx\"}\r\n",gpuMinerContexts[(int) gpuIDs[minerI]].gpuID,gpuMinerContexts[(int) gpuIDs[minerI]].platformID,nonceAsUlong[0]);
      fflush(stdout);
    }*/
    gpuMinerContexts[(int) gpuIDs[minerI]].hasNonce[0] = 0;
    /*clEnqueueUnmapMemObject(gpuMinerContexts[(int) gpuIDs[minerI]].queue,gpuMinerContexts[(int) gpuIDs[minerI]].nonce_out_buffer,gpuMinerContexts[(int) gpuIDs[minerI]].nonce_out_buffer_map,0,NULL,NULL);
    clEnqueueUnmapMemObject(gpuMinerContexts[(int) gpuIDs[minerI]].queue,gpuMinerContexts[(int) gpuIDs[minerI]].has_nonce_buffer,gpuMinerContexts[(int) gpuIDs[minerI]].has_nonce_buffer_map,0,NULL,NULL);
    clEnqueueUnmapMemObject(gpuMinerContexts[(int) gpuIDs[minerI]].queue,gpuMinerContexts[(int) gpuIDs[minerI]].cBlake_output_buffer,gpuMinerContexts[(int) gpuIDs[minerI]].cBlake_output_buffer_map,0,NULL,NULL);
    */
    
  }
  minerIterations += 0x0000000000000001;
  if((int)timeDiff>= 10){
    //printf("\n miner iterations %06llu loops per sec %i",minerIterations,(minerIterations-iterationsLast) / timeDiff );
    iterationsLast = minerIterations;
    lastTime = (cl_ulong)time(NULL);
  }
  if(shouldWeSleep == gpuCount){

#ifdef WIN32
    //printf("\n sleeping has win32");
    sleep(250);
#endif
#ifndef WIN32
    //printf("\n previous res %016llu",(unsigned long) previousResult);
    //printf("\n sleeping no win32");
    
    usleep(250000);
#endif
    //say we had 1 gpu, it submitted a solution
    //then in this while loop it'll blast the work file with so many requests
    //that it blocks out our write of the next block.
    //thus if all the GPU's say they're awaiting work, we'll not block anything here.
   //hasSolved = 1; //THIS TELLS THINGS TO END
  }

}


   /* Deallocate resources */
   clReleaseKernel(kernel);
   /*clReleaseMemObject(cBlake_output_buffer);
   clReleaseMemObject(input_buffer);
   clReleaseMemObject(input_len_buffer);
   clReleaseMemObject(key_buffer);
   clReleaseMemObject(keyLen_buffer);
   clReleaseMemObject(nonce_out_buffer);
   clReleaseMemObject(nonce_as_ulong_buffer);
   clReleaseMemObject(has_nonce_buffer);
   clReleaseMemObject(target_buffer);
   clReleaseCommandQueue(queue);*/
   clReleaseProgram(program);
   clReleaseContext(context);
}
int updateStatus(char *testInput, cl_ulong nonce, unsigned long hashesPS, int gpuID, int platformID){
   fprintf(stdout,"{\"input\":\"%s\",\"nonce\":\"%016llx\",\"type\":\"status\", \"hashRate\":%llu,\"gpuID\":%i,\"platformID\":%i}\r\n",testInput,nonce ,hashesPS,gpuID,platformID);
   fflush(stdout);
   return 0; 
}
int printResultsJSON(char *testInput, char *outputData, int inputLen,char *nonceOut, int solvedTarget, int gpuID, int platformID){
   
  fprintf(stdout,"{\"hash\":\"%s\",\"nonce\":\"%s\",\"type\":\"solution\", \"solvedTarget\":true,\"gpuID\":%i,\"platformID\":%i}\r\n",outputData,nonceOut,gpuID,platformID);
  fflush(stdout);
   return 0;
}
