#define PROGRAM_FILE "runoff.cl"
#define KERNEL_FUNC1 "add"
#define KERNEL_FUNC2 "subtract"
#define KERNEL_FUNC3 "ddrain"

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <assert.h>
#include <malloc.h>
#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#define min( a, b ) ( ((a) < (b)) ? (a) : (b) )

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include "CL/cl.h"
#endif



#ifdef _WIN32
#include <windows.h>
#include <conio.h> /* obtain _kbhit() and _getch() for killswitch - Ryan Spies */

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timeZone
{
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timeZone *tz)
{
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag;

    if (NULL != tv){
	    GetSystemTimeAsFileTime(&ft);

	    tmpres |= ft.dwHighDateTime;
	    tmpres <<= 32;
	    tmpres |= ft.dwLowDateTime;

	    /*converting file time to unix epoch*/
	    tmpres -= DELTA_EPOCH_IN_MICROSECS;
	    tmpres /= 10;  /*convert into microseconds*/
	    tv->tv_sec = (long)(tmpres / 1000000UL);
	    tv->tv_usec = (long)(tmpres % 1000000UL);
    }

    if (NULL != tz){
	    if (!tzflag){
	        _tzset();
	        tzflag++;
	    }
	    tz->tz_minuteswest = _timezone / 60;
	    tz->tz_dsttime = _daylight;
	}
	return 0;
}
#else
// If we aren't using Windows, get getch() and kbhit() this way - Ryan Spies
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

struct termios orig_termios;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int getch()
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}
#endif

cl_uint get_num_devices(int opencl) {
    cl_platform_id *platforms;
    int err;
    cl_uint num_platforms, num_devices; //must be uint

    /* Identify all platforms platforms */
    clGetPlatformIDs(0, NULL, &num_platforms);
    // printf("There are %d platforms \n", num_platforms);
    platforms = (cl_platform_id*) malloc (num_platforms*sizeof(cl_platform_id));
    err = clGetPlatformIDs(2, platforms, &num_platforms);

    if(err < 0) {
        perror("Couldn't identify a platform");
        exit(1);
    }

    if (opencl == 0){
        err = clGetDeviceIDs(platforms[opencl], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
        if(err == CL_DEVICE_NOT_FOUND) {
            err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
        }
        if(err < 0) {
            perror("Couldn't access any devices");
            exit(1);
        }
    }

    if (opencl == 1){
        err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_CPU, 0, NULL, &num_devices);
        if(err == CL_DEVICE_NOT_FOUND) {
            err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_CPU, 0, NULL, &num_devices);
        }
        if(err < 0) {
            perror("Couldn't access any devices");
            exit(1);
        }
    }

    return num_devices;
}

 /* Find a GPU or CPU associated with the first available platform */
cl_device_id* create_devices(int opencl, cl_uint num_devices) {
    cl_platform_id *platforms;
    cl_device_id* dev;
    int err;
    cl_uint num_platforms; //must be uint

    /* Identify all platforms platforms */
    clGetPlatformIDs(0, NULL, &num_platforms);
    // printf("There are %d platforms \n", num_platforms);
    platforms = (cl_platform_id*) malloc (num_platforms*sizeof(cl_platform_id));
    err = clGetPlatformIDs(2, platforms, &num_platforms);
    dev = (cl_device_id *)malloc(num_devices * sizeof(cl_device_id) );

    if(err < 0) {
        perror("Couldn't identify a platform");
        exit(1);
    }

    // Choose all available devices of one type - Ryan Spies
    if (opencl == 0){
        err = clGetDeviceIDs(platforms[opencl], CL_DEVICE_TYPE_GPU, num_devices, dev, NULL);
        if(err == CL_DEVICE_NOT_FOUND) {
	    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, num_devices, dev, NULL);
        }
        if(err < 0) {
	    perror("Couldn't access any devices");
	    exit(1);
        }
    }

    if (opencl == 1){
        err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_CPU, 1, dev, NULL);
        if(err == CL_DEVICE_NOT_FOUND) {
	    err = clGetDeviceIDs(platforms[opencl-1], CL_DEVICE_TYPE_CPU, 1, dev, NULL);
        }
        if(err < 0) {
	    perror("Couldn't access any devices");
	    exit(1);
        }
    }

    return dev;
}

