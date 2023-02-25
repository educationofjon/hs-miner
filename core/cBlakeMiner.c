#define PROGRAM_FILE "cBLAKE.cl"
#define KERNEL_FUNC "cBlake"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef MAC
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif


/* Find a GPU or CPU associated with the first available platform */
cl_device_id create_device() {

   cl_platform_id platform;
   cl_device_id dev;
   int err;

   char* value;
   size_t valueSize;
   cl_uint platformCount;
   cl_platform_id* platforms;
   cl_uint deviceCount;
   cl_device_id* devices;

   /* Identify a platform */
   err = clGetPlatformIDs(1, &platform, NULL);
   if(err < 0) {
      perror("Couldn't identify a platform");
      exit(1);
   }


   clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &deviceCount);
   devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
   clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, deviceCount, devices, NULL);

   /* Access a device */
   err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
   if(err == CL_DEVICE_NOT_FOUND) {
      printf("\nNOO, EFFING GPU WASNT FOUND!? %s\n","");
      err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
   }
   if(err < 0) {
      perror("Couldn't access any devices");
      exit(1);
   }

   /*for (int j = 0; j < deviceCount; j++) {

      // print device name
      clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 0, NULL, &valueSize);
      value = (char*) malloc(valueSize);
      clGetDeviceInfo(devices[j], CL_DEVICE_NAME, valueSize, value, NULL);
      printf("%d. Device: %s\n", j+1, value);
      free(value);
   }*/

   #ifdef MAC
      return devices[1];
   #else
      return dev;
   #endif
}