/* Create program from a file and compile it */
cl_program build_program(cl_context ctx, cl_device_id* dev, cl_uint num_devices, const char* filename) {

    cl_program program;
    FILE *program_handle;
    char *program_buffer, *program_log;
    size_t program_size, log_size;
    int err, i;

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
    program = clCreateProgramWithSource(ctx, 1, (const char**)&program_buffer, &program_size, &err);
    if(err < 0) {
        perror("Couldn't create the program");
        exit(1);
    }
    free(program_buffer);

    /* Build program */
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if(err < 0) {
        /* Find size of log and print to std output */
        for(i=0;i<num_devices;i++){
            clGetProgramBuildInfo(program, dev[i], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
            program_log = (char*) malloc(log_size + 1);
            program_log[log_size] = '\0';
            clGetProgramBuildInfo(program, dev[i], CL_PROGRAM_BUILD_LOG, log_size + 1, program_log, NULL);
            printf("%s\n", program_log);
            free(program_log);
        }
    exit(1);
    }
    return program;
}

const char* CLErrorToString(cl_int error) {
    switch( error ) {
        case CL_SUCCESS:                         return "Success.";
        case CL_DEVICE_NOT_FOUND:                return "Device not found.";
        case CL_DEVICE_NOT_AVAILABLE:            return "Device not available";
        case CL_COMPILER_NOT_AVAILABLE:          return "Compiler not available";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:   return "Memory object allocation failure";
        case CL_OUT_OF_RESOURCES:                return "Out of resources";
        case CL_OUT_OF_HOST_MEMORY:              return "Out of host memory";
        case CL_PROFILING_INFO_NOT_AVAILABLE:    return "Profiling information not available";
        case CL_MEM_COPY_OVERLAP:                return "Memory copy overlap";
        case CL_IMAGE_FORMAT_MISMATCH:           return "Image format mismatch";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:      return "Image format not supported";
        case CL_BUILD_PROGRAM_FAILURE:           return "Program build failure";
        case CL_MAP_FAILURE:                     return "Map failure";
        case CL_INVALID_VALUE:                   return "Invalid value";
        case CL_INVALID_DEVICE_TYPE:             return "Invalid device type";
        case CL_INVALID_PLATFORM:                return "Invalid platform";
        case CL_INVALID_DEVICE:                  return "Invalid device";
        case CL_INVALID_CONTEXT:                 return "Invalid context";
        case CL_INVALID_QUEUE_PROPERTIES:        return "Invalid queue properties";
        case CL_INVALID_COMMAND_QUEUE:           return "Invalid command queue";
        case CL_INVALID_HOST_PTR:                return "Invalid host pointer";
        case CL_INVALID_MEM_OBJECT:              return "Invalid memory object";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: return "Invalid image format descriptor";
        case CL_INVALID_IMAGE_SIZE:              return "Invalid image size";
        case CL_INVALID_SAMPLER:                 return "Invalid sampler";
        case CL_INVALID_BINARY:                  return "Invalid binary";
        case CL_INVALID_BUILD_OPTIONS:           return "Invalid build options";
        case CL_INVALID_PROGRAM:                 return "Invalid program";
        case CL_INVALID_PROGRAM_EXECUTABLE:      return "Invalid program executable";
        case CL_INVALID_KERNEL_NAME:             return "Invalid kernel name";
        case CL_INVALID_KERNEL_DEFINITION:       return "Invalid kernel definition";
        case CL_INVALID_KERNEL:                  return "Invalid kernel";
        case CL_INVALID_ARG_INDEX:               return "Invalid argument index";
        case CL_INVALID_ARG_VALUE:               return "Invalid argument value";
        case CL_INVALID_ARG_SIZE:                return "Invalid argument size";
        case CL_INVALID_KERNEL_ARGS:             return "Invalid kernel arguments";
        case CL_INVALID_WORK_DIMENSION:          return "Invalid work dimension";
        case CL_INVALID_WORK_GROUP_SIZE:         return "Invalid work group size";
        case CL_INVALID_WORK_ITEM_SIZE:          return "Invalid work item size";
        case CL_INVALID_GLOBAL_OFFSET:           return "Invalid global offset";
        case CL_INVALID_EVENT_WAIT_LIST:         return "Invalid event wait list";
        case CL_INVALID_EVENT:                   return "Invalid event";
        case CL_INVALID_OPERATION:               return "Invalid operation";
        case CL_INVALID_GL_OBJECT:               return "Invalid OpenGL object";
        case CL_INVALID_BUFFER_SIZE:             return "Invalid buffer size";
        case CL_INVALID_MIP_LEVEL:               return "Invalid mip-map level";
    }

    printf("Error is %d\n", error);
    return "Unknown";
}


void exitOnFail(cl_int err, const char* message)
{
    if (CL_SUCCESS != err){
        printf("error: %s %s %s%s%s\n", message, "with OpenCL error", "(",  CLErrorToString(err), ")");
        exit(-1);
    }
}

// Global variable
double **dem, **water, **oldwater, **diff, **bigdem, **bigwater;
double missingvalue, totaldrain, dem_header_value[10];
char dem_header_name[100], activity[20];
char **dem_header_line;
int numcols, numrows, drainrow, draincol;
int draincols[4];

/*
 * Functions Declarations
 */
int file_exist(char *filename);
char *upcase(char *);
void write_gis(char filename[80], int numrows, int numcols, char* header_line[8], double header_value[8]);
void read_water_array(char filename[80], int numrows, int numcols);
void read_dem_array(char filename[80], int numrows, int numcols);
void read_gis_header(char filename[80], bool print_output, char* header_line[8], double header_value[8]);
void print_copyright(char *);
void print_arg_list();
void print_narg_list();
void print_args (char demfile[80], char waterfile[80], char outputfile[80],
                      char scratchfile[80], double addwater, double subtractwater, double rof,
		            double eltol, double draintol, char *, int cpu, double thres, int iterlim);
void print_results(double initial_vol, double final_vol, double drainvol, double waterfrac,
                       double meanwater, double draindepth, double maxdepth, int iterlim);
void runoffa(int centerrow, int centercol, double missingvalue);
void runoffs(int centerrow, int centercol, double missingvalue);
void runoffd(int centerrow, int centercol, int drainrow, int draincol, double missingvalue);
void print_iteration_summary_headings();
void print_basin_summary(double basin_area, double initial_vol, int drainrow, int draincol, double minel);
double drain(int drainrow, int draincol);

int main(int argc, char *argv[])
{
#ifdef _WIN32
    FreeConsole();
#endif
    /* OpenCL structures */
    cl_device_id* devices;
    cl_context context;
    cl_program program;
    cl_kernel* kernels;
    cl_command_queue* queues;
    cl_int ii, jj, err, ll;
    cl_event event;
    cl_event* events;
    /* buffers */
    cl_mem input_buffer, sum_buffer;
    cl_int num_groups;
    cl_uint num_devices;

    cl_mem *d_water, *d_dem, *d_totaldrain;
    cl_mem dd_water, dd_dem;
    cl_mem d_bigwater, d_bigdem;
    double *bbigdem,*bbigwater,*btotaldrain;
    double **b_dem, **b_water, **m_water, **mm_water;
    double c_water;
    size_t bytes, byte1, byte2, byte3;

    char DEMFileName[80], OutputFileName[80], outformat[80];
    char InParameterFileName[80], WaterFileName[80], sliceformat[80];
    char ScratchFileName[80], drain_tolerancestring[10];
    char elevation_tolerancestring[10], addstring[10], iteration_limitstring[10];
    char subtractstring[10], rofstring[10], CpuString[10], GpuString[10], threshold[20];
    char exitChar;

    double max_diff, elevation_tolerance, drain_tolerance;
    double cellsize, cellarea, addwater, subtractwater, rof;
    double thres;
    double diffdrain, olddrain, mindrain, drainn;
    bool done, isKillSwitch, gracefulExit;
    int i, j, k, n, arg_count,basincount, watercount, row, col, iteration_limit;
    int slice, boundary, cpu, gpu;
    int cols_base, remaining_cols;
    int *offsets, *section_sizes, *iteration_cols;

    int *widths, *startcols, *endcols, *boundarystartcols, *boundaryendcols;
    double watertotal, waterfrac, meanwater, maxdepth, draindepth, drainvol;
    double initial_vol, final_vol, basin_area;
    struct timeval starttime, currenttime;
    //clock_t starttime;
    //clock_t currenttime;

    FILE * filez;
    char input[100];
	setbuf(stdout, NULL);
    switch (argc){
	    case 1:
		    print_narg_list();
		    exit(42);
	    case 2:
		    filez=fopen(argv[1],"r");
		    int i=1;
		while(!feof(filez)){
		    fscanf(filez,"%s",input);
		    if (i==1){ strcpy(activity, input); }
		    i++;
		}
		fclose(filez);
		break;
	    case 12:
		strcpy(activity, argv[1]);
		break;
	    case 13:
		strcpy(activity, argv[1]);
		break;
	    default:
	        strcpy(activity, argv[1]);
            printf("%s %d\n","Here's the number of arguments you gave: ",argc);
		print_arg_list();
		exit(42);
	}

    //print copyrights
    print_copyright(activity);

    // extract arguments
    if (strcmp(activity, "add") == 0) {
	    switch (argc){
	        case 2:
		    filez=fopen(argv[1],"r");
		    int i=1;
		    while(!feof(filez))
		    {
		        fscanf(filez,"%s",input);
		        if (i==2){ strcpy(DEMFileName, input); }
		        if (i==3){ strcpy(WaterFileName, input); }
		        if (i==4){ strcpy(OutputFileName, input); }
		        if (i==5){ strcpy(ScratchFileName, input); }
		        if (i==6){ strcpy(addstring, input); }
		        if (i==7){ strcpy(rofstring, input); }
		        if (i==8){ strcpy(elevation_tolerancestring, input); }
		        if (i==9){ strcpy(CpuString, input); }
		        if (i==10){ strcpy(GpuString, input); }
		        if (i==11){strcpy(threshold, input); }
		        if (i==12){ strcpy(iteration_limitstring, input); }
                i++;
		    }
		    fclose(filez);
		    break;
	        case 13:
		    strcpy(DEMFileName, argv[2]);
		    strcpy(WaterFileName, argv[3]);
		    strcpy(OutputFileName, argv[4]);
		    strcpy(ScratchFileName, argv[5]);
		    strcpy(addstring, argv[6]);
		    strcpy(rofstring, argv[7]);
		    strcpy(elevation_tolerancestring, argv[8]);
		    strcpy(CpuString, argv[9]);
		    strcpy(GpuString, argv[10]);
		    strcpy(threshold, argv[11]);
            strcpy(iteration_limitstring, argv[12]);
		    break;
	        default:
            printf("%s %d\n","Here's the number of arguments you gave: ",argc);
		    print_arg_list();
		    exit(42);
	    }
	    // convert parameter strings to integer and reals
	    addwater=atof(addstring);
	    rof=atof(rofstring);
	    elevation_tolerance=atof(elevation_tolerancestring);
        iteration_limit = (int)atof(iteration_limitstring);
        if (iteration_limit == 0) {
            isKillSwitch = false;
        }
        else{
            isKillSwitch = true;
        }
	    cpu = (int)atof(CpuString);
	    gpu = (int)atof(GpuString);
	    thres = (double)atof(threshold);
	    // write parameters to stdout
	    print_args(DEMFileName, WaterFileName, OutputFileName, ScratchFileName,
		    addwater, 0, rof, elevation_tolerance, 0, activity, cpu, thres, iteration_limit);

	    // convert parameters from mm to m
	    elevation_tolerance = elevation_tolerance/1000.0;
	    addwater= addwater/1000.0;
	    thres = thres/1000;
    }
    else if (strcmp(activity, "subtract") == 0) {
	    switch (argc){
	        case 2:
		    filez=fopen(argv[1],"r");
		    int i=1;
		    while(!feof(filez)){
		        fscanf(filez,"%s",input);
		        if (i==2){ strcpy(DEMFileName, input); }
		        if (i==3){ strcpy(WaterFileName, input); }
		        if (i==4){ strcpy(OutputFileName, input); }
		        if (i==5){ strcpy(ScratchFileName, input); }
		        if (i==6){ strcpy(subtractstring, input); }
		        if (i==7){ strcpy(elevation_tolerancestring, input); }
		        if (i==8){ strcpy(CpuString, input); }
		        if (i==9){ strcpy(GpuString, input); }
		        if (i==10){ strcpy(threshold, input); }
            	if (i==11){ strcpy(iteration_limitstring, input); }
		        i++;
		    }
		    fclose(filez);
		    break;
	        case 12:
		    strcpy(DEMFileName, argv[2]);
		    strcpy(WaterFileName, argv[3]);
		    strcpy(OutputFileName, argv[4]);
		    strcpy(ScratchFileName, argv[5]);
		    strcpy(subtractstring, argv[6]);
		    strcpy(elevation_tolerancestring, argv[7]);
		    strcpy(CpuString, argv[8]);
		    strcpy(GpuString, argv[9]);
		    strcpy(threshold, argv[10]);
            strcpy(iteration_limitstring, argv[11]);
		    break;
	        default:
                printf("%s %d\n","Here's the number of arguments you gave: ",argc);
	        print_arg_list();
	        exit(42);
	    }
	    // convert parameter strings to integer and reals
	    subtractwater=atof(subtractstring);
	    elevation_tolerance=atof(elevation_tolerancestring);
        iteration_limit = (int)atof(iteration_limitstring);
        if (iteration_limit == 0) {
            isKillSwitch = false;
        }
        else{
            isKillSwitch = true;
        }
	    cpu = (int)atof(CpuString);
	    gpu = (int)atof(GpuString);
	    thres = (double)atof(threshold);
	    // write parameters to stdout
	    print_args(DEMFileName, WaterFileName, OutputFileName, ScratchFileName,
		    0, subtractwater, 0, elevation_tolerance, 0, activity, cpu, thres, iteration_limit);

	    // convert parameters from mm to m
	    elevation_tolerance = elevation_tolerance/1000.0;
	    subtractwater = subtractwater/1000;
	    thres = thres/1000;
    }
    else if (strcmp(activity, "drain") == 0) {
	    switch (argc)
	    {
	        case 2:
		    filez=fopen(argv[1],"r");
		    int i=1;
		    while(!feof(filez))
		    {
		        fscanf(filez,"%s",input);
		        if (i==2){ strcpy(DEMFileName, input); }
		        if (i==3){ strcpy(WaterFileName, input); }
		        if (i==4){ strcpy(OutputFileName, input); }
		        if (i==5){ strcpy(ScratchFileName, input); }
		        if (i==6){ strcpy(elevation_tolerancestring, input); }
		        if (i==7){ strcpy(drain_tolerancestring, input); }
		        if (i==8){ strcpy(CpuString, input); }
		        if (i==9){ strcpy(GpuString, input); }
		        if (i==10){ strcpy(threshold,input); }
            	if (i==11){ strcpy(iteration_limitstring, input); }
		        i++;
		    }
		    fclose(filez);
		    break;
	        case 12:
		    strcpy(DEMFileName, argv[2]);
		    strcpy(WaterFileName, argv[3]);
		    strcpy(OutputFileName, argv[4]);
		    strcpy(ScratchFileName, argv[5]);
		    strcpy(elevation_tolerancestring, argv[6]);
		    strcpy(drain_tolerancestring, argv[7]);
		    strcpy(CpuString, argv[8]);
		    strcpy(GpuString, argv[9]);
		    strcpy(threshold, argv[10]);
        	strcpy(iteration_limitstring, argv[11]);
		    break;
	        default:
            printf("%s %d\n","Here's the number of arguments you gave: ",argc);
		    print_arg_list();
		    exit(42);
	    }
	    // convert parameter strings to integer and reals
	    elevation_tolerance=atof(elevation_tolerancestring);
	    drain_tolerance=atof(drain_tolerancestring);
        iteration_limit = (int)atof(iteration_limitstring);
        if (iteration_limit == 0) {
            isKillSwitch = false;
        }
        else{
             isKillSwitch = true;
        }
	    cpu = (int)atof(CpuString);
	    gpu = (int)atof(GpuString);
	    thres = (double)atof(threshold);
	    // write parameters to stdout
	    print_args(DEMFileName, WaterFileName, OutputFileName, ScratchFileName,
		    0, 0, 0,  elevation_tolerance, drain_tolerance, activity, cpu, thres, iteration_limit);
	    // convert parameters from mm to m
	    elevation_tolerance = elevation_tolerance/1000.0;
	    thres = thres/1000;
    }
    else
    {
        printf("%s\n", "Error not a valid mode.");
        exit(42);
    }

    gracefulExit = false;

    // now open files to input arrays
    // get data from DEM header
    if (!DEMFileName){
        printf("%s\n", "Error DEM file missing");
        exit(42);
    }
    else{
	    // reads from ArcGIS file
	    FILE *fgheader;
	    fgheader=fopen(DEMFileName,"r");

	    dem_header_line = malloc(7 * sizeof(char *));
	    for (i=0; i<7; i++){
	        dem_header_line[i] = malloc(100 * sizeof(char));
	    }
	    i=1;
	    while(i<=6){
	        fscanf(fgheader, "%30s %lf", dem_header_name, &dem_header_value[i]);
	        strcpy(dem_header_line[i], dem_header_name);
	        i++;
	    }
	    fclose(fgheader);
    }


    // get numbers of rows & cols from header
    read_gis_header(DEMFileName, true, dem_header_line, dem_header_value);
    numcols = (int)dem_header_value[1];
    numrows = (int)dem_header_value[2];
    missingvalue = dem_header_value[6];
    cellsize = dem_header_value[5];
    cellarea = cellsize*cellsize;

    printf("%30s\n", "Setting array sizes");
    // set array sizes
    dem = malloc(numrows*sizeof(double *));
    water = malloc(numrows*sizeof(double *));
    oldwater = malloc((numrows+2)*sizeof(double *));
    diff = malloc((numrows+2)*sizeof(double *));
    bigdem = malloc((numrows+2)*sizeof(double *));
    bigwater = malloc((numrows+2)*sizeof(double *));
    m_water = malloc((numrows+2)*sizeof(double *));
    mm_water = malloc(2*sizeof(double *));

    for(i=0;i<(numrows+2); i++){
	    m_water[i]=malloc(4*sizeof(double));
    }

    for(i=0; i<2; i++){
	    mm_water[i]=malloc(2*(numrows+2)*sizeof(double));
    }	

    for (i=0; i<numrows; i++){
        dem[i] = malloc(numcols * sizeof(double));
        water[i] = malloc(numcols * sizeof(double));
    }
    for (i=0; i<numrows+2; i++){
        oldwater[i] = malloc((numcols+2) * sizeof(double));
        diff[i] = malloc((numcols+2) * sizeof(double));
        bigdem[i] = malloc((numcols+2) * sizeof(double));
        bigwater[i] = malloc((numcols+2) * sizeof(double));
    }
    for (i=0; i<numrows; i++){
        for (j=0; j<numcols; j++){
	        water[i][j]=0.0;
	        dem[i][j]=0.0;
        }
    }
    for (i=0; i<numrows+2; i++){
        for (j=0; j<numcols+2; j++){
	        bigwater[i][j]=0.0;
	        oldwater[i][j]=0.0;
	        bigdem[i][j]=0.0;
	        diff[i][j]=0.0;
        }
    }

    // read DEM data
    read_dem_array(DEMFileName, numrows, numcols);
    printf("%s\n","           ");

    int offset = 4;
    int IterationNum = 1000; //1000
    // get number of cells in array
    basincount=0;
    for(i=0; i<numrows; i++){
        for(j=0; j<numcols; j++){
            if( dem[i][j] > missingvalue ){
                basincount++;
            }
        }
    }
    if(cpu==1){
        // Something about the docs is wrong here. Here the input for the GPU is 0, but in the docs it's 1. - Ryan Spies
	    /* Create device and context */
	    num_devices = get_num_devices(gpu);
        //num_devices = 4;
        int balanced_cell = (int) basincount / num_devices;
        printf("The number of balance active cell is %d \n", balanced_cell);

        devices = (cl_device_id *)malloc(num_devices * sizeof(cl_device_id) );
	    devices = create_devices(gpu, num_devices);
	    d_water = (cl_mem *)malloc(num_devices * sizeof(cl_mem) );
	    d_dem = (cl_mem *)malloc(num_devices * sizeof(cl_mem) );
	    queues = (cl_command_queue *)malloc(num_devices * sizeof(cl_command_queue) );
	    kernels = (cl_kernel *)malloc(num_devices * sizeof(cl_kernel) );
	    offsets = malloc(num_devices * sizeof(int));
	    section_sizes = malloc(num_devices * sizeof(int));
	    iteration_cols = malloc(num_devices * sizeof(int));
	    d_totaldrain = (cl_mem *)malloc(num_devices * sizeof(cl_mem) );
        b_water = malloc(num_devices*sizeof(double *));
        b_dem = malloc(num_devices*sizeof(double *));
	    if (gpu==0){
	        printf("%40s\n","Using OpenCL GPU for Computation");
	    }
	    if (gpu==1){
	        printf("%40s\n","Using OpenCL CPU for Computation");
	    }

	    if (num_devices == 1){
		    offsets[0] = 0;
		    section_sizes[0] = numcols+2;
		    iteration_cols[0] = numcols;
	    }
        if (num_devices > 1){
            int cur_col = 0;
            for(i=0; i<num_devices; i++){
                if (i == 0){
                    int cur_count = 0;
                    for(j=0; j<numcols; j++){
                        for(k=0; k<numrows; k++){
                            if( dem[k][j] > missingvalue ){
                                cur_count++;
                                if (cur_count == balanced_cell)
                                    cur_col = j;
                            }
                        }
                    }
                    if ((cur_col+2)%3 == 0){
                        section_sizes[i] = cur_col;
                        iteration_cols[i] = cur_col - 2;
                        offsets[i] = 0;
                        cur_col = cur_col - 1;
                    }
                    else if ((cur_col+2)%3 == 1){
                        section_sizes[i] = cur_col + 2;
                        iteration_cols[i] = cur_col;
                        offsets[i] = 0;
                        cur_col = cur_col + 1;
                    }
                    else if ((cur_col+2)%3 == 2){
                        section_sizes[i] = cur_col + 1;
                        iteration_cols[i] = cur_col - 1;
                        offsets[i] = 0;
                        cur_col = cur_col;
                    }
                    printf("section: %d, iteration: %d, offset: %d, cur_col: %d \n", section_sizes[i], iteration_cols[i], offsets[i], cur_col);
                }
                else if(i == (num_devices-1)){
				    offsets[0] = 0;
				    section_sizes[i] = (numcols-1) - cur_col + 1 + 1;
				    iteration_cols[i] = section_sizes[i] - 2;
				    offsets[i] = offsets[i-1]+section_sizes[i-1]-1;
				    printf("section: %d, iteration: %d, offset: %d, cur_col: %d \n", section_sizes[i], iteration_cols[i], offsets[i], cur_col);
			    }
                else{
                    int cur_count = 0;
                    int start_col = cur_col;
                    for(j=start_col; j<numcols; j++){
                        for(k=0; k<numrows; k++){
                            if( dem[k][j] > missingvalue ){
                                cur_count++;
                                if (cur_count == balanced_cell)
                                    cur_col = j;
                            }
                        }
                    }
                    if ((cur_col-start_col+1)%3 == 0){
                        section_sizes[i] = cur_col - start_col + 2;
                        iteration_cols[i] = cur_col - start_col;
                        offsets[i] = offsets[i-1]+section_sizes[i-1]-1;
                        cur_col = cur_col + 2;
                    }
                    else if ((cur_col-start_col+1)%3 == 1){
                        section_sizes[i] = cur_col - start_col + 1;
                        iteration_cols[i] = cur_col -start_col - 1;
                        offsets[i] = offsets[i-1]+section_sizes[i-1]-1;
                        cur_col = cur_col + 1;
                    }
                    else if ((cur_col-start_col+1)%3 == 2){
                        section_sizes[i] = cur_col - start_col;
                        iteration_cols[i] = cur_col - start_col - 2;
                        offsets[i] = offsets[i-1]+section_sizes[i-1]-1;
                        cur_col = cur_col;
                    }
                    printf("section: %d, iteration: %d, offset: %d, cur_col: %d \n", section_sizes[i], iteration_cols[i], offsets[i], cur_col);
                }
                b_water[i] = malloc(section_sizes[i]*(numrows+2)*sizeof(double));
	            b_dem[i] = malloc(section_sizes[i]*(numrows+2)*sizeof(double));
            }
        }

	    printf("         section size    iteration column size    start from \n");
	    for(i=0; i<num_devices; i++){
		    printf("%d :         %d                  %d            %d   \n", i, section_sizes[i],iteration_cols[i],offsets[i]);
	    }

	    context = clCreateContext(NULL, num_devices, devices, NULL, NULL, &err);

	    if(err < 0) {
	        perror("Couldn't create a context");
	        exit(1);
	    }

	    /* Build program */
        program = build_program(context, devices, num_devices, PROGRAM_FILE);
	    /* Create a command queue on each device */
        // Updated this to use a more up to date OpenCL method. The WDPM still behaves the same with this new method. - Ryan Spies
        for (i=0;i<num_devices;i++){
            queues[i] = clCreateCommandQueue(context, devices[i], NULL, &err);
	        if(err < 0) {
                perror("Couldn't create a command queue");
                exit(1);
            };
	    }

	    /* Create a kernel */
	    for (i=0;i<num_devices;i++){
	        if (strcmp(activity, "add") == 0) {
	            kernels[i] = clCreateKernel(program, KERNEL_FUNC1, &err);
	        }
	        else if (strcmp(activity, "subtract") == 0) {
	            kernels[i] = clCreateKernel(program, KERNEL_FUNC2, &err);
	        }
	        else if (strcmp(activity, "drain") == 0) {
	            kernels[i] = clCreateKernel(program, KERNEL_FUNC3, &err);
	        }
	        if(err < 0) {
	            perror("Couldn't create a kernel");
	            exit(1);
     	    }
	    }

	    ll=(numrows+2)*(numcols+2);
	    bytes = ll*sizeof(double);
	    bbigdem = malloc(bytes);
	    bbigwater = malloc(bytes);
	    btotaldrain = malloc(num_devices*sizeof(double));
    }

    printf("%s\n","           ");

    char *temp1, *temp2;
    if (strcmp(activity, "add") == 0) {
        // get water volume at beginning
        initial_vol=0;
        for(i=0; i<numrows; i++){
            for(j=0; j<numcols; j++){
                if(dem[i][j]>missingvalue){
                    initial_vol+=water[i][j];
                }
            }
        }
        initial_vol=initial_vol*cellarea;

        temp1 = upcase(ScratchFileName);
        // check for existence of scratch file & read if available
	    if ( strcmp(temp1,"NULL")!=0 ){
            if ( file_exist(ScratchFileName)){
                printf("%s\n","           ");
                printf("%30s\n", "Scratch file found");
                read_water_array(ScratchFileName, numrows, numcols);
            }
            else{
                printf("%s\n","           ");
                printf("%30s\n", "No Scratch file found");
                printf("%s\n","           ");
                printf("%30s\n", "New Scratch will be saved");
                printf("%s\n","           ");
                printf("%30s\n", "Now proceeding with Waterfile checking");
		        printf("%s\n", "           ");
                // check for water values
                temp2 = upcase(WaterFileName);
                if (strcmp(temp2,"NULL") != 0){
                    if (file_exist(WaterFileName)){
                        // file exists, so read values
                        printf("%30s\n", "Existing water file found");
                        read_water_array(WaterFileName, numrows, numcols);
			            // get water volume at beginning
			            initial_vol = 0;
			            for (i = 0; i<numrows; i++){
			                for (j = 0; j<numcols; j++){
				                if (dem[i][j]>missingvalue){
				                    initial_vol += water[i][j];
				                }
			                }
			            }
			            initial_vol = initial_vol*cellarea;
                    }
                    else
                    {
                        // no water file
                        printf("%30s\n", "Water file missing, will be created");
                        for(i=0; i<numrows; i++){
                            for(j=0; j<numcols; j++){
                                if(dem[i][j]>missingvalue){
                                    water[i][j]=0;
                                }
                            }
                        }
                    }
                }
		        else{
                    printf("%30s\n", "Water file will be created");
                    for(i=0; i<numrows; i++){
                        for(j=0; j<numcols; j++){
                            if(dem[i][j]>missingvalue){
                                water[i][j]=0;
                            }
                        }
                    }
                }
                free(temp2);
                // add water to array
                for(i=0; i<numrows; i++){
                    for(j=0; j<numcols; j++){
                        if(dem[i][j]>missingvalue && water[i][j]>0){
                            water[i][j]+=addwater;
                        }
                    }
                }
                for(i=0; i<numrows; i++){
                    for(j=0; j<numcols; j++){
                        if(dem[i][j]>missingvalue && water[i][j]<=0){
                            water[i][j]=addwater*rof;
                        }
                    }
                }
            }
        }
	    else{
            // check for water values
            temp2 = upcase(WaterFileName);
            if (strcmp(temp2,"NULL") != 0){
                if (file_exist(WaterFileName)){
                    // file exists, so read values
                    printf("%30s\n", "Existing water file found");
                    read_water_array(WaterFileName, numrows, numcols);
                }
                else{
                    // no water file
                    printf("%30s\n", "Water file missing, will be created");
                    for(i=0; i<numrows; i++){
                        for(j=0; j<numcols; j++){
                            if(dem[i][j]>missingvalue){
                                water[i][j]=0;
                            }
                        }
                     }
                }
	        }
            else{
                printf("%30s\n", "Water file will be created");
                for(i=0; i<numrows; i++){
                    for(j=0; j<numcols; j++){
                        if(dem[i][j]>missingvalue){
                            water[i][j]=0;
                        }
                    }
                }
            }
            free(temp2);
            // add water to array
            for(i=0; i<numrows; i++){
                for(j=0; j<numcols; j++){
                    if(dem[i][j]>missingvalue && water[i][j]>0){
                        water[i][j]+=addwater;
                    }
                }
            }
            for(i=0; i<numrows; i++){
                for(j=0; j<numcols; j++){
                    if(dem[i][j]>missingvalue && water[i][j]<=0){
                        water[i][j]=addwater*rof;
                    }
                }
            }
        }
        free(temp1);
        // intialize big arrays
        for(i=0; i<numrows+2; i++){
            for(j=0; j<numcols+2; j++){
                bigdem[i][j]=missingvalue;
                bigwater[i][j]=0;
            }
        }

        for(i=1; i<numrows+1; i++){
            for(j=1; j<numcols+1; j++){
                bigdem[i][j]=dem[i-1][j-1];
                bigwater[i][j]=water[i-1][j-1];
            }
        }

        print_iteration_summary_headings();
    }
    else if (strcmp(activity, "subtract") == 0) {
        // get water volume at beginning
        initial_vol = 0;
        for(i = 0; i<numrows; i++){
            for (j = 0; j<numcols; j++){
                if(dem[i][j]>0){
                    initial_vol += water[i][j];
                }
            }
        }
        initial_vol = initial_vol*cellarea;
        temp1 = upcase(ScratchFileName);
        // check for existence of scratch file & read if available
        if ( strcmp(temp1,"NULL")!=0 ){
            if ( file_exist(ScratchFileName)){
                printf("%s\n","           ");
                printf("%30s\n", "Scratch file found");
                read_water_array(ScratchFileName, numrows, numcols);
            }
            else
            {
                printf("%s\n","           ");
                printf("%30s\n", "No Scratch file found");
                printf("%s\n","           ");
                printf("%30s\n", "New Scratch will be saved.");
                printf("%s\n","           ");
                printf("%30s\n", "Now proceeding with Waterfile checking");
		        printf("%s\n", "           ");
                temp2 = upcase(WaterFileName);
                // check for water values
                if (strcmp(temp2,"NULL") != 0){
                    if ( file_exist(WaterFileName)){
                        // file exists, so read values
                        printf("%30s\n", "Existing water file found");
                        read_water_array(WaterFileName, numrows, numcols);
			            // get water volume at beginning
			            initial_vol = 0;
			            for (i = 0; i<numrows; i++){
			                for (j = 0; j<numcols; j++){
				                if (dem[i][j]>0){
				                    initial_vol += water[i][j];
				                }
			                }
			            }
			            initial_vol = initial_vol*cellarea;
                    }
                    else{
                        // no water file
                        printf("%30s\n", "Water file missing, will be created");
                        for(i=0; i<numrows; i++){
                            for(j=0; j<numcols; j++){
                                water[i][j]=0;
                            }
                        }
                    }
                }
                else{
                    printf("%30s\n", "Water file will be created");
                    for(i=0; i<numrows; i++){
                        for(j=0; j<numcols; j++){
                            water[i][j]=0;
                        }
                    }
                }
                free(temp2);
                // add water to array
                for(i=0; i<numrows; i++){
                    for(j=0; j<numcols; j++){
                        if(dem[i][j]>missingvalue){
                            water[i][j]=max(water[i][j]-subtractwater,0);
                        }
                    }
                }
            }
	    }
        else{
            temp2 = upcase(WaterFileName);
            // check for water values
            if (strcmp(temp2,"NULL") != 0){
                if ( file_exist(WaterFileName)){
                    // file exists, so read values
                    printf("%30s\n", "Existing water file found");
                    read_water_array(WaterFileName, numrows, numcols);
                }
                else{
                    // no water file
                    printf("%30s\n", "Water file missing, will be created");
                    for(i=0; i<numrows; i++){
                        for(j=0; j<numcols; j++){
                            water[i][j]=0;
                        }
                    }
                }
            }
            else{
                printf("%30s\n", "Water file will be created");
                for(i=0; i<numrows; i++){
                    for(j=0; j<numcols; j++){
                        water[i][j]=0;
                    }
                }
            }
            free(temp2);
            // add water to array
            for(i=0; i<numrows; i++){
                for(j=0; j<numcols; j++){
                    if(dem[i][j]>missingvalue){
                        water[i][j]=max(water[i][j]-subtractwater,0);
                    }
                }
           }
	    }
        free(temp1);
        // intialize big arrays
        for(i=0; i<numrows+2; i++){
	        for(j=0; j<numcols+2; j++){
		        bigdem[i][j]=missingvalue;
		        bigwater[i][j]=0;
	        }
        }

        for(i=1; i<numrows+1; i++){
	        for(j=1; j<numcols+1; j++){
		        bigdem[i][j]=dem[i-1][j-1];
		        bigwater[i][j]=water[i-1][j-1];
	        }
        }
        print_iteration_summary_headings();
    }
    else  if (strcmp(activity, "drain") == 0) {
        temp1 = upcase(ScratchFileName);
        // check for existence of scratch file & read if available
        if ( strcmp(temp1,"NULL")!=0 ){
            if ( file_exist(ScratchFileName)){
                printf("%s\n","           ");
                printf("%30s\n", "Scratch file found");
                read_water_array(ScratchFileName, numrows, numcols);
            }
            else{
                printf("%s\n","           ");
                printf("%30s\n", "No Scratch file found");
                printf("%s\n","           ");
                printf("%30s\n", "New Scratch will be saved");
                printf("%s\n","           ");
                printf("%30s\n", "Now proceeding with Waterfile checking");
		        printf("%s\n", "           ");
                if (file_exist(WaterFileName)){
                    // file exists, so read values
                    printf("%30s\n", "Existing water file found");
                    read_water_array(WaterFileName, numrows, numcols);
                }
                else{
                    // no water file
                    printf("%30s\n", "Error water file missing");
                    exit(42);
                }
            }
        }
        else{
            if (file_exist(WaterFileName)){
               // file exists, so read values
               printf("%30s\n", "Existing water file found");
               read_water_array(WaterFileName, numrows, numcols);
            }
            else{
                // no water file
                printf("%30s\n", "Error water file missing");
                exit(42);
            }
        }
	    free(temp1);
        // intialize big arrays
        for(i=0; i<numrows+2; i++){
            for(j=0; j<numcols+2; j++){
                bigdem[i][j]=missingvalue;
                bigwater[i][j]=0;
            }
        }
        for(i=1; i<numrows+1; i++){
            for(j=1; j<numcols+1; j++){
                bigdem[i][j]=dem[i-1][j-1];
                bigwater[i][j]=water[i-1][j-1];
            }
        }

        //find minimum depth
        mindrain=100000000;
        for(i=0; i<numrows+2; i++){
            for(j=0; j<numcols+2; j++){
                if(bigdem[i][j]>0){
                    if(bigdem[i][j]<mindrain){
                        mindrain=bigdem[i][j];
                        drainrow = i;
                        draincol = j;
                    }
                }
            }
        }
	 
	    //modify the value of draincol
	    for(int i=0; i<4; i++){
	 	    draincols[i] = numcols;
	    }
	 
	    if(num_devices > 0)
	    {
	        for(int i=0; i<num_devices; i++){
	    	    if( (draincol>(offsets[i]+i)) && (draincol<offsets[i]+i+iteration_cols[i]) ){
			        draincols[i] = draincol - offsets[i]-i;
		        }
	        }
	    }

        // get water volume at beginning
        initial_vol=0;
        for(i=0; i<numrows; i++){
            for(j=0; j<numcols; j++){
                if(dem[i][j]>missingvalue){
                    initial_vol+=water[i][j];
                }
            }
        }
        initial_vol=initial_vol*cellarea;
        totaldrain=max(bigwater[drainrow][draincol],0);
        basin_area = basincount*cellarea;

        print_basin_summary(basin_area, initial_vol, drainrow, draincol, bigdem[drainrow][draincol]);
        print_iteration_summary_headings();
    }

    // now do water runoff until finished
    done = false;
    k = 0;
    int oi, oj;
    // do a set of iterations and then test for convergence
    gettimeofday(&starttime, NULL);
    while (done == false){
	    for(i=0; i<(numrows+2); i++){
	        for(j=0; j<(numcols+2); j++){
		        if(bigwater[i][j] < thres){
		            bigwater[i][j] = 0;
		        }
	        }
	    }
	    if (strcmp(activity, "drain") == 0) {
	        olddrain = totaldrain;
	    }
	    for(i=0; i<numrows+2; i++){
	        for(j=0; j<numcols+2; j++){
		        oldwater[i][j] = bigwater[i][j];
	        }
	    }
	    if(cpu == 0){
	        if (strcmp(activity, "drain") == 0) {
		        for (i=0; i<IterationNum; i++){
		            for (oi=1; oi<offset; oi++){
			            for (oj=1; oj<offset; oj++){
			                for(row=oi; row<=numrows; row+=(offset-1)){
				                for(col=oj; col<=numcols; col+=(offset-1)){
				                    if (bigwater[row][col] > 0.0 && (bigdem[row][col] > missingvalue)
								        && (row != drainrow || col !=draincol )){
					                    runoffd(row, col, drainrow, draincol, missingvalue);
				                    }
				                }
			                }
			            }
		            }
		            totaldrain =  totaldrain + drain(drainrow,draincol);
		        }// 1000 iterations
	        }
	        else if (strcmp(activity, "add") == 0){
		        for (i=0; i<IterationNum; i++){
		            for (oi=1; oi<offset; oi++){
			            for (oj=1; oj<offset; oj++){
			                for(row=oi; row<=numrows; row+=(offset-1)){
				                for(col=oj; col<=numcols; col+=(offset-1)){
				                    if (bigwater[row][col] > 0.0 && (bigdem[row][col] > missingvalue)){
					                    runoffs(row, col, missingvalue);
				                    }
				                }
			                }
			            }
		            }
		        }// 1000 iterations
	        }
	        else if (strcmp(activity, "subtract") == 0){
		        for (i=0; i<IterationNum; i++){
		            for (oi=1; oi<offset; oi++){
			            for (oj=1; oj<offset; oj++){
			                for(row=oi; row<=numrows; row+=(offset-1)){
				                for(col=oj; col<=numcols; col+=(offset-1)){
				                    if (bigwater[row][col] > 0.0 && (bigdem[row][col] > missingvalue)){
					                    runoffs(row, col, missingvalue);
				                    }
				                }
			                }
			            }
		            }
		        }// 1000 iterations
	        }
	        k = k + IterationNum;
	    }
	    else if(cpu == 1){
	        if(num_devices > 1){
	            // Flattened the Matrix to 1D array
	            for(i=0; i<numrows+2; i++){
		            for(j=0; j<numcols+2; j++){
		                bbigdem[i+j*(numrows+2)]=bigdem[i][j];
  	                    bbigwater[i+j*(numrows+2)]=bigwater[i][j];
		            }
	            }
                for(i=0; i<num_devices; i++){
		        //changed from numcols+2
                    for(j=0; j<(section_sizes[i]*(numrows+2)); j++){
                        b_dem[i][j] = bbigdem[j+((offsets[i]+i)*(numrows+2))];
                        b_water[i][j] = bbigwater[j+((offsets[i]+i)*(numrows+2))];
                    }
       	        }

	            if(strcmp(activity, "drain") == 0) {
	                for (i=0; i<num_devices; i++){
	                    btotaldrain[i]=totaldrain / num_devices;
	                }
	            }
	   
                for(i=0;i<num_devices;i++){
                    d_water[i] = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR, section_sizes[i]*(numrows+2)*sizeof(double), b_water[i], &err);
                    exitOnFail(err, "Create subwater buffer");
                    d_dem[i] = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR, section_sizes[i]*(numrows+2)*sizeof(double), b_dem[i], &err);
                    exitOnFail(err, "Create subdem buffer");
                    err = clEnqueueWriteBuffer(queues[i], d_water[i], CL_FALSE, 0, section_sizes[i] * (numrows+2) * sizeof(double), b_water[i], 0, NULL, &event);
                    exitOnFail(err, "Fill subwater buffer");
                    err = clWaitForEvents(1, &event);
                    exitOnFail(err, "Wait for subwater buffer");
                    err = clEnqueueWriteBuffer(queues[i], d_dem[i], CL_FALSE, 0, section_sizes[i] * (numrows+2) * sizeof(double), b_dem[i], 0, NULL, &event);
                    exitOnFail(err, "Fill subdem buffer");
                    err = clWaitForEvents(1, &event);
                    exitOnFail(err, "Wait for subdem buffer");
                    //set kernel argument
	                err = clSetKernelArg(kernels[i], 0, sizeof(cl_mem), &d_water[i]);
	                exitOnFail(err, "set kernel argument bigwater");
	                err = clSetKernelArg(kernels[i], 1, sizeof(cl_mem), &d_dem[i]);
	                exitOnFail(err, "set kernel argument bigdem");
	                err = clSetKernelArg(kernels[i], 2, sizeof(double), (void *)&missingvalue);
	                exitOnFail(err, "set kernel argument missingvalue");
	                err = clSetKernelArg(kernels[i], 3, sizeof(int), (void *)&numrows);
	                exitOnFail(err, "set kernel argument numrows");
	                err = clSetKernelArg(kernels[i], 4, sizeof(int), (void *)&iteration_cols[i]);
	                exitOnFail(err, "set kernel argument numcols");
	                err = clSetKernelArg(kernels[i], 5, sizeof(int), (void *)&offset);
	                exitOnFail(err, "set kernel argument offset");
	                if (strcmp(activity, "drain") == 0) {
		                d_totaldrain[i] = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR,sizeof(double),(void *) &btotaldrain[i],&err);
		                exitOnFail(err, "create buffer for totaldrain");

		                err = clEnqueueWriteBuffer(queues[i],d_totaldrain[i],CL_FALSE,0,sizeof(double),(void *) &btotaldrain[i],0, NULL, &event);
		                exitOnFail(err, "write totaldrain to device");
		                err = clWaitForEvents(1, &event);
		                exitOnFail(err, "wait for totaldrain X to device");
		                clReleaseEvent(event);

		                err = clSetKernelArg(kernels[i], 8, sizeof(cl_mem), &d_totaldrain[i]);
		                exitOnFail(err, "set kernel argument totaldrain");
		                err = clSetKernelArg(kernels[i], 9, sizeof(int), (void *)&drainrow);
		                exitOnFail(err, "set kernel argument drainrow");
		                err = clSetKernelArg(kernels[i], 10, sizeof(int), (void *)&draincols[i]);
	  	                exitOnFail(err, "set kernel argument draincol");
	                }
	            }

	            for (i=0; i<IterationNum; i++){
	                //Declare these here since they don't change and to avoid recalculating for the same results
	                int tmp1 = (int)((numrows+2)/(offset-1));
	                int tmp2 = (int)(tmp1/32);

	                //write the boundary back to the device
	                if(num_devices > 1){
	                    if(i>0){
		                    for(j=0; j<(num_devices-1); j++){ 
		                        err = clEnqueueWriteBuffer(queues[j], d_water[j], CL_FALSE, (section_sizes[j]-2)*(numrows+2)*sizeof(double), (numrows+2)*2*sizeof(double), b_dem[j], 0, NULL, &event);
		                        exitOnFail(err, "load updated subwater buffer");
		                        err = clWaitForEvents(1, &event);
		                        exitOnFail(err, "wait for updated subwater buffer to load");
		                        clReleaseEvent(event);
		                    }
	                        for(j=1; j<num_devices; j++){
		                        err = clEnqueueWriteBuffer(queues[j], d_water[j], CL_FALSE, 0, (numrows+2)*2*sizeof(double), b_water[j], 0, NULL, &event);
		                        exitOnFail(err, "load updated subwater buffer");
		                        err = clWaitForEvents(1, &event);
		                        exitOnFail(err, "wait for updated subwater buffer to load");
		                        clReleaseEvent(event);
		                    }
	                    }
	                }
	  
	                //kernel executing
	                for(oj=1; oj<(offset-1); oj++){
	  	                for(oi=1; oi<offset; oi++){
		                    for(j=0; j<num_devices; j++){
			                    size_t global[2];
			                    int tmp1a = (int)((iteration_cols[j]+2)/(offset-1));
			                    int tmp2a = (int)(tmp1a/32);
			                    global[0] = (tmp2+1)*32;
			                    global[1] = (tmp2a+1)*32;
			                    err = clSetKernelArg(kernels[j], 0, sizeof(cl_mem), &d_water[j]);
		                        exitOnFail(err, "update kernel argument bigwater");
		                        err = clSetKernelArg(kernels[j], 6, sizeof(int), (void *)&oi);
		                        exitOnFail(err, "set kernel argument oi");
		                        err = clSetKernelArg(kernels[j], 7, sizeof(int), (void *)&oj);
		                        exitOnFail(err, "set kernel argument oj");
		                        err= clEnqueueNDRangeKernel(queues[j],kernels[j],2, NULL, global, NULL,0,NULL, NULL);
		                        exitOnFail(err, "enqueue kernel");
		                    }
		                }
	                }
      
        	        //update boundary	            
		            //get the boundary data from device
		            for(j=0; j<(num_devices-1); j++){
			            err = clEnqueueReadBuffer(queues[j], d_water[j], CL_FALSE, (section_sizes[j]-2)*(numrows+2)*sizeof(double), 2*(numrows+2)*sizeof(double), b_dem[j], 0, NULL, &event);
			            exitOnFail(err, "update subwater buffer");
			            err = clWaitForEvents(1, &event);
			            exitOnFail(err, "wait for main subwater buffer to update");
			            clReleaseEvent(event);
		            }

		            for(j=1; j<num_devices; j++){
	                    err = clEnqueueReadBuffer(queues[j], d_water[j], CL_FALSE, 0, 2*(numrows+2)*sizeof(double), b_water[j], 0, NULL, &event);
		                exitOnFail(err, "update subwater buffer");
		                err = clWaitForEvents(1, &event);
		                exitOnFail(err, "wait for main subwater buffer to update");
		                clReleaseEvent(event);
		            }
	            
	                oj = 3;
	                for(oi=1; oi<offset; oi++){
		                for(j=0; j<num_devices; j++){
			                size_t global[2];
			                int tmp1a = (int)((iteration_cols[j]+2)/(offset-1));
			                int tmp2a = (int)(tmp1a/32);
			                global[0] = (tmp2+1)*32;
			                global[1] = (tmp2a+1)*32;
			                err = clSetKernelArg(kernels[j], 0, sizeof(cl_mem), &d_water[j]);
			                exitOnFail(err, "update kernel argument bigwater");
			                err = clSetKernelArg(kernels[j], 6, sizeof(int), (void *)&oi);
			                exitOnFail(err, "set kernel argument oi");
			                err = clSetKernelArg(kernels[j], 7, sizeof(int), (void *)&oj);
			                exitOnFail(err, "set kernel argument oj");
			                err= clEnqueueNDRangeKernel(queues[j],kernels[j],2, NULL, global, NULL,0,NULL, NULL);
			                exitOnFail(err, "enqueue kernel");
		                }
	                }

		            //first method:handle the boundary in serial
		            for(int o=0; o<(num_devices-1); o++){
		                for(int m=0; m<2; m++){
		     	            for(int n=0; n<(numrows+2); n++){
				                bigwater[n][offsets[o+1]-1+o+m]=b_dem[o][m*(numrows+2)+n];
				                bigwater[n][offsets[o+1]-1+o+m+2]=b_water[o+1][m*(numrows+2)+n];
			                }
			            }
		            }
		    
		            for(int o=0; o<(num_devices-1); o++){
		                for(int m=1; m<=numrows; m++){
		                    for(int n=(offsets[o+1]+o); n<=(offsets[o+1]+o+1); n++){
			                    if (bigwater[m][n]>0.0 && (bigdem[m][n]>missingvalue)){
			                        runoffa(m, n, missingvalue);
		                        }
		                    }
                        }
		            }
		            for(int o=0; o<(num_devices-1); o++){
		                for(int m=0; m<2; m++){
			                for(int n=0; n<(numrows+2); n++){
			                    b_dem[o][m*(numrows+2)+n]=bigwater[n][offsets[o+1]-1+o+m];
				                b_water[o+1][m*(numrows+2)+n]=bigwater[n][offsets[o+1]-1+o+m+2];
			                }
			            }
		            }
		    
	                if (strcmp(activity, "drain") == 0) {
	                    totaldrain = 0;
		                int count;
		                for (count=0; count<num_devices; count++){
             	            err= clEnqueueReadBuffer(queues[count], d_totaldrain[count], CL_FALSE, 0, sizeof(double), (void *) &btotaldrain[count], 0, NULL, &event);
		                    exitOnFail(err, "read totaldrain from device");
		                    err = clWaitForEvents(1, &event);
		                    exitOnFail(err, "wait for read totaldrain from device");
		                    clReleaseEvent(event);
		                    totaldrain += btotaldrain[count];
		                }
		                totaldrain =  totaldrain + drain(drainrow,draincol);
	                    //printf("%lf \n", totaldrain);
	                }
	            }

	            for (i=0; i<num_devices; i++){
	                err= clEnqueueReadBuffer(queues[i], d_water[i], CL_FALSE, 0, section_sizes[i] * (numrows+2) * sizeof(double), b_water[i], 0, NULL, &event);
	                exitOnFail(err, "read bigwater from device");
	                err = clWaitForEvents(1, &event);
	                exitOnFail(err, "wait for read bigwater from device");
	                clReleaseEvent(event);
	            }

	            for (i=0; i<num_devices; i++) {
	                err = clReleaseMemObject(d_water[i]);
	                err = clReleaseMemObject(d_dem[i]);
	            }
	            if (strcmp(activity, "drain") == 0) {
	                for (i=0; i<num_devices; i++){
	                    err= clReleaseMemObject(d_totaldrain[i]);
	                }
                }
		    
	            // Reshape Flattened Matrix back to 2D array
                for(i=0; i<num_devices; i++){
                    for(j=0; j<(section_sizes[i]*(numrows+2)); j++){
                        bbigwater[j+((offsets[i]+i)*(numrows+2))] = b_water[i][j];
                    }
                }

	            for(i=0; i<numrows+2; ++i){
	                for(j=0; j<numcols+2; ++j){
		                bigwater[i][j]=bbigwater[i+j*(numrows+2)];
	                }
	            }
	            k = k+IterationNum;
	        }
	
	        else if(num_devices == 1)
  	        {
	            for(i=0; i<numrows+2; i++){
		            for(j=0; j<numcols+2; j++){
		                bbigdem[i+j*(numrows+2)]=bigdem[i][j];
		                bbigwater[i+j*(numrows+2)]=bigwater[i][j];
		            }
	            }   
	            if (strcmp(activity, "drain") == 0){
	                btotaldrain[0]=totaldrain;
	            }    
	            d_bigwater = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR,bytes,bbigwater,&err);
	            exitOnFail(err, "create buffer for bigwater");
	            d_bigdem = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR,bytes,bbigdem,&err);
	            exitOnFail(err, "create buffer for bigdem");

	            err = clEnqueueWriteBuffer(queues[0],d_bigwater,CL_FALSE,0,bytes,bbigwater,0, NULL, &event);
	            exitOnFail(err, "write bigwater to device");
	            err = clWaitForEvents(1, &event);
	            exitOnFail(err, "wait for bigwater X to device");
	            clReleaseEvent(event);	
	    
	            err = clEnqueueWriteBuffer(queues[0],d_bigdem,CL_FALSE,0,bytes,bbigdem,0, NULL, &event);  
	            exitOnFail(err, "write bigdem to device");
	            err = clWaitForEvents(1, &event);
	            exitOnFail(err, "wait for bigdem X to device");
	            clReleaseEvent(event);

	            err = clSetKernelArg(kernels[0], 0, sizeof(cl_mem), &d_bigwater);  
	            exitOnFail(err, "set kernel argument bigwater");
	            err = clSetKernelArg(kernels[0], 1, sizeof(cl_mem), &d_bigdem); 		  
	            exitOnFail(err, "set kernel argument bigdem");
	            err = clSetKernelArg(kernels[0], 2, sizeof(double), (void *)&missingvalue);		  
	            exitOnFail(err, "set kernel argument missingvalue");
	            err = clSetKernelArg(kernels[0], 3, sizeof(int), (void *)&numrows);	 
	            exitOnFail(err, "set kernel argument numrows"); 
	            err = clSetKernelArg(kernels[0], 4, sizeof(int), (void *)&numcols);	 
	            exitOnFail(err, "set kernel argument numcols"); 		    
	            err = clSetKernelArg(kernels[0], 5, sizeof(int), (void *)&offset);	 
	            exitOnFail(err, "set kernel argument offset");
	            if (strcmp(activity, "drain") == 0) {
		            d_totaldrain[0] = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR,sizeof(double),btotaldrain,&err);	
		            exitOnFail(err, "create buffer for totaldrain");
		
		            err = clEnqueueWriteBuffer(queues[0],d_totaldrain[0],CL_FALSE,0,sizeof(double),btotaldrain,0, NULL, &event);
		            exitOnFail(err, "write totaldrain to device");
		            err = clWaitForEvents(1, &event);
		            exitOnFail(err, "wait for totaldrain X to device");
		            clReleaseEvent(event);		
		
		            err = clSetKernelArg(kernels[0], 8, sizeof(cl_mem), &d_totaldrain[0]);
		            exitOnFail(err, "set kernel argument totaldrain");
		            err = clSetKernelArg(kernels[0], 9, sizeof(int), (void *)&drainrow);
		            exitOnFail(err, "set kernel argument drainrow");
		            err = clSetKernelArg(kernels[0], 10, sizeof(int), (void *)&draincol);
		            exitOnFail(err, "set kernel argument draincol");
	            }
	    
	            for (i=0; i<IterationNum; i++){ 
		            for(oi=1; oi<offset; oi++){        
		                for(oj=1; oj<offset; oj++){	    
		                    err = clSetKernelArg(kernels[0], 6, sizeof(int), (void *)&oi);	 
		                    exitOnFail(err, "set kernel argument oi");
		                    err = clSetKernelArg(kernels[0], 7, sizeof(int), (void *)&oj);	 
		                    exitOnFail(err, "set kernel argument oj");		     

		                    size_t global[2];
		                    int tmp1 = (int)((numrows+2)/(offset-1));
		                    int tmp2 = (int)(tmp1/32);
		                    int tmp1a = (int)((numcols+2)/(offset-1));
		                    int tmp2a = (int)(tmp1a/32);		    
		                    global[0] = (tmp2+1)*32;
		                    global[1] = (tmp2a+1)*32;
		                    err= clEnqueueNDRangeKernel(queues[0],kernels[0],2, NULL, global, NULL,0,NULL,&event);
		                    exitOnFail(err, "enqueue kernel");
		                    // wait for kernel, this forces execution
		                    err = clWaitForEvents(1, &event);
		                    exitOnFail(err, "wait for enqueue kernel");
		                    clReleaseEvent(event);
		                }		  
		            }
		            if (strcmp(activity, "drain") == 0) {
		                err= clEnqueueReadBuffer(queues[0], d_totaldrain[0], CL_FALSE, 0, sizeof(double), btotaldrain, 0, NULL, &event); 
		                exitOnFail(err, "read totaldrain from device");
		                err = clWaitForEvents(1, &event);
		                exitOnFail(err, "wait for read totaldrain from device");
		                clReleaseEvent(event);	
		                totaldrain=btotaldrain[0];
		                totaldrain =  totaldrain + drain(drainrow,draincol);
		            }	
	            }
	            err= clEnqueueReadBuffer(queues[0], d_bigwater, CL_FALSE, 0, bytes, bbigwater, 0, NULL, &event);
	            exitOnFail(err, "read bigwater from device");
	            err = clWaitForEvents(1, &event);
	            exitOnFail(err, "wait for read bigwater from device");
	            clReleaseEvent(event);
	    
	            err= clReleaseMemObject(d_bigwater);
	            err= clReleaseMemObject(d_bigdem); 
	            if (strcmp(activity, "drain") == 0) {	
	                err= clReleaseMemObject(d_totaldrain[0]);		  
	            }  	    
	            // Reshape Flattened Matrix back to 2D array
	            for(i=0; i<numrows+2; ++i){
		            for(j=0; j<numcols+2; ++j){
		                bigwater[i][j]=bbigwater[i+j*(numrows+2)];
		            }
	            }   
	            k = k+IterationNum;
	        }
        }
        for(i=0; i<numrows+2; i++){
	        for(j=0; j<numcols+2; j++){
	            diff[i][j] = fabs(bigwater[i][j] - oldwater[i][j]);
	        }
        }

        max_diff=diff[0][0];
        for(i=0; i<numrows+2; i++){
	        for(j=0; j<numcols+2; j++){
	            if(bigdem[i][j]>missingvalue){
		            if(diff[i][j]>max_diff){
		                max_diff = diff[i][j];
		            }
	            }
	        }
        }

        if (strcmp(activity, "drain") == 0) {
	        diffdrain = (fabs(totaldrain - olddrain))*cellarea;
	        final_vol=0;
	        for(i=0; i<numrows+2; i++){
	            for(j=0; j<numcols+2; j++){
		            if(bigdem[i][j]>missingvalue){
		                final_vol+=bigwater[i][j];
		            }
	            }
	        }
	        final_vol=final_vol*cellarea;
        }
        gettimeofday(&currenttime, NULL);
        if (strcmp(activity, "drain") == 0) {
	        printf("%7s %d %7s %8.6f %5s %10.1f %5s %12.1f %5s %8.2f\n", "", k, "", max_diff,"", diffdrain, "", final_vol, "",
                    (double)(currenttime.tv_usec-starttime.tv_usec)/1000000 + (double)(currenttime.tv_sec-starttime.tv_sec));
        }
        else{
	        printf("%7s %d %7s %8.6f %5s %8.2f\n", "", k, "",
	                       max_diff,"", (double)(currenttime.tv_usec-starttime.tv_usec)/1000000 + (double)(currenttime.tv_sec-starttime.tv_sec));
        }

        // check to see if graceful exit triggered - Ryan Spies
        #ifdef _WIN32
        if (_kbhit()){
            exitChar = _getch();
            if (exitChar == 'q'){
                gracefulExit = true;
                printf("Now beginning graceful exit...\n");
            }
        }
        #else
        if (kbhit()){
            exitChar = getch();
            if (exitChar == 'q')
            {
                gracefulExit = true;
                printf("Now beginning graceful exit...\n");
            }
        }
        #endif
	    // check to see if finished
	    if (strcmp(activity, "drain") == 0) {
	        temp1 = upcase(ScratchFileName);
	        if (max_diff <= elevation_tolerance || diffdrain < drain_tolerance || ((k == iteration_limit) && isKillSwitch) || gracefulExit) {
		        done = true;
	        }
	        else if (strcmp(temp1, "NULL")!=0) {
		        // write output to ArcGIS file
		        // convert back to small arrays
		        for(i=0; i<numrows; i++){
		            for(j=0; j<numcols; j++){
			            water[i][j]=bigwater[i+1][j+1];
		            }
		        }
		        write_gis(ScratchFileName, numrows, numcols, dem_header_line, dem_header_value);
	        }
	        free(temp1);
	    }
	    else{
	        temp1 = upcase(ScratchFileName);
	        if (max_diff <= elevation_tolerance || ((k == iteration_limit) && isKillSwitch) || gracefulExit) {
		        done = true;
	        }
	        else if (strcmp(temp1, "NULL")!=0) {
		        // write output to ArcGIS file
		        // convert back to small arrays
		        for(i=0; i<numrows; i++){
		            for(j=0; j<numcols; j++){
			            water[i][j]=bigwater[i+1][j+1];
		            }
		        }

		        if (strcmp(activity, "add") == 0) {
		            for(i=0; i<numrows; i++){
			            for(j=0; j<numcols; j++){
			                if(dem[i][j]<=missingvalue){
				                water[i][j]=missingvalue;
			                }
			            }
		            }
		        }
		        write_gis(ScratchFileName, numrows, numcols, dem_header_line, dem_header_value);
	        }
	        free(temp1);
	    }
    }

    // convert back to small arrays
    for(i=0; i<numrows; i++){
	    for(j=0; j<numcols; j++){
	        water[i][j]=bigwater[i+1][j+1];
	    }
    }

    for(i=0; i<numrows; i++){
	    for(j=0; j<numcols; j++){
	        if(dem[i][j]<=missingvalue){
		        water[i][j]=missingvalue;
	        }
	    }
    }

    // get watercovered elements & mean depth
    watercount=0;
    for(i=0; i<numrows; i++){
	    for(j=0; j<numcols; j++){
	        if(water[i][j]>0.001 && dem[i][j]> missingvalue){
		        watercount++;
	        }
	    }
    }

    watertotal=0;
    for(i=0; i<numrows; i++){
	    for(j=0; j<numcols; j++){
	        if(dem[i][j]> missingvalue){
		        watertotal+=water[i][j];
	        }
	    }
    }

    final_vol=0;
    for(i=0; i<numrows; i++){
	    for(j=0; j<numcols; j++){
	        if(dem[i][j]>missingvalue){
		        final_vol+=water[i][j];
	        }
	    }
    }
    final_vol=final_vol*cellarea;
    meanwater = watertotal/((float)watercount);
    basincount=0;
    for(i=0; i<numrows; i++){
	    for(j=0; j<numcols; j++){
	        if(dem[i][j]>missingvalue){
		        basincount++;
	        }
	    }
    }

    waterfrac = (float)watercount/(float)basincount;

    if (strcmp(activity, "drain") == 0) {
	    drainvol = totaldrain*cellarea;
	    draindepth = (drainvol/((float)basincount*cellarea))*1000;
    }

    maxdepth=water[0][0];
    for(i=0; i<numrows; i++){
	    for(j=0; j<numcols; j++){
	        if(water[i][j]>maxdepth){
		        maxdepth = water[i][j];
	        }
	    }
    }
    maxdepth = maxdepth*1000;

    if (strcmp(activity, "drain") == 0) {
        print_results(initial_vol, final_vol, drainvol, waterfrac, meanwater, draindepth, maxdepth, iteration_limit);
    }
    else{
        print_results(initial_vol, final_vol, 0, waterfrac, meanwater, 0, maxdepth, iteration_limit);
    }

    //write output to ArcGIS file
    write_gis(OutputFileName, numrows, numcols, dem_header_line, dem_header_value);

    // print execution time
    gettimeofday(&currenttime, NULL);
    printf("%20s %10.2f %s\n","Run Time", (double)(currenttime.tv_usec-starttime.tv_usec)/1000000 + (double)(currenttime.tv_sec-starttime.tv_sec), "s");
    if(cpu == 1){
	    for(i=0;i<num_devices;i++){
	        err= clReleaseKernel(kernels[i]);
	        err= clReleaseCommandQueue(queues[i]);
	        if(num_devices > 1){
                free(b_water[i]);
                free(b_dem[i]);
            }
	    }
	    err= clReleaseProgram(program);
	    err= clReleaseContext(context);
	    free(bbigdem);
	    free(bbigwater);
    }
    for (i=0; i<7; i++){
	    free(dem_header_line[i]);
    }
    free(dem_header_line);
    for (i=0; i<numrows; i++){
        free(dem[i]);
        free(water[i]);
    }
    for (i=0; i<numrows+2; i++){
        free(oldwater[i]);
        free(diff[i]);
        free(bigdem[i]);
        free(bigwater[i]);
    }
    free(dem);
    free(water);
    free(oldwater);
    free(diff);
    free(bigdem);
    free(bigwater);
    return 0;
}

int file_exist(char *filename){
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}


char *upcase(char *string){
    int i = 0;
    int len = 0;
    len = strlen(string);
    char* USTR = malloc((len+1)*sizeof(char));
    for (i=0; i<=strlen(string); i++){
        if (string[i]>=97 && string[i]<=122){
            USTR[i]=string[i]-32;
        }
        else{
            USTR[i]=string[i];
        }
    }
    return USTR;
}



void write_gis(char filename[80], int numrows, int numcols,
                      char* header_line[8], double header_value[8]){
   //write output to ArcGIS file
    FILE *fgis;
    fgis=fopen(filename,"w");
    fprintf(fgis, "%s %d\n", header_line[1], (int)header_value[1]);
    fprintf(fgis, "%s %d\n", header_line[2], (int)header_value[2]);
    fprintf(fgis, "%s %14.6f\n", header_line[3], header_value[3]);
    fprintf(fgis, "%s %14.6f\n", header_line[4], header_value[4]);
    fprintf(fgis, "%s %9.6f\n", header_line[5], header_value[5]);
    fprintf(fgis, "%s %14.6f\n", header_line[6], header_value[6]);
    int row=0;
    int col=0;
    for (row=0; row < numrows; row++){
        for (col=0; col < numcols; col++){
            fprintf(fgis, "%f ", water[row][col]);
        }
        fprintf(fgis, "\n");
    }
    fclose(fgis);
}