/* Create program from a file and compile it */
cl_program build_program(cl_context ctx, cl_device_id dev, const char* filename) {

   cl_program program;
   FILE *program_handle;
   char *program_buffer, *program_log;
   size_t program_size, log_size;
   int err;

   /* Read program file and place content into buffer */
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

   /* Create program from file */
   program = clCreateProgramWithSource(ctx, 1,
      (const char**)&program_buffer, &program_size, &err);
   if(err < 0) {
      perror("Couldn't create the program");
      exit(1);
   }
   free(program_buffer);

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

int main( int argc, char *argv[]) {
   unsigned long nonce = 0;


   unsigned long output[8];
   /* OpenCL structures */
   cl_device_id device;
   cl_context context;
   cl_program program;
   cl_kernel kernel;
   cl_command_queue queue;
   cl_int i, err;

   size_t local_size, global_size;
   int clMinerThreads = 1;

   /* Data and buffers */
   cl_mem bitCapacity_buffer, delimitedSuffix_buffer;
   cl_mem input_buffer, cBlake_output_buffer, input_len_buffer, outputLen_buffer, key_buffer, keyLen_buffer, target_buffer;
   cl_mem nonce_out_buffer, has_nonce_buffer, nonce_as_ulong_buffer;

   /* Create device and context */
   device = create_device();
   context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
   if(err < 0) {
      perror("Couldn't create a context");
      exit(1);
   }

   /* Build program */
   program = build_program(context, device, PROGRAM_FILE);

   /* Create data buffer */
   #ifdef MAC
      global_size = 4194304;//16384;
      local_size = 128;
   #else
      global_size = 2097152;//16384;
      local_size = 512;
   #endif

   /*global_size = 1;
   local_size = 1;*/


   /* Create a command queue */
   queue = clCreateCommandQueue(context, device, 0, &err);
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
   int hasSolved = 0;
   cl_int *inputLength = (cl_int*)malloc(sizeof(cl_int));
   cl_char *testInput;

   cl_int *outputLen = (cl_int*)malloc(sizeof(cl_int));
   outputLen[0] = (int) 512;
   cl_char *key;
   cl_int *keyLen = (cl_int*)malloc(sizeof(cl_int));
   cl_int *hasNonce = (cl_int*)malloc(sizeof(cl_int)); //1 || 0 we found a nonce that satisfies target
   unsigned long* nonceOutput = (uint8_t*)malloc(8*sizeof(unsigned long)); //only need to return the best one

   unsigned long lastTime = (unsigned long)time(NULL);
   unsigned long timeDiff;
   unsigned long lastNonce = 0;
   unsigned long nonceDiff;


   char *outputData = (char*) malloc(64*sizeof(char));
   char *outputNonceData = (char*) malloc(64*sizeof(char));
   char *kmacOutPrettified = (char*) malloc(64*sizeof(char));

   inputLength[0] = strlen(argv[1]) / 2;
   int inputLen = (int) strlen(argv[1]) / 2;

   testInput = (cl_char*)malloc(strlen(argv[1]));

   uint8_t* inputBytes;
   //inputBytes = (uint8_t*)malloc(strlen(argv[1])/2);

   char tmp[3];
   tmp[2] = '\0';

   char tmp2[9];
   tmp2[8] = '\0';

   int rB,kkLen,krB, nonceUlPos;

   uint8_t* keyBytes;
   //keyBytes = (uint8_t*)malloc(strlen(argv[2])/2);

   for(int i=0;i<strlen(argv[1]);i++){
      testInput[i] = argv[1][i];
   }

   //only set inputBytes on init, and todo: socket update

   kkLen = (int) strlen(argv[2]);

   key = (cl_char*)malloc(strlen(argv[2]));




   /*unsigned long lastTime = (unsigned long)time(NULL);
   unsigned long timeDiff;
   unsigned long lastNonce = 0;
   unsigned long nonceDiff;*/

   unsigned long nonceOffset = 0x0;
   unsigned long minerIterations = 0x0;

   keyLen[0] = strlen(argv[2]) / 2;
   kkLen = (int) strlen(argv[2]) / 2;

   int len = strlen(argv[3]) || 1;
   unsigned long* targetBytes;
   unsigned long* nonceAsUlong;


   //nonceAsUlong = (unsigned long*)malloc(8);

   int tB = 0;
   for(int i=0;i<strlen(argv[2]);i++){
      key[i] = argv[2][i];
   }

while(hasSolved == 0){
   keyBytes = (uint8_t*)malloc(strlen(argv[2])/2);
   inputBytes = (uint8_t*)malloc(strlen(argv[1])/2);
   targetBytes = (unsigned long*)malloc(8*sizeof(unsigned long));
   nonceAsUlong = (unsigned long*)malloc(8*sizeof(unsigned long));
   hasNonce = (cl_int*)malloc(sizeof(cl_int));
   //reset garbage again
   keyLen[0] = strlen(argv[2]) / 2;
   kkLen = (int) strlen(argv[2]) / 2;
   inputLength[0] = strlen(argv[1]) / 2;
   inputLen = (int) strlen(argv[1]) / 2;
   outputLen[0] = (int) 512;

   nonce = (unsigned long) (minerIterations * (int) global_size * clMinerThreads );

   unsigned long timeNow = (unsigned long)time(NULL);
   timeDiff = timeNow - lastTime;
   nonceDiff = nonce - lastNonce;

   nonceUlPos = 0;
   #pragma unroll
   for(int i=0;i<strlen(argv[2]);i+=8){
      tmp2[0] = argv[2][i+0];
      tmp2[1] = argv[2][i+1];
      tmp2[2] = argv[2][i+2];
      tmp2[3] = argv[2][i+3];
      tmp2[4] = argv[2][i+4];
      tmp2[5] = argv[2][i+5];
      tmp2[6] = argv[2][i+6];
      tmp2[7] = argv[2][i+7];
      nonceAsUlong[nonceUlPos] = strtoul(tmp2,NULL,16);
      nonceUlPos++;
   }
   //printf("\nnonceasulong 0 %016llx 1 %016llx",nonceAsUlong[0],nonceAsUlong[1]);
   nonceAsUlong[1] = 0x0000000000000000;

   if((int)timeDiff>= 20){

      lastTime = timeNow;
      lastNonce = nonce;
      int hashPS = nonceDiff / 20;
      nonce += nonceAsUlong[0];
      //printf("\ntimediff %i noncediff %f hashps %i",nonceDiff,(float)nonceDiff,hashPS);
      updateStatus(argv[1],nonce,hashPS);
      //printResultsJSON(testInput,outputData,inputLen,key);
   }

   /*annoyingly do this in the loop because openCL. sigh. */


   rB = 0;
   #pragma unroll
   for(int i=0;i<strlen(argv[1]);i+=2){
      tmp[0] = argv[1][i+0];
      tmp[1] = argv[1][i+1];
      inputBytes[rB] = strtol(tmp,NULL,16); //
      //printf("\nuint8_t char %i %02lx %02lu",rB,inputBytes[rB],inputBytes[rB]);
      rB++;
   }

   krB = 0;
   #pragma unroll
   for(int i=0;i<strlen(argv[2]);i+=2){
      tmp[0] = argv[2][i+0];
      tmp[1] = argv[2][i+1];
      keyBytes[krB] = strtol(tmp,NULL,16);
      //printf("\n nonce char isset %i %02x",i,keyBytes[krB]);
      krB++;
   }

   tB = 0;

   #pragma unroll
   for(int i=0;i<strlen(argv[3]);i+=8){
      tmp2[0] = argv[3][i+0];
      tmp2[1] = argv[3][i+1];
      tmp2[2] = argv[3][i+2];
      tmp2[3] = argv[3][i+3];
      tmp2[4] = argv[3][i+4];
      tmp2[5] = argv[3][i+5];
      tmp2[6] = argv[3][i+6];
      tmp2[7] = argv[3][i+7];
      targetBytes[tB] = strtoul(tmp2,NULL,16) ;
      tB++;
   }

   //targetBytes[0] = ((targetBytes[0] << 32 & 0xFFFFFFFF00000000) | targetBytes[1] & 0x00000000FFFFFFFF);
   //targetBytes[0] <<= 8;
   targetBytes[0] = (unsigned long*) ((targetBytes[0] << 40) & 0xFFFFFFFF00000000 | targetBytes[1] << 8 & 0x000000FF00000000 | targetBytes[1] << 8 & 0x00000000FFFFFFFF);

   //targetBytes[0] = (unsigned long*) ((targetBytes[0] << 40) & 0xFFFFFFFF00000000 | targetBytes[1] << 8 & 0x000000FF00000000 | targetBytes[1] << 8 & 0x00000000FFFFFFFF);
   targetBytes[1] = 0x0000000000000000;
   /*printf("\n target bytes in C land %016llx %016llx",targetBytes[0],targetBytes[1]);
   exit(0);*/
   /*end sigh*/

   nonceOffset = (unsigned long) (minerIterations * (unsigned long)( (unsigned long)((int)global_size-1) * clMinerThreads));

   nonceAsUlong[0] += nonceOffset;
   nonceAsUlong[0] = (nonceAsUlong[0] << 32 & 0xFFFFFFFF00000000);
   minerIterations += 0x0000000000000001;

   //printf("\n nonceas ullong %016llx minerIterations: %016llu global_size: %i clMinerThreads: %i \n",nonceAsUlong[0],minerIterations,global_size,clMinerThreads);


   input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY |
         CL_MEM_COPY_HOST_PTR, inputLen * sizeof(unsigned char), inputBytes, &err);
   cBlake_output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE |
         CL_MEM_COPY_HOST_PTR, 8 * sizeof(unsigned long), output, &err);
   input_len_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE |
         CL_MEM_COPY_HOST_PTR, sizeof(cl_int), inputLength, &err);
   key_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY |
         CL_MEM_COPY_HOST_PTR, kkLen * sizeof(unsigned char), keyBytes, &err);
   keyLen_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE |
         CL_MEM_COPY_HOST_PTR, sizeof(cl_int), keyLen, &err);
   outputLen_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE |
         CL_MEM_COPY_HOST_PTR, sizeof(cl_int), outputLen, &err);
   target_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY |
         CL_MEM_COPY_HOST_PTR, 8 * sizeof(unsigned long), targetBytes, &err);
   has_nonce_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE |
         CL_MEM_COPY_HOST_PTR, sizeof(cl_int), hasNonce, &err);
   nonce_out_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE |
         CL_MEM_COPY_HOST_PTR, 8 * sizeof(unsigned long), nonceOutput, &err);
   nonce_as_ulong_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY |
         CL_MEM_COPY_HOST_PTR, 8 * sizeof(unsigned long), nonceAsUlong, &err);


   if(err < 0) {
      perror("Couldn't create a buffer");
      exit(1);
   };
   /* Create kernel arguments */
   err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer);
   err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &cBlake_output_buffer);
   err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &input_len_buffer);
   err |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &outputLen_buffer);
   err |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &key_buffer);
   err |= clSetKernelArg(kernel, 5, sizeof(cl_mem), &keyLen_buffer);
   err |= clSetKernelArg(kernel, 6, sizeof(cl_mem), &target_buffer);
   err |= clSetKernelArg(kernel, 7, sizeof(cl_mem), &has_nonce_buffer);
   err |= clSetKernelArg(kernel, 8, sizeof(cl_mem), &nonce_out_buffer);
   err |= clSetKernelArg(kernel, 9, sizeof(cl_mem), &nonce_as_ulong_buffer);


   /* Enqueue kernel */
   err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size,
         &local_size, 0, NULL, NULL);
   if(err < 0) {
      perror("Couldn't enqueue the kernel");
      exit(1);
   }

   //clFinish(queue);

   /* Read the kernel's output */
   err = clEnqueueReadBuffer(queue, cBlake_output_buffer, CL_TRUE, 0,
         sizeof(output), output, 0, NULL, NULL);
   err |= clEnqueueReadBuffer(queue, input_len_buffer, CL_TRUE, 0,
         sizeof(int), inputLength, 0, NULL, NULL);
   err |= clEnqueueReadBuffer(queue, nonce_out_buffer, CL_TRUE, 0,
         sizeof(unsigned long) * 8, nonceOutput, 0, NULL, NULL);
   err |= clEnqueueReadBuffer(queue, has_nonce_buffer, CL_TRUE, 0,
         sizeof(int), hasNonce, 0, NULL, NULL);

   if(err < 0) {
      perror("Couldn't read the buffer");
      exit(1);
   }

   //prob dont need this here unless we solve tho
   #pragma unroll
   for(i=0;i<sizeof(output)/sizeof(unsigned long)/2;i++){
      sprintf(outputData+i*16,"%016llx",output[i]);
   }
   #pragma unroll
   for(i=0;i<sizeof(output)/sizeof(unsigned long)/2;i++){
      sprintf(outputNonceData+(i*16),"%016llx",nonceOutput[i]);
   }
   int clHasSolved = (int) hasNonce[0];

   if(clHasSolved == 1){

         printResultsJSON(testInput,outputData,inputLen,outputNonceData,1);
      hasSolved = 1; //!!!!!1!!!
   }
   hasNonce[0] = 0;

   free(keyBytes);
   free(inputBytes);
   free(targetBytes);
   free(nonceAsUlong);
   //hasSolved = 1; //THIS TELLS THINGS TO END

}


   /* Deallocate resources */
   clReleaseKernel(kernel);
   clReleaseMemObject(cBlake_output_buffer);
   clReleaseMemObject(input_buffer);
   clReleaseMemObject(input_len_buffer);
   clReleaseMemObject(key_buffer);
   clReleaseMemObject(keyLen_buffer);
   clReleaseMemObject(nonce_out_buffer);
   clReleaseMemObject(nonce_as_ulong_buffer);
   clReleaseMemObject(has_nonce_buffer);
   clReleaseMemObject(target_buffer);
   clReleaseCommandQueue(queue);
   clReleaseProgram(program);
   clReleaseContext(context);
   return 0;
}
int updateStatus(char *testInput, unsigned long* nonce, int hashesPS){
   printf("{\"input\":\"%s\",\"nonce\":\"%016llx\",\"type\":\"status\", \"hashRate\":%i}\n",testInput,nonce,hashesPS);
   return 0;
}
int printResultsJSON(char *testInput, char *outputData, int inputLen,char *nonceOut, int solvedTarget){

   printf("{\"hash\":\"%s\",\"nonce\":\"%s\",\"type\":\"solution\", \"solvedTarget\":true}\n",outputData,nonceOut);

   return 0;
}