void read_water_array(char filename[80], int numrows, int numcols) {
    // reads data from ArcGIS file
    // 1st read and discard header
    FILE *fwarray;
    fwarray=fopen(filename,"r");
    int row;
    int col;
    fscanf(fwarray,"%s %lf", dem_header_name, &dem_header_value[0]);
    fscanf(fwarray,"%s %lf", dem_header_name, &dem_header_value[0]);
    fscanf(fwarray,"%s %lf", dem_header_name, &dem_header_value[0]);
    fscanf(fwarray,"%s %lf", dem_header_name, &dem_header_value[0]);
    fscanf(fwarray,"%s %lf", dem_header_name, &dem_header_value[0]);
    fscanf(fwarray,"%s %lf", dem_header_name, &dem_header_value[0]);
    for (row=0; row < numrows; row++){
        for (col=0; col < numcols; col++){
            fscanf(fwarray,"%lf",&water[row][col]);
        }
     }
    fclose(fwarray);
}


void read_dem_array(char filename[80], int numrows, int numcols) {
    // reads data from ArcGIS file
    // 1st read and discard header
    FILE *fdarray;
    fdarray=fopen(filename,"r");
    int row;
    int col;
    fscanf(fdarray,"%s %lf", dem_header_name, &dem_header_value[0]);
    fscanf(fdarray,"%s %lf", dem_header_name, &dem_header_value[0]);
    fscanf(fdarray,"%s %lf", dem_header_name, &dem_header_value[0]);
    fscanf(fdarray,"%s %lf", dem_header_name, &dem_header_value[0]);
    fscanf(fdarray,"%s %lf", dem_header_name, &dem_header_value[0]);
    fscanf(fdarray,"%s %lf", dem_header_name, &dem_header_value[0]);
    for (row=0; row < numrows; row++){
        for (col=0; col < numcols; col++){
            fscanf(fdarray,"%lf",&dem[row][col]);
        }
    }
    fclose(fdarray);
}


void read_gis_header(char filename[80], bool print_output, char* header_line[8], double header_value[8]){

    if (print_output){
        printf("%s\n", "                  ");
        printf("%30s\n", "ArcGIS file header");
        printf("%30s %d\n", header_line[1], (int)header_value[1]);
        printf("%30s %d\n", header_line[2], (int)header_value[2]);
        printf("%30s %9.1f\n", header_line[3], header_value[3]);
        printf("%30s %9.1f\n", header_line[4], header_value[4]);
        printf("%30s %9.1f\n", header_line[5], header_value[5]);
        printf("%30s %9.1f\n", header_line[6], header_value[6]);
    }
}


void print_copyright(char *activity){
    printf("%s\n", "                                                                   ");
    printf("%s\n", "                                                                   ");
    if (strcmp(activity, "add") == 0) {
      printf("%s\n", "WDPM_add 1.0 - Wetland DEM Ponding Model - parallel version");
    }
    else if (strcmp(activity, "subtract") == 0) {
      printf("%s\n", "WDPM_subtract 1.0 - Wetland DEM Ponding Model - parallel version");
    }
    else  if (strcmp(activity, "drain") == 0) {
      printf("%s\n", "WDPM_drain 1.0 - Wetland DEM Ponding Model - parallel version");
    }
    printf("%s\n", "Copyright (C) 2010,2012 Kevin Shook, Centre for Hydrology");
    printf("%s\n", "--------------------------------------------------------------------");
    printf("%s\n", "                                                                   ");
    printf("%s\n", "This program is free software: you can redistribute it and/or modify");
    printf("%s\n", "it under the terms of the GNU General Public License as published by");
    printf("%s\n", "the Free Software Foundation, either version 3 of the License, or");
    printf("%s\n", "(at your option) any later version.");
    printf("%s\n", "                                                                   ");
    printf("%s\n", "This program is distributed in the hope that it will be useful,");
    printf("%s\n", "but WITHOUT ANY WARRANTY; without even the implied warranty of");
    printf("%s\n", "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the");
    printf("%s\n", "GNU General Public License for more details.");
    printf("%s\n", "                                                                   ");
    printf("%s\n", "You should have received a copy of the GNU General Public License");
    printf("%s\n", "along with this program.  If not, see <http://www.gnu.org/licenses/>.");
    printf("%s\n", "                                                                   ");

    if (strcmp(activity, "add") == 0) {
      printf("%s\n", "This program adds water to an ArcGIS ASCII file of water runoff");
      printf("%s\n", "and redistributes water over the DEM");
    }
    else if (strcmp(activity, "subtract") == 0) {
      printf("%s\n", "This program removes a depth water to an ArcGIS ASCII file of water depths");
      printf("%s\n", "and redistributes water over the DEM");
    }
    else  if (strcmp(activity, "drain") == 0) {
      printf("%s\n", "This program drains an ArcGIS ASCII file of water runoff");
      printf("%s\n", "from the lowest point in the DEM, which acts as a drain");
    }
    printf("%s\n", "From the algorithm of Shapiro, M., & Westervelt, J. (1992). ");
    printf("%s\n", "An Algebra for GIS and Image Processing (pp. 1-22).");
    printf("%s\n", "                                                                   ");
    printf("%s\n", "                                                                   ");
}


void print_arg_list(){
    printf("%s\n", "                                          ");
    printf("%s\n", "Program arguments in order of specification");
    if (strcmp(activity,"add")==0){
      printf("%s\n", "Add module specified");
    }
    else if (strcmp(activity,"subtract")==0){
      printf("%s\n", "Subtract module specified");
    }
    else if (strcmp(activity,"drain")==0){
      printf("%s\n", "Drain module specified");
    }
    printf("%s\n", "Path and Name of Report file");
    printf("%s\n", "DEM file name (string)");
    printf("%s\n", "Water file name (string)");
    printf("%s\n", "Output file name (string)");
    printf("%s\n", "Scratch file name (string) - Optional, use --NULL-- to omit");
    if (strcmp(activity,"add")==0){
      printf("%s\n", "Depth of water to add (mm) (real)");
      printf("%s\n", "Water runoff fraction (real)");
      printf("%s\n", "Elevation tolerance (mm) (real)");
    }
    else if (strcmp(activity,"subtract")==0){
      printf("%s\n", "Depth of water to remove (mm) (real)");
      printf("%s\n", "Elevation tolerance (mm) (real)");
    }
    else if (strcmp(activity,"drain")==0){
      printf("%s\n", "Elevation tolerance (mm) (real)");
      printf("%s\n", "Drain tolerance (m3) (real)");
    }
    printf("%s\n", "Specify 0 for serial CPU and 1 for opencl ");
    printf("%s\n", "Specify 0 for OpenCL CPU and 1 for opencl GPU ");
    printf("%s\n", "Zero depth threshold (mm) (real)");
    printf("%s\n", "Iteration limit (multiple of 1000)");
    printf("%s\n", "                                          ");
}

void print_narg_list(){
    printf("%s\n", "Module name: add");
    printf("%s\n", "Path and Name of Report file");
    printf("%s\n", "DEM file name (string)");
    printf("%s\n", "Water file name (string)");
    printf("%s\n", "Output file name (string)");
    printf("%s\n", "Scratch file name (string) - Optional, use --NULL-- to omit");
    printf("%s\n", "Depth of water to add (mm) (real)");
    printf("%s\n", "Water runoff fraction (real)");
    printf("%s\n", "Elevation tolerance (mm) (real)");
    printf("%s\n", "Specify 0 for serial CPU and 1 for opencl ");
    printf("%s\n", "Specify 0 for OpenCL CPU and 1 for opencl GPU ");
    printf("%s\n", "Zero depth threshold (mm) (real) ");
    printf("%s\n", "Iteration limit (multiple of 1000)");
    printf("%s\n", "                                          ");
    printf("%s\n", "                                          ");
    printf("%s\n", "Module name: subtract");
    printf("%s\n", "Path and Name of Report file");
    printf("%s\n", "DEM file name (string)");
    printf("%s\n", "Water file name (string)");
    printf("%s\n", "Output file name (string)");
    printf("%s\n", "Scratch file name (string) - Optional, use --NULL-- to omit");
    printf("%s\n", "Depth of water to remove (mm) (real)");
    printf("%s\n", "Elevation tolerance (mm) (real)");
    printf("%s\n", "Specify 0 for serial CPU and 1 for opencl ");
    printf("%s\n", "Specify 0 for OpenCL CPU and 1 for opencl GPU ");
    printf("%s\n", "Zero depth threshold (mm) (real) ");
    printf("%s\n", "Iteration limit (multiple of 1000)");
    printf("%s\n", "                                          ");
    printf("%s\n", "                                          ");
    printf("%s\n", "Module name: drain");
    printf("%s\n", "Path and Name of Report file");
    printf("%s\n", "DEM file name (string)");
    printf("%s\n", "Water file name (string)");
    printf("%s\n", "Output file name (string)");
    printf("%s\n", "Scratch file name (string) - Optional, use --NULL-- to omit");
    printf("%s\n", "Elevation tolerance (mm) (real)");
    printf("%s\n", "Drain tolerance (m3) (real)");
    printf("%s\n", "Specify 0 for serial CPU and 1 for opencl ");
    printf("%s\n", "Specify 0 for OpenCL CPU and 1 for opencl GPU ");
    printf("%s\n", "Zero depth threshold (mm) (real) ");
    printf("%s\n", "Iteration limit (multiple of 1000)");
    printf("%s\n", "                                          ");
}


void print_args (char demfile[80], char waterfile[80], char outputfile[80],
                      char scratchfile[80], double addwater, double subtractwater, double rof,
		             double eltol, double draintol, char *activity, int cpu, double thres, int iterlim){
    printf("%30s\n", "WDPM Parameters");
    printf("%30s %s\n", "Function used:", activity);
    printf("%30s %s\n", "DEM file:", demfile);
    printf("%30s %s\n", "Water file:", waterfile);
    printf("%30s %s\n", "Output file:", outputfile);
    printf("%30s %s\n", "Scratch file:", scratchfile);
    if (strcmp(activity,"add")==0){
      printf("%30s %0.1f %s\n", "Water added:", addwater, "mm");
      printf("%30s %0.1f\n", "Runoff fraction:", rof);
      printf("%30s %0.1f %s\n", "Elevation tolerance:", eltol, "mm");
      printf("%30s %0.4f %s\n", "Zero depth threshold:", thres, "mm");
    }
    if (strcmp(activity,"subtract")==0){
      printf("%30s %0.1f %s\n", "Water subtracted:", subtractwater, "mm");
      printf("%30s %0.1f %s\n", "Elevation tolerance:", eltol, "mm");
      printf("%30s %0.4f %s\n", "Zero depth threshold:", thres, "mm");
    }
    if (strcmp(activity,"drain")==0){
      printf("%30s %0.1f %s\n", "Elevation tolerance:", eltol, "mm");
      printf("%30s %0.1f %s\n", "Drain tolerance:", draintol, "m3");
      printf("%30s %0.4f %s\n", "Zero depth threshold:", thres, "mm");
    }
    if (cpu==0){
      printf("%s\n", "               ");
      printf("%41s\n", "Using Serial CPU for Computation");
    }
    else
    {
      printf("%s\n", "               ");
      printf("%41s\n", "Using Parallel OpenCL for Computation");
    }
    if (iterlim == 0){
      printf("%41s\n", "No iteration limit specified");
    }
    else
    {
      printf("%30s %i\n", "Iteration limit:", iterlim);
    }
}

void print_iteration_summary_headings(){
    if (strcmp(activity, "add") == 0) {
	printf("%s\n", "               ");
	printf("%30s\n", "Doing calculations");
	printf("%15s %15s %15s\n", "iterations", "max diff", "run time");
	printf("%13s %14s %15s\n", " ", "(m)", "(s)");
    }
    else if (strcmp(activity, "subtract") == 0) {
	printf("%s\n", "               ");
	printf("%30s\n", "Doing calculations");
	printf("%15s %15s %15s\n", "iterations", "max diff", "run time");
	printf("%13s %14s %15s\n", " ", "(m)", "(s)");
    }
    else  if (strcmp(activity, "drain") == 0) {
	printf("%s\n", "               ");
	printf("%30s\n", "Doing calculations");
	printf("%15s %15s %15s %15s %15s\n", "iterations", "max diff", "vol change", "water left", "run time");
	printf("%13s %14s %15s %16s %17s\n", " ", "(m)", "(m3)", "(m3)", "(s)");
    }
}

void print_basin_summary(double basin_area, double initial_vol, int drainrow, int draincol, double minel){
	printf("%s\n", "               ");
	printf("%30s\n", "Basin summary");
	printf("%20s %10.1f %s\n", "Basin area:", basin_area, "m2");
	printf("%20s %10.1f %s\n", "Initial volume:", initial_vol, "m3");
	printf("%20s %d\n", "Drain column:", draincol);
	printf("%20s %d\n", "Drain row:", drainrow);
	printf("%20s %10.3f %s\n", "Min DEM elevation:", minel, "m");
}


void print_results(double initial_vol, double final_vol, double drainvol, double waterfrac,
                       double meanwater, double draindepth, double maxdepth, int iterlim){
    if (strcmp(activity, "drain") == 0) {
	    printf("%s\n", "                     ");
	    printf("%30s\n", "WDPM run summary");
	    printf("%20s %10.1f %s\n", "Initial volume",initial_vol,"m3");
	    printf("%20s %10.1f %s\n", "Final volume",final_vol,"m3");
	    printf("%20s %10.1f %s\n", "Volume change",initial_vol-final_vol,"m3");
	    printf("%20s %10.1f %s\n", "Volume drained",drainvol,"m3");
	    printf("%20s %10.3f %s\n", "Final water coverage", waterfrac,"");
	    printf("%20s %10.1f %s\n", "Mean water depth", meanwater*1000.,"mm");
	    printf("%20s %10.1f %s\n", "Depth drained", draindepth,"mm ");
	    printf("%20s %10.1f %s\n", "Max water depth", maxdepth,"mm ");
        printf("%20s %d\n", "Iteration limit", iterlim);
    }
    else{
	    printf("%s\n", "                     ");
	    printf("%30s\n", "WDPM run summary");
	    printf("%20s %10.1f %s\n", "Initial volume",initial_vol,"m3");
	    printf("%20s %10.1f %s\n", "Final volume",final_vol,"m3");
	    printf("%20s %10.1f %s\n", "Volume change",final_vol - initial_vol,"m3");
	    printf("%20s %10.3f %s\n", "Final water coverage", waterfrac,"");
	    printf("%20s %10.1f %s\n", "Mean water depth", meanwater*1000.,"mm");
	    printf("%20s %10.1f %s\n", "Max water depth", maxdepth,"mm ");
        printf("%20s %d\n", "Iteration limit", iterlim);
    }
}

double drain(int drainrow, int draincol) {
    // drains water to specified location
    int i,j;
    double drainn;
    double **waterslice, **demslice;
    waterslice = malloc(3 * sizeof(double *));
    demslice = malloc(3 * sizeof(double *));
    for (i=0; i<3; i++){
        waterslice[i] = malloc(3 * sizeof(double));
        demslice[i] = malloc(3 * sizeof(double));
    }
    // remove existing water
    for (i=0; i<3; i++){
        for (j=0; j<3; j++){
	        demslice[i][j] = bigdem[drainrow-1+i][draincol-1+j];
	        waterslice[i][j] = bigwater[drainrow-1+i][draincol-1+j];
	    }
    }
    drainn=0;
    for (i=0; i<3; i++){
        for (j=0; j<3; j++){
	        if(demslice[i][j] > missingvalue && waterslice[i][j] > 0){
	            drainn+=waterslice[i][j];
	        }
	    }
    }
    for (i=drainrow-1; i<=drainrow+1; i++){
        for (j=draincol-1; j<=draincol+1; j++){
	        bigwater[i][j] = 0.0 ;
	    }
    }
    for (i=0; i<3; i++){
      free(waterslice[i]);
      free(demslice[i]);
    }
    free(waterslice);
    free(demslice);
    return drainn;
}



void runoffa(int centerrow, int centercol, double missingvalue) {
     // examines at a 3x3 section of the water array around the passed location
    // and only drains water away from center location
    int rowloc, colloc;
    double ht_diff, center_water_elev, cell_water_elev, flow;

    for (rowloc=centerrow-1; rowloc<=centerrow+1; rowloc++){
	    for (colloc=centercol-1; colloc<=centercol+1; colloc++){
	    // make sure centre element is not included
	        if (((rowloc != centerrow) || (colloc != centercol)) &&
		                    (bigdem[rowloc][colloc] > missingvalue)){
		        cell_water_elev = bigdem[rowloc][colloc] + bigwater[rowloc][colloc];
		        center_water_elev =  bigdem[centerrow][centercol] + bigwater[centerrow][centercol];
		        ht_diff =  center_water_elev - cell_water_elev;
		        if (ht_diff > 0) {
		            if (bigdem[centerrow][centercol] > cell_water_elev) {
		                flow = bigwater[centerrow][centercol]/8.0;
		            }
		            else{
		                flow = ((bigdem[centerrow][centercol] - bigdem[rowloc][colloc]) +
			                (bigwater[centerrow][centercol] - bigwater[rowloc][colloc]))/8.0;
		            }
		            flow = min(max(flow, 0.0), bigwater[centerrow][centercol]);
		            bigwater[centerrow][centercol] = max(bigwater[centerrow][centercol] - flow, 0.0);
		            bigwater[rowloc][colloc] = bigwater[rowloc][colloc] + flow;
		        }
	        }
	    }
    }
}


void runoffs(int centerrow, int centercol, double missingvalue) {
     // examines at a 3x3 section of the water array around the passed location
    // and only drains water away from center location
    int rowloc, colloc;
    double ht_diff, center_water_elev, cell_water_elev, flow;

    for (rowloc=centerrow-1; rowloc<=centerrow+1; rowloc++){
	    for (colloc=centercol-1; colloc<=centercol+1; colloc++){
	    // make sure centre element is not included
	        if (((rowloc != centerrow) || (colloc != centercol)) &&
		                    (bigdem[rowloc][colloc] > missingvalue)){
		        ht_diff =  (bigdem[centerrow][centercol] + bigwater[centerrow][centercol])-
		            (bigdem[rowloc][colloc] + bigwater[rowloc][colloc]);
		        if (ht_diff > 0) {
		            if (bigdem[centerrow][centercol] > (bigdem[rowloc][colloc]+bigwater[rowloc][colloc])) {
		                flow = bigwater[centerrow][centercol]/8.0;
		            }
		            else{
		                flow = ((bigdem[centerrow][centercol] - bigdem[rowloc][colloc]) +
			                (bigwater[centerrow][centercol] - bigwater[rowloc][colloc]))/8.0;
		                flow = ht_diff/8.0;
		            }
		            flow = min(flow, bigwater[centerrow][centercol]);
		            bigwater[centerrow][centercol] = bigwater[centerrow][centercol] - flow;
		            bigwater[rowloc][colloc] = bigwater[rowloc][colloc] + flow;
		        }
	        }
	    }
    }
}


void runoffd(int centerrow, int centercol, int drainrow, int draincol, double missingvalue) {
     // examines at a 3x3 section of the water array around the passed location
    // and only drains water away from center location
    int rowloc, colloc;
    double ht_diff, center_water_elev, cell_water_elev, flow;
    for (rowloc=centerrow-1; rowloc<=centerrow+1; rowloc++){
	    for (colloc=centercol-1; colloc<=centercol+1; colloc++){
	    // make sure centre element is not included
	        if (((rowloc != centerrow) || (colloc != centercol)) &&
		                    (bigdem[rowloc][colloc] > missingvalue)){
		        center_water_elev =  bigdem[centerrow][centercol] + bigwater[centerrow][centercol];
		        cell_water_elev = bigdem[rowloc][colloc] + bigwater[rowloc][colloc];
	            // check for drain
	            if((colloc == draincol) && (rowloc == drainrow)){
		            // drain all water in center cell and edge cell
		            totaldrain = totaldrain + bigwater[drainrow][draincol] + bigwater[centerrow][centercol];
		            bigwater[drainrow][draincol] = 0.0;
		            bigwater[centerrow][centercol] = 0.0;
		        }
		        else{
		            ht_diff =  center_water_elev - cell_water_elev;
		            if (ht_diff > 0) {
		                if (bigdem[centerrow][centercol] > cell_water_elev) {
			                flow = bigwater[centerrow][centercol]/8.0;
		                }
		                else{
			                flow = ((bigdem[centerrow][centercol] - bigdem[rowloc][colloc]) +
				                (bigwater[centerrow][centercol] - bigwater[rowloc][colloc]))/8.0;
		                }
		                flow = min(max(flow, 0.0), bigwater[centerrow][centercol]);
		                bigwater[centerrow][centercol] = max(bigwater[centerrow][centercol] - flow, 0.0);
		                bigwater[rowloc][colloc] = bigwater[rowloc][colloc] + flow;
		            }
		        }
	        }
	    }
    }
}
